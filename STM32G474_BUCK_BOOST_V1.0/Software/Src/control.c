/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     control.c  Version: V1.1  Date: 2024/07/29
* Brief:    BUCK-BOOST 电源控制主模块
*           由 TIM7 每 1ms 调用 CTRL_Run()，完成模式判定、PID 调节、
*           PWM 更新以及静态桥臂 GPIO 切换。
*================================================================*/
#include "control.h"

/* 已确认的硬件映射关系：
 *   TE2 -> S1，TE1 -> S2
 *   TF1 -> S3，TF2 -> S4
 * 当前控制策略：
 *   BUCK  (Vout < Vin): TE 半桥输出 PWM，TF 走 GPIO 静态桥臂
 *   BOOST (Vout > Vin): TF 半桥输出 PWM，TE 走 GPIO 静态桥臂 */
#define CTRL_SINGLE_HALFBRIDGE_TEST   0

/* 单桥调试开关：1=进入单桥测试，0=正常闭环控制 */
#define CTRL_TEST_TIMER_E             1

/* Fixed compare value for test PWM (Period=30000):
 * 15000 ~= 50% nominal duty point
 */
#define CTRL_TEST_CMP1                (HRTIM_PERIOD / 2UL)

/* 闭环控制安全边界：
 * STATIC_UPPER_ON_CMP / STATIC_LOWER_ON_CMP 仅用于比较寄存器初始化；
 * 真正的静态上管现在由 GPIO 直接控制，不再依赖 PWM 伪静态。 */
#define CTRL_STATIC_UPPER_ON_CMP      1UL
#define CTRL_STATIC_LOWER_ON_CMP      (HRTIM_PERIOD - 1UL)
#define CTRL_ACTIVE_CMP_MIN           200UL
#define CTRL_ACTIVE_CMP_MAX           (HRTIM_PERIOD - 200UL)
#define CTRL_MODE_SWITCH_BLANK_TICKS  5U
#define CTRL_BB_TF1_PHASE_COMP        0UL
#define CTRL_DEBUG_PRINT_DIV          200U
#define CTRL_BOOST_STARTUP_HOLD_TICKS 40U

/* 控制器全局实例与内部辅助函数声明 */
CTRL_TypeDef CTRL;
static void CTRL_AutoSelectMode(void);
static void CTRL_ApplyDuty(void);
static void CTRL_SetBridgeAsPwm(GPIO_TypeDef *port, uint16_t pin_low, uint16_t pin_high, uint32_t alternate);
static void CTRL_SetBridgeAsStatic(GPIO_TypeDef *port, uint16_t pin_low, uint16_t pin_high, GPIO_PinState low_state, GPIO_PinState high_state);
static void CTRL_ApplyOutputTopology(CTRL_ModeTypeDef mode, uint8_t pwm_enable);
static const char *CTRL_ModeName(CTRL_ModeTypeDef mode);
static CTRL_ModeTypeDef s_lastAppliedMode = CTRL_MODE_OFF;
static uint8_t s_modeSwitchBlankCnt = 0;
static uint8_t s_outputsGated = 0;
static uint16_t s_debugPrintDiv = 0;
static uint8_t s_boostStartupHoldCnt = 0;
#define CTRL_OUTPUT_GATE_NONE        0U
#define CTRL_OUTPUT_GATE_STARTUP     1U
#define CTRL_OUTPUT_GATE_MODE_SWITCH 2U

/* 初始化控制器、PID 参数和模式状态。 */
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
    CTRL.Mode=CTRL_MODE_OFF;
    CTRL.OutputMode = CTRL_OUTPUT_MODE_DCM;
    CTRL.WorkMode = CTRL_WORK_MODE_AUTO;
    CTRL.DutyTE=DUTY_MIN;
    CTRL.DutyTF=DUTY_MIN; CTRL.HRTIMActive=0;
    s_lastAppliedMode = CTRL_MODE_OFF;
    s_modeSwitchBlankCnt = 0;
    s_outputsGated = 0;
    s_debugPrintDiv = 0;
    s_boostStartupHoldCnt = 0;
}

