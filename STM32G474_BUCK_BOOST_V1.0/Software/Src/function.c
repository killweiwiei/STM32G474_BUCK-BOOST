/******************** (C) COPYRIGHT 2024 Robot ********************

* File:     function.c

* Version:  V1.1

* Date:     2024/07/29

* Author:   Robot

 * Brief:    ??????????????

*           ????: ADC????/???, ????, ??????, LCD????,

*                 RGB LED(WS2812B), FAN???, TMP112???, SPI Flash

*================================================================*/



#include "function.h"
#include "ui_main.h"

#include "control.h"
#include "config_store.h"  /* persistent config load/save */   /* CTRL_Init, CTRL_Run, PID */



/* ??????

 * ADCSAP: ADC???????? (????? + ??????????)

 * BD    : ?????????  (FAN/LCD/KEY/ECDR/RGBLED/TMP/FLASH) */

ADC_SampleTypeDef ADCSAP;

BD_TypeDef        BD;



/* ????????????????? */

static void AdcSample_TypeStructInit(void);

static void BD_TypeStructInit(void);



/* ==============================================================

 *  BoardDevice_Init  ???????

 * ============================================================== */

/**

  * @brief  ????????????

  * @note   ???????????????:

  *   1. ADC???????????? + OPAMP??? + ADC?????

  *   2. BD???????????????????????

  *   3. LCD???????DEV -> ?? -> ????? -> ??????

  *   4. SPI Flash??????Flash ID

  *   5. TMP112?????????I2C DMA????

  *   6. ???????????: TIM6/TIM7/TIM16/TIM17

  *   7. ????PWM: TIM2(FAN) / TIM5(WS2812B)

  *   8. ??????????: TIM1

  *   9. PID???????CTRL_Init??

  *  10. ????????2s?????????????????????

  * @retval void

  */

/**
 * @brief  LCD???????????? LVGL ?????
 * @note   ????? -> LCD InitReg -> ???????? -> ????????
 *         ?????????????????????????????
 */
void __InitModule_LCD(void);  /* forward declaration */

void LCD_HW_Init(void)
{
    /* ??????? LCD ??????????????? BD ??? */
    __InitModule_LCD();
}

void BoardDevice_Init(void)

{

    AdcSample_TypeStructInit();   /* ADC???????? */

    BD_TypeStructInit();          /* BD????????????? */

    /* SPI Flash??????JEDEC ID???SPI??? */

    BD.SPIFLASH.FlashSelfcheck();

    printf("SPIFlashID = %#X\r\n", BD.SPIFLASH.FlashID);



    /* TMP112??????????????HAL_I2C_MemRxCpltCallback???? */

    BD.TMP.GetBoardTemperature();



    /* ???????????????:

     *   TIM6  100ms: ADC???????? + TMP112?????

     *   TIM7   10ms: PID?????

     *   TIM16  50ms: ?????????

     *   TIM17 200ms: FAN???? + RGB LED??? */

    HAL_TIM_Base_Start_IT(&htim6);   /* TIM6  100ms: ADC sample */

    HAL_TIM_Base_Start_IT(&htim7);   /* TIM7   10ms: PID loop */

    HAL_TIM_Base_Start_IT(&htim16);  /* TIM16  50ms: ??????? */

    HAL_TIM_Base_Start_IT(&htim17);  /* TIM17 200ms: FAN+RGB */



    /* PWM:

     *   TIM2 CH1: FAN????PWM??0~1000???0~100%??

     *   TIM5 CH1: WS2812B RGB LED??????? */

    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

    HAL_TIM_PWM_Start(&htim5, TIM_CHANNEL_1);



    /* ????????????TIM1??????????*/

    HAL_TIM_Encoder_Start_IT(&htim1, TIM_CHANNEL_ALL);



    /* PID???????????????TIM7 10ms????????*/

    /* PID/HRTIM init: default Vout=3.300V, auto mode select */
    CTRL_Init();

    /* Load persistent config AFTER CTRL_Init so defaults are not overwritten */
    if (Config_Load() == 0) {
        printf("[CFG] Using defaults: Kp=%.1f Ki=%.1f Kd=%.3f\r\n",
               CTRL.PIDVout.Kp, CTRL.PIDVout.Ki, CTRL.PIDVout.Kd);
    }
    /* Wait for ADC to stabilize before enabling control decisions */
    HAL_Delay(500);
    printf("CTRL auto-start disabled for safety: Vin=%.2fV Vref=%.3fV\r\n",
           ADCSAP.Conv.VoltageIn, CTRL.VoutRef);



    /* ????????LCD/Flash/????????*/

    /* DEV_Delay_ms(2000); removed */



    /* HRTIM BUCK-BOOST?????????????????CTRL_Start()??????:

     * HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TE1|HRTIM_OUTPUT_TE2);

     * HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TF1|HRTIM_OUTPUT_TF2);

     * HAL_HRTIM_WaveformCounterStart(&hhrtim1,

     *   HRTIM_TIMERID_MASTER|HRTIM_TIMERID_TIMER_E|HRTIM_TIMERID_TIMER_F); */

}



