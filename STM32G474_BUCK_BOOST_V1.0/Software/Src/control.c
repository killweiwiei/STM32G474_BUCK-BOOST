/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     control.c  Version: V1.1  Date: 2024/07/29
* Brief:    ?????BUCK-BOOST PID?????????
* ????: TIM7 1ms -> CTRL_Run() -> ?????->PID->HRTIM????
*================================================================*/
#include "control.h"

/* Confirmed hardware mapping:
 *   TE2 -> left upper,  TE1 -> left lower
 *   TF2 -> right upper, TF1 -> right lower
 * Control strategy used here:
 *   BUCK  (Vout < Vin): TE complementary PWM, TF2 static ON
 *   BOOST (Vout > Vin): TF complementary PWM, TE2 static ON */
#define CTRL_SINGLE_HALFBRIDGE_TEST   0

/* Select which half-bridge to test:
 * 1 -> Timer E (PWM1H/PWM1L)
 * 0 -> Timer F (PWM2H/PWM2L)
 */
#define CTRL_TEST_TIMER_E             1

/* Fixed compare value for test PWM (Period=30000):
 * 15000 ~= 50% nominal duty point
 */
#define CTRL_TEST_CMP1                (HRTIM_PERIOD / 2UL)

/* Closed-loop safe compare range (avoid edge states) */
#define CTRL_STATIC_UPPER_ON_CMP      1UL
#define CTRL_STATIC_LOWER_ON_CMP      (HRTIM_PERIOD - 1UL)
#define CTRL_ACTIVE_CMP_MIN           2000UL
#define CTRL_ACTIVE_CMP_MAX           (HRTIM_PERIOD - 2000UL)
#define CTRL_MODE_SWITCH_BLANK_TICKS  5U

CTRL_TypeDef CTRL;
static void CTRL_AutoSelectMode(void);
static void CTRL_ApplyDuty(void);
static CTRL_ModeTypeDef s_lastAppliedMode = CTRL_MODE_OFF;
static uint8_t s_modeSwitchBlankCnt = 0;
static uint8_t s_outputsGated = 0;
#define CTRL_OUTPUT_GATE_NONE        0U
#define CTRL_OUTPUT_GATE_STARTUP     1U
#define CTRL_OUTPUT_GATE_MODE_SWITCH 2U

/* @brief ???????????PID??????????TIM7?????????
 * ???????(200kHz,12V/5A): Kp=0.05, Ki=0.50, Kd=0.001 */
void CTRL_Init(void) {
    /* Tuning for HRTIM scale (Period=30000, Ts=1ms)
     * Conservative 1kHz-loop startup values:
     * lower Kp avoids mode-edge jumping, higher Ki restores follow speed,
     * Kd disabled first to avoid ADC-noise amplification */
    CTRL.PIDVout.Kp=900.0f; CTRL.PIDVout.Ki=400.0f; CTRL.PIDVout.Kd=0.0f;
    CTRL.PIDVout.Ts=0.001f;
    CTRL.PIDVout.OutMin=(float)CTRL_ACTIVE_CMP_MIN;
    CTRL.PIDVout.OutMax=(float)CTRL_ACTIVE_CMP_MAX;
    CTRL.PIDVout.Enable=0;
    PID_Reset(&CTRL.PIDVout);
    CTRL.VoutRef=VOUT_DEFAULT_REF; CTRL.IoutLimit=IOUT_MAX_REF;
    CTRL.PIDVout.Ref=CTRL.VoutRef;
    CTRL.Mode=CTRL_MODE_OFF; CTRL.DutyTE=DUTY_MIN;
    CTRL.DutyTF=DUTY_MIN; CTRL.HRTIMActive=0;
    s_lastAppliedMode = CTRL_MODE_OFF;
    s_modeSwitchBlankCnt = 0;
    s_outputsGated = 0;
}

/* @brief ???HRTIM PWM?????????PID??????
 * ???PID?????????????????HRTIM?????? */