/* 启动输出：
 * 1. 依据 Vref / Vin 判断 BUCK 或 BOOST
 * 2. 先只建立静态桥臂通路
 * 3. 延时若干个控制周期后再放开 PWM 半桥 */
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
    printf("[CTRL_START] Vref=%.3fV Vin=%.3fV Out=%s -> %s\r\n",
           CTRL.VoutRef, ADCSAP.Conv.VoltageIn,
           (CTRL.OutputMode == CTRL_OUTPUT_MODE_CCM) ? "CCM" : "DCM",
           (CTRL.Mode == CTRL_MODE_BOOST) ? "BOOST" :
           (CTRL.Mode == CTRL_MODE_BB) ? "BB" : "BUCK");
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
        case CTRL_MODE_BOOST: {
            /* BOOST startup: use Vin/Vref feedforward to avoid TF duty slamming to an extreme at Vout≈0 */
            float d_boost = 1.0f - (ADCSAP.Conv.VoltageIn / CTRL.VoutRef);
            if (d_boost < 0.10f) d_boost = 0.10f;
            if (d_boost > 0.45f) d_boost = 0.45f;
            CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.DutyTF = (uint32_t)((1.0f - d_boost) * (float)HRTIM_PERIOD);
            break;
        }
        case CTRL_MODE_BB: {
            /* BB startup: AUTO 下保持中间区，强制 BB 时允许全范围 buck-boost */
            float d_bb = CTRL.VoutRef / (ADCSAP.Conv.VoltageIn + CTRL.VoutRef);
            if (CTRL.WorkMode == CTRL_WORK_MODE_BB) {
                if (d_bb < 0.10f) d_bb = 0.10f;
                if (d_bb > 0.90f) d_bb = 0.90f;
            } else {
                if (d_bb < 0.45f) d_bb = 0.45f;
                if (d_bb > 0.55f) d_bb = 0.55f;
            }
            CTRL.DutyTE = (uint32_t)((1.0f - d_bb) * (float)HRTIM_PERIOD);
            CTRL.DutyTF = CTRL.DutyTE;
            break;
        }
        default:
            CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.DutyTF = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.Mode   = CTRL_MODE_BUCK;
            break;
    }

    CTRL_ApplyDuty();
    HAL_HRTIM_WaveformCounterStart(&hhrtim1,
        HRTIM_TIMERID_MASTER|HRTIM_TIMERID_TIMER_E|HRTIM_TIMERID_TIMER_F);
    /* Stage-1 startup: keep only the static upper path active.
     * The PWM half-bridge is enabled a few control ticks later. */
    CTRL_ApplyOutputTopology(CTRL.Mode, 0U);
    s_outputsGated = CTRL_OUTPUT_GATE_STARTUP;
    s_modeSwitchBlankCnt = CTRL_MODE_SWITCH_BLANK_TICKS;
    s_boostStartupHoldCnt = (CTRL.Mode == CTRL_MODE_BOOST) ? CTRL_BOOST_STARTUP_HOLD_TICKS : 0U;
    CTRL.PIDVout.Enable=1; CTRL.HRTIMActive=1;
#endif
}

/* 停止输出：关闭 PWM、关闭计数器，并清空 PID 状态。 */
void CTRL_Stop(void) {
    if (!CTRL.HRTIMActive) {
        CTRL_ApplyOutputTopology(CTRL_MODE_OFF, 0U);
        CTRL.Mode = CTRL_MODE_OFF;
        return;
    }
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
    CTRL_ApplyOutputTopology(CTRL_MODE_OFF, 0U);
    CTRL.Mode = CTRL_MODE_OFF;
    CTRL.HRTIMActive = 0;
    s_lastAppliedMode = CTRL_MODE_OFF;
    s_modeSwitchBlankCnt = 0;
    s_outputsGated = 0;
    s_boostStartupHoldCnt = 0;
}

/* 设置输出电压目标值，同时同步到电压环参考值。 */
void CTRL_SetVoutRef(float vref){CTRL.VoutRef=vref; CTRL.PIDVout.Ref=vref;}
/* 手动设置运行模式；若处于 AUTO 逻辑，CTRL_Run 仍可重新判模。 */
void CTRL_SetMode(CTRL_ModeTypeDef mode){CTRL.Mode=mode;}
void CTRL_SetWorkMode(CTRL_WorkModeTypeDef mode){CTRL.WorkMode=mode;}