/* ==============================================================

 *  ADC ????

 * ============================================================== */

/**

  * @brief  ????5ADC DMA????

  * @note   DMA???????????????????

  *         HAL_ADC_ConvCpltCallback()

  *         ??????:

  *           ADC1 -> Vin  (PA2,  ADC1_IN3)

  *           ADC2 -> Iin  (PA6,  ADC2_IN3)

  *           ADC3 -> Vout (PB1,  ADC3_IN1)

  *           ADC4 -> Iout (PB12, ADC4_IN3)

  *           ADC5 -> MCU???????

  *         ?????ADC_SampleNumber????????

  * @retval void

  */

CCMRAM void __GetAdcSampleCode(void)

{

    HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADCSAP.Code.Vin,   ADC_SampleNumber); /* Vin  */

    HAL_ADC_Start_DMA(&hadc2, (uint32_t *)ADCSAP.Code.Iin,   ADC_SampleNumber); /* Iin  */

    HAL_ADC_Start_DMA(&hadc3, (uint32_t *)ADCSAP.Code.Vout,  ADC_SampleNumber); /* Vout */

    HAL_ADC_Start_DMA(&hadc4, (uint32_t *)ADCSAP.Code.Iout,  ADC_SampleNumber); /* Iout */

    HAL_ADC_Start_DMA(&hadc5, (uint32_t *)ADCSAP.Code.MTemp, ADC_SampleNumber); /* MCU???? */

}



/**

  * @brief  ADC??????????????

  * @note   ??HAL_ADC_ConvCpltCallback()??ADC??????????

  *

  *         ???: V = code*(Vref/4096)*ADC_VMULT

  *           ADC_VMULT = (R_top+R_bot)/R_bot = 75.0/4.7 (?????)

  *

  *         ????: I = code*(Vref/4096)*(ADC_IMULT/ADC_SAMPRES)

  *           ADC_IMULT   = 0.1/6.2  (???????)

  *           ADC_SAMPRES = 0.005  (????????)

  *

  *         MCU??????????????:

  *           T = (CAL2_TEMP-CAL1_TEMP)/(TS_CAL2-TS_CAL1)

  *               * (code*(3.3/3.0) - TS_CAL1) + 30

  *           ?: Vref=3.3V??????Vref=3.0V

  *

  *         ?: ??if-else-if?????????????????ADC

  * @param  hadc  ???????ADC???

  * @retval void

  */

CCMRAM void __ConvAdcSampleCode(ADC_HandleTypeDef *hadc)

{

    if (hadc == &hadc1)

    {

        /* ????Vin */

        ADCSAP.Code.VinAvg    = MiddleAverageFilter((const uint32_t *)ADCSAP.Code.Vin, ADC_SampleNumber);

        ADCSAP.Conv.VoltageIn = ADCSAP.Code.VinAvg * (ADC_VREF / 4096.0f) * ADC_VMULT;

    }

    else if (hadc == &hadc2)

    {

        /* Iin */

        ADCSAP.Code.IinAvg    = MiddleAverageFilter((const uint32_t *)ADCSAP.Code.Iin, ADC_SampleNumber);

        ADCSAP.Conv.CurrentIn = ADCSAP.Code.IinAvg * (ADC_VREF / 4096.0f) * (ADC_IMULT / ADC_SAMPRES);

    }

    else if (hadc == &hadc3)

    {

        /* ????Vout */

        ADCSAP.Code.VoutAvg    = MiddleAverageFilter((const uint32_t *)ADCSAP.Code.Vout, ADC_SampleNumber);

        ADCSAP.Conv.VoltageOut = ADCSAP.Code.VoutAvg * (ADC_VREF / 4096.0f) * ADC_VMULT;

    }

    else if (hadc == &hadc4)

    {

        /* Iout */

        ADCSAP.Code.IoutAvg    = MiddleAverageFilter((const uint32_t *)ADCSAP.Code.Iout, ADC_SampleNumber);

        ADCSAP.Conv.CurrentOut = ADCSAP.Code.IoutAvg * (ADC_VREF / 4096.0f) * (ADC_IMULT / ADC_SAMPRES);

    }

    else if (hadc == &hadc5)

    {

        /* MCU??????? (Vref: 3.3V????3.0V) */

        ADCSAP.Code.MTempAvg = MiddleAverageFilter((const uint32_t *)ADCSAP.Code.MTemp, ADC_SampleNumber);

        ADCSAP.Conv.TempMcu  = (float)(TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP)

                             / (float)(TS_CAL2 - TS_CAL1)

                             * ((float)ADCSAP.Code.MTempAvg * (3.3f / 3.0f) - (float)TS_CAL1)

                             + 30.0f;

    }

}