void CTRL_Start(void) {
    if (CTRL.HRTIMActive) return;

#if CTRL_SINGLE_HALFBRIDGE_TEST
    /* Single half-bridge safe test: bypass PID and mode switching */
    PID_Reset(&CTRL.PIDVout);
    CTRL.PIDVout.Enable = 0;
    CTRL.Mode = CTRL_MODE_OFF;

#if CTRL_TEST_TIMER_E
    /* Test Timer E (PWM1H/PWM1L), keep Timer F disabled */
    CTRL.DutyTE = CTRL_TEST_CMP1;
    CTRL.DutyTF = HRTIM_PERIOD - 1UL;
    CTRL_ApplyDuty();

    HAL_HRTIM_WaveformOutputStart(&hhrtim1,HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
    HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
#else
    /* Test Timer F (PWM2H/PWM2L), keep Timer E disabled */
    CTRL.DutyTE = HRTIM_PERIOD - 1UL;
    CTRL.DutyTF = CTRL_TEST_CMP1;
    CTRL_ApplyDuty();

    HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
    HAL_HRTIM_WaveformOutputStart(&hhrtim1,HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
#endif

    HAL_HRTIM_WaveformCounterStart(&hhrtim1,
        HRTIM_TIMERID_MASTER|HRTIM_TIMERID_TIMER_E|HRTIM_TIMERID_TIMER_F);
    CTRL.HRTIMActive = 1;
    return;
#else
    PID_Reset(&CTRL.PIDVout);
    /* Decide mode from current Vref and Vin each time output is enabled. */
    CTRL_AutoSelectMode();
    printf("[CTRL_START] Vref=%.3fV Vin=%.3fV -> %s\r\n",
           CTRL.VoutRef, ADCSAP.Conv.VoltageIn,
           (CTRL.Mode == CTRL_MODE_BOOST) ? "BOOST" : "BUCK");
    CTRL.PIDVout.Integral = 0.0f;
    s_lastAppliedMode = CTRL.Mode;
    s_modeSwitchBlankCnt = 0;
    s_outputsGated = 0;

    switch (CTRL.Mode) {
        case CTRL_MODE_BUCK:
            /* BUCK startup: TF static upper ON, TE PWM leg near-zero power */
            CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.DutyTF = CTRL_STATIC_UPPER_ON_CMP;
            break;
        case CTRL_MODE_BOOST:
            /* BOOST startup: TE static upper ON, TF PWM leg near-zero power */
            CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.DutyTF = CTRL_STATIC_UPPER_ON_CMP;
            break;
        default:
            CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.DutyTF = (uint32_t)CTRL_ACTIVE_CMP_MAX;
            CTRL.Mode   = CTRL_MODE_BUCK;
            break;
    }

    CTRL_ApplyDuty();
    HAL_HRTIM_WaveformCounterStart(&hhrtim1,
        HRTIM_TIMERID_MASTER|HRTIM_TIMERID_TIMER_E|HRTIM_TIMERID_TIMER_F);
    /* Stage-1 startup: enable only the static-path half-bridge first.
     * The PWM half-bridge is enabled a few control ticks later. */
    if (CTRL.Mode == CTRL_MODE_BUCK) {
        HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
    } else {
        HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
    }
    s_outputsGated = CTRL_OUTPUT_GATE_STARTUP;
    s_modeSwitchBlankCnt = CTRL_MODE_SWITCH_BLANK_TICKS;
    CTRL.PIDVout.Enable=1; CTRL.HRTIMActive=1;
#endif
}

/* @brief ???HRTIM PWM???????PID??????????????? */
void CTRL_Stop(void) {
    if (!CTRL.HRTIMActive) return;
    CTRL.PIDVout.Enable = 0;
    CTRL.PIDVout.Ref = 0.0f;
    PID_Reset(&CTRL.PIDVout);
    CTRL.DutyTE = 0UL;
    CTRL.DutyTF = HRTIM_PERIOD - 1UL;
    CTRL_ApplyDuty();
    HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
    HAL_HRTIM_WaveformOutputStop(&hhrtim1,HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
    HAL_HRTIM_WaveformCounterStop(&hhrtim1,
        HRTIM_TIMERID_MASTER|HRTIM_TIMERID_TIMER_E|HRTIM_TIMERID_TIMER_F);
    CTRL.Mode = CTRL_MODE_OFF;
    CTRL.HRTIMActive = 0;
    s_lastAppliedMode = CTRL_MODE_OFF;
    s_modeSwitchBlankCnt = 0;
    s_outputsGated = 0;
}

/* @brief ?????????????????? */
void CTRL_SetVoutRef(float vref){CTRL.VoutRef=vref; CTRL.PIDVout.Ref=vref;}
/* @brief ??????????? */
void CTRL_SetMode(CTRL_ModeTypeDef mode){CTRL.Mode=mode;}

/**
  * @brief  ??????????????10ms??TIM7?????
  * @note   ??????:
  *   1. ???: HRTIM/PID???????????
  *   2. ???????droop: ????????????????????
  *   3. ??????BUCK/BOOST/BB??(5%????????????)
  *   4. PID????(?????Vout?????)
  *   5. ?????????PID?????Buck/Boost????
  *   6. ??HRTIM???????
  * @retval void
  */
void CTRL_Run(void) {
#if CTRL_SINGLE_HALFBRIDGE_TEST
    return;
#else
    if (!CTRL.HRTIMActive || !CTRL.PIDVout.Enable) return;

    /* Hard OCP with debounce: avoid false trip during boost / mode switching */
    static uint8_t s_ocp_cnt = 0;
    if (ADCSAP.Conv.CurrentOut > CTRL.IoutLimit * 2.0f) {
        if (++s_ocp_cnt >= 8U) {
            CTRL_Stop();
            printf("[OCP] Iout=%.2fA > limit=%.2fA, SHUTDOWN\r\n",
                   ADCSAP.Conv.CurrentOut, CTRL.IoutLimit * 2.0f);
            s_ocp_cnt = 0;
            return;
        }
    } else {
        s_ocp_cnt = 0;
    }

    /* ???? Soft current droop: reduce Vref when over limit ???? */
    if (ADCSAP.Conv.CurrentOut > CTRL.IoutLimit) {
        float r = (ADCSAP.Conv.CurrentOut - CTRL.IoutLimit) * 0.5f;
        CTRL.PIDVout.Ref = CTRL.VoutRef - r;
        if (CTRL.PIDVout.Ref < 0.0f) CTRL.PIDVout.Ref = 0.0f;
    } else {
        CTRL.PIDVout.Ref = CTRL.VoutRef;
    }

    /* Slew rate limit: max 0.2V/step (per 1ms = 200V/s max) */
    static float s_vref_slewed = VOUT_DEFAULT_REF;
    float vref_target = CTRL.PIDVout.Ref;
    float slew_max = 0.2f;  /* 0.2V/1ms = 200V/s max slew rate */
    if (vref_target > s_vref_slewed + slew_max)      s_vref_slewed += slew_max;
    else if (vref_target < s_vref_slewed - slew_max) s_vref_slewed -= slew_max;
    else                                              s_vref_slewed  = vref_target;
    CTRL.PIDVout.Ref = s_vref_slewed;

    /* Re-evaluate BUCK/BOOST from current Vref and Vin.
     * If target mode changes, use existing blanking sequence for a safe transition. */
    CTRL_AutoSelectMode();

    if (CTRL.Mode != s_lastAppliedMode) {
        /* Hard-safe transition: blank all outputs for several control ticks */
        HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2|HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
        s_outputsGated = CTRL_OUTPUT_GATE_MODE_SWITCH;
        s_modeSwitchBlankCnt = CTRL_MODE_SWITCH_BLANK_TICKS;
        s_lastAppliedMode = CTRL.Mode;
        return;
    }

    if (s_modeSwitchBlankCnt > 0U) {
        s_modeSwitchBlankCnt--;
        return;
    }

    if (s_outputsGated) {
        if (s_outputsGated == CTRL_OUTPUT_GATE_STARTUP) {
            if (CTRL.Mode == CTRL_MODE_BUCK) {
                HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
            } else {
                HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            }
        } else {
            HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2|HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
        }
        s_outputsGated = CTRL_OUTPUT_GATE_NONE;
    }

    float pidOut = PID_Compute(&CTRL.PIDVout, ADCSAP.Conv.VoltageOut);
    uint32_t pwmCmp = (uint32_t)(HRTIM_PERIOD - (uint32_t)pidOut);

    switch (CTRL.Mode) {
        case CTRL_MODE_BUCK:
            /* BUCK: TE complementary PWM, TF upper static ON */
            CTRL.DutyTE = pwmCmp;
            CTRL.DutyTF = CTRL_STATIC_UPPER_ON_CMP;
            break;
        case CTRL_MODE_BOOST:
            /* BOOST: TF complementary PWM, TE upper static ON */
            CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.DutyTF = pwmCmp;
            break;
        case CTRL_MODE_BB:
            if (CTRL.VoutRef <= ADCSAP.Conv.VoltageIn) {
                CTRL.DutyTE = pwmCmp;
                CTRL.DutyTF = CTRL_STATIC_UPPER_ON_CMP;
                CTRL.Mode   = CTRL_MODE_BUCK;
            } else {
                CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
                CTRL.DutyTF = pwmCmp;
                CTRL.Mode   = CTRL_MODE_BOOST;
            }
            break;
        default:
            CTRL_Stop();
            return;
    }

    if (CTRL.DutyTE < CTRL_ACTIVE_CMP_MIN && CTRL.DutyTE != CTRL_STATIC_LOWER_ON_CMP && CTRL.DutyTE != CTRL_STATIC_UPPER_ON_CMP)
        CTRL.DutyTE = CTRL_ACTIVE_CMP_MIN;
    if (CTRL.DutyTE > CTRL_ACTIVE_CMP_MAX && CTRL.DutyTE != CTRL_STATIC_LOWER_ON_CMP && CTRL.DutyTE != CTRL_STATIC_UPPER_ON_CMP)
        CTRL.DutyTE = CTRL_ACTIVE_CMP_MAX;
    if (CTRL.DutyTF < CTRL_ACTIVE_CMP_MIN && CTRL.DutyTF != CTRL_STATIC_LOWER_ON_CMP && CTRL.DutyTF != CTRL_STATIC_UPPER_ON_CMP)
        CTRL.DutyTF = CTRL_ACTIVE_CMP_MIN;
    if (CTRL.DutyTF > CTRL_ACTIVE_CMP_MAX && CTRL.DutyTF != CTRL_STATIC_LOWER_ON_CMP && CTRL.DutyTF != CTRL_STATIC_UPPER_ON_CMP)
        CTRL.DutyTF = CTRL_ACTIVE_CMP_MAX;

    CTRL_ApplyDuty();
    s_lastAppliedMode = CTRL.Mode;
#endif
}

/* Mode decision strictly from current target and input voltage:
 * Vref > Vin + 0.3V -> BOOST
 * otherwise         -> BUCK */
static void CTRL_AutoSelectMode(void) {
    float vin = ADCSAP.Conv.VoltageIn;

    if (CTRL.VoutRef > (vin + 0.3f)) CTRL.Mode = CTRL_MODE_BOOST;
    else                             CTRL.Mode = CTRL_MODE_BUCK;
}

/* ???HRTIM?????
 * Timer-E CMP1 <- DutyTE (TE?)
 * Timer-F CMP1 <- DutyTF (TF?) */
static void CTRL_ApplyDuty(void) {
    uint32_t cmp1_te = CTRL.DutyTE;
    uint32_t cmp1_tf = CTRL.DutyTF;

    if (cmp1_te < 1UL) cmp1_te = 1UL;
    if (cmp1_te > HRTIM_PERIOD - 1UL) cmp1_te = HRTIM_PERIOD - 1UL;
    if (cmp1_tf < 1UL) cmp1_tf = 1UL;
    if (cmp1_tf > HRTIM_PERIOD - 1UL) cmp1_tf = HRTIM_PERIOD - 1UL;

    __HAL_HRTIM_SETCOMPARE(&hhrtim1,
        HRTIM_TIMERINDEX_TIMER_E, HRTIM_COMPAREUNIT_1, cmp1_te);
    __HAL_HRTIM_SETCOMPARE(&hhrtim1,
        HRTIM_TIMERINDEX_TIMER_F, HRTIM_COMPAREUNIT_1, cmp1_tf);
}

/**
  * @brief  ???PID????????????????????(Anti-windup)
  * @note   ??????:
  *   err       = Ref - feedback
  *   integral += Ki*Ts*err  (????????????????)
  *   deriv     = Kd/Ts*(err-errPrev)  (??????????????????)
  *   out       = clamp(Kp*err+integral+deriv, OutMin, OutMax)
  * @param  pid      PID??????
  * @param  feedback ???????????
  * @retval ??????????????
  */
float PID_Compute(PID_TypeDef *pid, float feedback) {
    if (!pid->Enable) return pid->OutMin;
    pid->Fdb=feedback;
    pid->Err=pid->Ref-pid->Fdb;
    float p=pid->Kp*pid->Err;
    /* Clamp P term to 10% of output range per step */
    float p_limit = (pid->OutMax - pid->OutMin) * 0.10f;
    if      (p >  p_limit) p =  p_limit;
    else if (p < -p_limit) p = -p_limit;
    float d=(pid->Kd/pid->Ts)*(pid->Err-pid->ErrPrev);
    float tryOut=p+pid->Integral+d;
    /* ????????: ??????????????????????????? */
    if (tryOut>pid->OutMin && tryOut<pid->OutMax)
        pid->Integral+=pid->Ki*pid->Ts*pid->Err;
    pid->Out=p+pid->Integral+d;
    if      (pid->Out>pid->OutMax) pid->Out=pid->OutMax;
    else if (pid->Out<pid->OutMin) pid->Out=pid->OutMin;
    pid->ErrPrev2=pid->ErrPrev; pid->ErrPrev=pid->Err;
    return pid->Out;
}

/* @brief ??PID????????????????????????? */
void PID_Reset(PID_TypeDef *pid) {
    pid->Err=pid->ErrPrev=pid->ErrPrev2=pid->Integral=pid->Out=0.0f;
}