static const char *CTRL_ModeName(CTRL_ModeTypeDef mode)
{
    switch (mode) {
        case CTRL_MODE_BUCK:  return "BUCK";
        case CTRL_MODE_BOOST: return "BOOST";
        case CTRL_MODE_BB:    return "BB";
        default:              return "OFF";
    }
}

/**
  * @brief  1ms 主控制循环。
  * @note   主要流程：
  *         1. 过流保护与软限流
  *         2. 参考值斜率限制
  *         3. 自动判定 BUCK / BOOST
  *         4. 模式切换时先关断，再延时恢复
  *         5. 运行 PID，换算为 HRTIM 比较值并更新输出
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

    /* 软限流：当输出电流超过限流阈值时，下调电压环参考值。 */
    if (ADCSAP.Conv.CurrentOut > CTRL.IoutLimit) {
        float r = (ADCSAP.Conv.CurrentOut - CTRL.IoutLimit) * 0.5f;
        CTRL.PIDVout.Ref = CTRL.VoutRef - r;
        if (CTRL.PIDVout.Ref < 0.0f) CTRL.PIDVout.Ref = 0.0f;
    } else {
        CTRL.PIDVout.Ref = CTRL.VoutRef;
    }

    /* CCM 模式下调参后持续运行；DCM 模式在调参确认后会回到 OFF。 */

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
        CTRL_ApplyOutputTopology(CTRL.Mode, 1U);
        s_outputsGated = CTRL_OUTPUT_GATE_NONE;
    }

    if ((CTRL.Mode == CTRL_MODE_BOOST) && (s_boostStartupHoldCnt > 0U)) {
        s_boostStartupHoldCnt--;
        CTRL_ApplyDuty();
        CTRL_ApplyOutputTopology(CTRL.Mode, 1U);
        return;
    }

    float pidOut = PID_Compute(&CTRL.PIDVout, ADCSAP.Conv.VoltageOut);

    /* BUCK、BOOST、BB 三种模式下的 PWM 比较值：
     * - BUCK：S1(TE2)/S2(TE1) 互补，TE2 的有效占空比 = (Period-CMP)/Period
     * - BOOST：S3(TF1)/S4(TF2) 互补，TF1 的有效占空比 = (Period-CMP)/Period
     * - BB：采用中心点前馈占空 d≈Vout/(Vin+Vout)，避免在 Vin≈Vout 时占空大幅跳变 */
    uint32_t pwmCmpBuck  = (uint32_t)(HRTIM_PERIOD - (uint32_t)pidOut);
    uint32_t pwmCmpBoost = (uint32_t)(HRTIM_PERIOD - (uint32_t)pidOut);
    if ((CTRL.Mode == CTRL_MODE_BOOST) && (ADCSAP.Conv.VoltageOut < (CTRL.VoutRef * 0.8f))) {
        float d_boost_ff = 1.0f - (ADCSAP.Conv.VoltageIn / CTRL.VoutRef);
        if (d_boost_ff < 0.10f) d_boost_ff = 0.10f;
        if (d_boost_ff > 0.45f) d_boost_ff = 0.45f;
        d_boost_ff += 0.05f;
        if (d_boost_ff > 0.50f) d_boost_ff = 0.50f;
        uint32_t pwmCmpBoostMin = (uint32_t)((1.0f - d_boost_ff) * (float)HRTIM_PERIOD);
        if (pwmCmpBoost < pwmCmpBoostMin) pwmCmpBoost = pwmCmpBoostMin;
    }
    float bb_d = CTRL.VoutRef / (ADCSAP.Conv.VoltageIn + CTRL.VoutRef);
    if (CTRL.WorkMode == CTRL_WORK_MODE_BB) {
        if (bb_d < 0.10f) bb_d = 0.10f;
        if (bb_d > 0.90f) bb_d = 0.90f;
    } else {
        bb_d += (CTRL.VoutRef - ADCSAP.Conv.VoltageOut) * 0.01f;
        if (bb_d < 0.45f) bb_d = 0.45f;
        if (bb_d > 0.55f) bb_d = 0.55f;
    }
    uint32_t pwmCmpBB = (uint32_t)((1.0f - bb_d) * (float)HRTIM_PERIOD);
    uint32_t pwmCmpBB_TE = pwmCmpBB;
    uint32_t pwmCmpBB_TF = pwmCmpBB;

    switch (CTRL.Mode) {
        case CTRL_MODE_BUCK:
            /* BUCK：TE 半桥输出 PWM，TF 由 GPIO 保持静态通路 */
            CTRL.DutyTE = pwmCmpBuck;
            CTRL.DutyTF = CTRL_STATIC_UPPER_ON_CMP;
            break;
        case CTRL_MODE_BOOST:
            /* BOOST：TF 半桥输出 PWM，TE 由 GPIO 保持静态通路 */
            CTRL.DutyTE = CTRL_STATIC_UPPER_ON_CMP;
            CTRL.DutyTF = pwmCmpBoost;
            break;
        case CTRL_MODE_BB:
            /* BB：以中心点前馈占空维持 S1/S3 同步，避免 Vin≈Vout 时来回横跳 */
            CTRL.DutyTE = pwmCmpBB_TE;
            CTRL.DutyTF = pwmCmpBB_TF;
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
    CTRL_ApplyOutputTopology(CTRL.Mode, 1U);
    s_lastAppliedMode = CTRL.Mode;

    if (++s_debugPrintDiv >= CTRL_DEBUG_PRINT_DIV) {
        float vin = ADCSAP.Conv.VoltageIn;
        float dv  = 0.2f * vin;
        float d_s1 = ((float)(HRTIM_PERIOD - CTRL.DutyTE) / (float)HRTIM_PERIOD) * 100.0f; /* TE2=S1 */
        float d_s3 = ((float)(HRTIM_PERIOD - CTRL.DutyTF) / (float)HRTIM_PERIOD) * 100.0f; /* TF1=S3 */
        s_debugPrintDiv = 0;
        printf("[CTRL_DBG] mode=%s out=%s Vin=%.3f Vout=%.3f Vref=%.3f dV=%.3f TE=%lu TF=%lu dS1=%.2f%% dS3=%.2f%%\r\n",
               CTRL_ModeName(CTRL.Mode),
               (CTRL.OutputMode == CTRL_OUTPUT_MODE_CCM) ? "CCM" : "DCM",
               vin, ADCSAP.Conv.VoltageOut, CTRL.VoutRef, dv,
               (unsigned long)CTRL.DutyTE, (unsigned long)CTRL.DutyTF,
               d_s1, d_s3);
    }
#endif
}