/**

  * @brief  ADC????????????

  * @note   ????????; ???4OPAMP; 5ADC???????

  *         ???????HAL_ADC_Init()???????

  * @retval void

  */

static void AdcSample_TypeStructInit(void)

{

    ADCSAP.GetAdcSampleCode  = __GetAdcSampleCode;

    ADCSAP.ConvAdcSampleCode = __ConvAdcSampleCode;



    /* ???????????????????ADC???????*/

    HAL_OPAMP_Start(&hopamp1); /* OPAMP1: Vin  ?????? */

    HAL_OPAMP_Start(&hopamp2); /* OPAMP2: Iin  ?????? */

    HAL_OPAMP_Start(&hopamp3); /* OPAMP3: Vout ?????? */

    HAL_OPAMP_Start(&hopamp4); /* OPAMP4: Iout ?????? */



    /* ADC?????????????*/

    HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

    HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);

    HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED);

    HAL_ADCEx_Calibration_Start(&hadc4, ADC_SINGLE_ENDED);

    HAL_ADCEx_Calibration_Start(&hadc5, ADC_SINGLE_ENDED);

}



/* ==============================================================

 *  KEY  

 * ============================================================== */

/**
  * @brief  ???????????
  * @note   HAL_GPIO_EXTI_Callback() ????
  *         ???????????:
  *           - KEY1: ?????????/??
  *           - KEY2: ???????
  *           - KEY3: ???????
  *           - ENCODE1_KEY: ????/????څ???
  * @param  GPIO_Pin  ???????GPIO????
  * @retval void
  */

/**
 * @brief  ?????????????????????????
 * @note  ????????? EXTI ????????????????
 */
CCMRAM void KEY_Process(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case ENCODE1_KEY_Pin:
            ui_vout_toggle_adjust();
            break;

        case KEY2_Pin:
            ui_vout_cursor_left();
            break;

        case KEY3_Pin:
            ui_vout_cursor_right();
            break;

        case KEY1_Pin:
            if (CTRL.HRTIMActive) {
                CTRL_Stop();
                printf("KEY1: output OFF\r\n");
            } else {
                CTRL_Start();
                printf("KEY1: output ON, auto mode by Vref/Vin (Vref=%.3fV Vin=%.3fV)\r\n",
                       CTRL.VoutRef, ADCSAP.Conv.VoltageIn);
            }
            break;

        case KEY4_Pin:
            printf("KEY4 pressed\r\n");
            break;

        default: break;
    }
}


/* ==============================================================

 *  ENCODER  

 * ============================================================== */

/**

  * @brief  ?????????????

  * @note   HAL_TIM_IC_CaptureCallback()???

  *         ???TIM1???????????

  *         ???????????????

  *         

  *         ???????:

  *           KEY1?????????????

  *           ???(FWD): Vref??0.1V????0V

  *           ???(REV): Vref????0.1V?????20V

  * @retval void

  */

/**
 * @brief  ???????????????????????????????
 * @note  ???????????????????????????????? 5ms
 */
CCMRAM void Encoder_Process(void)
{
    static uint32_t s_enc_tick = 0;
    uint32_t now = HAL_GetTick();
    /* ?????? 5ms????????? */
    if ((now - s_enc_tick) < 5U) return;
    s_enc_tick = now;

    BD.ECDR.Cnt = __HAL_TIM_GetCounter(&htim1);
    if (BD.ECDR.Cnt == 0) return;

    BD.ECDR.Dir = __HAL_TIM_DIRECTION_STATUS(&htim1);

    if (BD.ECDR.Dir == FWD) {
        BD.ECDR.FwdFlag = 1;
        BD.ECDR.FwdNum++;
        /* ????????????: ????????? +1 */
        ui_vout_digit_inc();
    } else {
        BD.ECDR.RevFlag = 1;
        BD.ECDR.RevNum++;
        /* ????????????: ????????? -1 */
        ui_vout_digit_dec();
    }

    __HAL_TIM_SET_COUNTER(&htim1, 0);
    lv_port_encoder_diff((BD.ECDR.Dir == FWD) ? 1 : -1);
}