/* 按当前目标电压与输入电压自动判定工作模式：
 * ΔV = 10% * Vin
 * Vin <  Vout - ΔV -> BOOST：S1 常开、S2 常关，S3/S4 互补导通
 * Vin >  Vout + ΔV -> BUCK：S3 常关、S4 常开，S1/S2 互补导通
 * 其余区间        -> BB：S1/S3 同时导通同时关断，S2/S4 分别互补导通 */
static void CTRL_AutoSelectMode(void) {
    if (CTRL.WorkMode == CTRL_WORK_MODE_BB) {
        CTRL.Mode = CTRL_MODE_BB;
        return;
    }

    float vin = ADCSAP.Conv.VoltageIn;
    float dv  = 0.2f * vin;

    if (vin < (CTRL.VoutRef - dv)) {
        CTRL.Mode = CTRL_MODE_BOOST;
    } else if (vin > (CTRL.VoutRef + dv)) {
        CTRL.Mode = CTRL_MODE_BUCK;
    } else {
        CTRL.Mode = CTRL_MODE_BB;
    }
}

/* 将控制器内部的比较值写入 HRTIM：
 * Timer-E CMP1 <- DutyTE
 * Timer-F CMP1 <- DutyTF */
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

/* 将一组桥臂切回 HRTIM 复用功能，供 PWM 使用。 */
static void CTRL_SetBridgeAsPwm(GPIO_TypeDef *port, uint16_t pin_low, uint16_t pin_high, uint32_t alternate) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin_low | pin_high;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = alternate;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

/* 将一组桥臂切为普通 GPIO，并直接输出静态高低电平。 */
static void CTRL_SetBridgeAsStatic(GPIO_TypeDef *port, uint16_t pin_low, uint16_t pin_high, GPIO_PinState low_state, GPIO_PinState high_state) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = pin_low | pin_high;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
    HAL_GPIO_WritePin(port, pin_low, low_state);
    HAL_GPIO_WritePin(port, pin_high, high_state);
}

/* 根据当前模式选择输出拓扑：
 * pwm_enable=0 时只保留静态桥臂，pwm_enable=1 时放开 PWM 半桥。 */
static void CTRL_ApplyOutputTopology(CTRL_ModeTypeDef mode, uint8_t pwm_enable) {
    switch (mode) {
        case CTRL_MODE_BUCK:
            /* BUCK：TE 为 PWM 半桥，TF 走 GPIO 静态通路（TF1=0，TF2=1） */
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            CTRL_SetBridgeAsStatic(PWM2_L_GPIO_Port, PWM2_L_Pin, PWM2_H_Pin, GPIO_PIN_RESET, GPIO_PIN_SET);
            CTRL_SetBridgeAsPwm(PWM1_L_GPIO_Port, PWM1_L_Pin, PWM1_H_Pin, GPIO_AF3_HRTIM1);
            if (pwm_enable)
                HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
            else
                HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
            break;
        case CTRL_MODE_BOOST:
            /* BOOST：TE 静态桥臂保持 S1(TE2) 常开、S2(TE1) 常关；TF 为 S3/S4 互补 PWM */
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);
            CTRL_SetBridgeAsStatic(PWM1_L_GPIO_Port, PWM1_L_Pin, PWM1_H_Pin, GPIO_PIN_RESET, GPIO_PIN_SET);
            CTRL_SetBridgeAsPwm(PWM2_L_GPIO_Port, PWM2_L_Pin, PWM2_H_Pin, GPIO_AF13_HRTIM1);
            if (pwm_enable)
                HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            else
                HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            break;
        case CTRL_MODE_BB:
            /* BB：目标是让 S1(TE2) 与 S3(TF1) 同相，S2/TE1 与 S4/TF2 分别互补 */
            CTRL_SetBridgeAsPwm(PWM1_L_GPIO_Port, PWM1_L_Pin, PWM1_H_Pin, GPIO_AF3_HRTIM1);
            CTRL_SetBridgeAsPwm(PWM2_L_GPIO_Port, PWM2_L_Pin, PWM2_H_Pin, GPIO_AF13_HRTIM1);
            if (pwm_enable)
                HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2|HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            else
                HAL_HRTIM_WaveformOutputStop(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2|HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            break;
        default:
            HAL_HRTIM_WaveformOutputStop(&hhrtim1,
                HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2|HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);
            CTRL_SetBridgeAsStatic(PWM1_L_GPIO_Port, PWM1_L_Pin, PWM1_H_Pin, GPIO_PIN_RESET, GPIO_PIN_RESET);
            CTRL_SetBridgeAsStatic(PWM2_L_GPIO_Port, PWM2_L_Pin, PWM2_H_Pin, GPIO_PIN_RESET, GPIO_PIN_RESET);
            break;
    }
}

/**
  * @brief  电压环 PID 计算，带输出限幅与积分抗饱和。
  * @param  pid      PID 对象
  * @param  feedback 当前输出电压反馈
  * @retval PID 输出值，对应后续的 PWM 控制量
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
    /* 若试算输出已打到上下限，则暂停积分，避免积分继续累加。 */
    if (tryOut>pid->OutMin && tryOut<pid->OutMax)
        pid->Integral+=pid->Ki*pid->Ts*pid->Err;
    pid->Out=p+pid->Integral+d;
    if      (pid->Out>pid->OutMax) pid->Out=pid->OutMax;
    else if (pid->Out<pid->OutMin) pid->Out=pid->OutMin;
    pid->ErrPrev2=pid->ErrPrev; pid->ErrPrev=pid->Err;
    return pid->Out;
}

/* 清空 PID 内部状态，用于启动、停机或模式切换后的重新进入。 */
void PID_Reset(PID_TypeDef *pid) {
    pid->Err=pid->ErrPrev=pid->ErrPrev2=pid->Integral=pid->Out=0.0f;
}