/* ==============================================================

 *  LCD ???

 * ============================================================== */

/**

  * @brief  LCD???????????????????

  * @retval void

  */

void LCD_Display(void)

{

    printf("LCD_1IN69 Demo\r\n");

    DEV_Module_Init();

    LCD_1IN69_SetBackLight(1000);

    LCD_1IN69_Init(VERTICAL);

    LCD_1IN69_Clear(BLACK);

    Paint_NewImage(LCD_1IN69_WIDTH, LCD_1IN69_HEIGHT, 0, BLACK);

    Paint_SetClearFuntion(LCD_1IN69_Clear);

    Paint_SetDisplayFuntion(LCD_1IN69_DrawPoint);

    Paint_Clear(BLACK);

    DEV_Delay_ms(100);

    Paint_SetRotate(180);

    Paint_DrawString_EN(30, 10, "123", &Font24, YELLOW, RED);

    Paint_DrawString_EN(30, 34, "ABC", &Font24, BLUE,   CYAN);

    Paint_DrawFloatNum(30, 58, 987.654321, 3, &Font12, WHITE, BLACK);

    Paint_DrawRectangle(185, 10, 285, 58, RED,     DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    Paint_DrawLine(185, 10, 285, 58, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawLine(285, 10, 185, 58, MAGENTA, DOT_PIXEL_2X2, LINE_STYLE_SOLID);

    Paint_DrawCircle(120, 60, 25, BLUE,   DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    Paint_DrawCircle(150, 60, 25, BLACK,  DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    Paint_DrawCircle(190, 60, 25, RED,    DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    Paint_DrawCircle(145, 85, 25, YELLOW, DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    Paint_DrawCircle(165, 85, 25, GREEN,  DOT_PIXEL_2X2, DRAW_FILL_EMPTY);

    DEV_Delay_ms(3000);

    DEV_Module_Exit();

}



/**

  * @brief  LCD???????????BoardDevice_Init?????

  * @note   ????: DEV????? -> ???????? -> ?? -> ????? ->

  *                ???? -> ????????/????? -> ???

  * @retval void

  */

void __InitModule_LCD(void)

{

    printf("LCD_1IN69 Init...\r\n");

    DEV_Module_Init();

    LCD_1IN69_SetBackLight(0);             /* ??????????????? */

    LCD_1IN69_Init(VERTICAL);              /* ????????? */

    LCD_1IN69_Clear(BLACK);                /* ???????? */

    /* ????????????????????????????? */
    while (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_READY);
    while (HAL_SPI_GetState(&hspi1)        != HAL_SPI_STATE_READY);
    LCD_1IN69_SetBackLight(1000);          /* ????????: 0~1000 */

    printf("LCD_1IN69 Init OK\r\n");
}


/* RGB LED WS2812B PWM+DMA */

CCMRAM void __Show_RGB(void)

{

    HAL_TIM_PWM_Start_DMA(&RGBLED_PWM_hTIMER, RGBLED_PWM_Chaneel,

                         (uint32_t *)(&BD.RGBLED.RGBLED_Data),

                         sizeof(BD.RGBLED.RGBLED_Data));

}



void __SetPixelColor_RGB(uint16_t index, uint8_t r, uint8_t g, uint8_t b)

{

    uint8_t j;

    if (index >= BD.RGBLED.Pixel_size) return;

    for (j = 0; j < 8; j++) {

        BD.RGBLED.RGBLED_Data.ColorRealData[24*index+j]    = (g&(0x80>>j)) ? BIT_1:BIT_0;

        BD.RGBLED.RGBLED_Data.ColorRealData[24*index+j+8]  = (r&(0x80>>j)) ? BIT_1:BIT_0;

        BD.RGBLED.RGBLED_Data.ColorRealData[24*index+j+16] = (b&(0x80>>j)) ? BIT_1:BIT_0;

    }

}



void __GetPixelColor_RGB(uint16_t index, uint8_t *r, uint8_t *g, uint8_t *b)

{

    uint8_t j; *r=0; *g=0; *b=0;

    if (index >= BD.RGBLED.Pixel_size) return;

    for (j = 0; j < 8; j++) {

        if (BD.RGBLED.RGBLED_Data.ColorRealData[24*index+j]    >= BIT_1) *g |= (0x80>>j);

        if (BD.RGBLED.RGBLED_Data.ColorRealData[24*index+j+8]  >= BIT_1) *r |= (0x80>>j);

        if (BD.RGBLED.RGBLED_Data.ColorRealData[24*index+j+16] >= BIT_1) *b |= (0x80>>j);

    }

}



void __SetPixelColor_From_RGB_Buffer(uint16_t pixelIndex, uint8_t pRGB_Buffer[][3], uint16_t DataCount)

{

    uint16_t Index, j = 0;

    for (Index = pixelIndex; Index < BD.RGBLED.Pixel_size; Index++) {

        BD.RGBLED.SetPixelColor_RGB(Index, pRGB_Buffer[j][0], pRGB_Buffer[j][1], pRGB_Buffer[j][2]);

        if (++j >= DataCount) return;

    }

}



void __SetALLColor_RGB(uint8_t r, uint8_t g, uint8_t b)

{

    uint16_t Index;

    for (Index = 0; Index < BD.RGBLED.Pixel_size; Index++)

        BD.RGBLED.SetPixelColor_RGB(Index, r, g, b);

}



/* FAN ??????? */

/* @brief ????FAN PWM???? 0~1000???0~100%??OFF?????0 */

void __SetSpeed_FAN(uint16_t speed)

{

    if (BD.FAN.Status == OFF)   speed = 0;

    else if (speed > 1000)      speed = 1000;

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);

}



/* @brief ????MCU??????????: T<=20????T>20?Speed=400+20*(T-20) */

CCMRAM void __TempAdjSpeed_FAN(void)

{

    if (ADCSAP.Conv.TempMcu > 20.0f) {

        BD.FAN.Status = ON;

        BD.FAN.Speed = (uint16_t)(400.0f + 20.0f*(ADCSAP.Conv.TempMcu-20.0f));

        if (BD.FAN.Speed > 1000) BD.FAN.Speed = 1000;

    } else {

        BD.FAN.Status = OFF; BD.FAN.Speed = 0;

    }

    __SetSpeed_FAN(BD.FAN.Speed);

}



/* TMP112 ????????????*/

/* @brief ????TMP112????????????HAL_I2C_MemRxCpltCallback????*/

CCMRAM void __GetBoardTemperature(void) { TMP112_GetTemperature(BD.TMP.BTCode); }

/* @brief ????TMP112???????????????LSB=0.0625C */

CCMRAM void __CovBoardTemperature(void) { BD.TMP.BoardTemp = TMP112_ConvTemperature(BD.TMP.BTCode); }



/* SPI Flash W25Qxx */

/* @brief ???JEDEC ID????????: 0xEF4017(W25Q64)/0xEF4018(W25Q128) */

void __FlashSelfcheck(void) { BD.SPIFLASH.FlashID = W25QXX_ReadID(); }



/* BD_TypeStructInit: ?????????????????????????*/

static void BD_TypeStructInit(void)

{

    /* FAN ???? */

    BD.FAN.Status           = ON;            /* ??????????????????*/

    BD.FAN.Speed            = 0;             /* ???????0 */

    BD.FAN.SetSpeed_Fan     = __SetSpeed_FAN;

    BD.FAN.TempAdjSpeed_Fan = __TempAdjSpeed_FAN;



    /* TMP112 ????????????*/

    BD.TMP.GetBoardTemperature = __GetBoardTemperature;

    BD.TMP.CovBoardTemperature = __CovBoardTemperature;



    /* LCD ?????*/

    BD.LCD.InitModule_LCD    = __InitModule_LCD;

    BD.LCD.Screen            = START;

    BD.LCD.AdjustMode        = 0;             /* ??????????????? */



    /* SPI Flash */

    BD.SPIFLASH.FlashSelfcheck = __FlashSelfcheck;



    /* WS2812B RGB LED */

    BD.RGBLED.Pixel_size                    = PIXEL_SIZE;

    BD.RGBLED.SetPixelColor_RGB             = __SetPixelColor_RGB;

    BD.RGBLED.GetPixelColor_RGB             = __GetPixelColor_RGB;

    BD.RGBLED.SetPixelColor_From_RGB_Buffer = __SetPixelColor_From_RGB_Buffer;

    BD.RGBLED.SetALLColor_RGB               = __SetALLColor_RGB;

    BD.RGBLED.show                          = __Show_RGB;



    /* KEY / ENCODER: ??????????????????????*/

}

