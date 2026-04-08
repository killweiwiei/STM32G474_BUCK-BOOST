/**
 * @file    callback.c
 * @brief   HAL \xd6\xd0\xb6\xcf\xbb\xd8\xb5\xf7\xba\xaf\xca\xfd\xbc\xaf\xd6\xd0\xb9\xdc\xc0\xed
 * @note    \xd6\xd0\xb6\xcf\xc0\xef\xd6\xbb\xc9\xe8\xd1\xf9\xd7\xe9\xd6\xb8\xa3\xac\xd6\xf7\xd1\xad\xbb\xb7\xd6\xd0\xb4\xa6\xc0\xed LVGL \xba\xaf\xca\xfd\xa3\xa8\xb7\xc0\xd6\xc6\x\xd2\xbb\xa3\xa9
 * @version V1.3
 * @date    2024/07/29
 * @author  Robot
 */

#include "callback.h"
#include "function.h"
#include "control.h"
#include "ui_main.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "ui_main.h"  /* g_fan_auto, g_fan_speed */

/* ----------------------------------------------------------------
 * \xc9\xe8\xd1\xf9\xd7\xe9\xca\xf3\xbe\xdd (\xd6\xd0\xb6\xcf\c9\xe8\xd6\xc3\xa3\xac\xd6\xf7\xd1\xad\xbb\xb7\xb6\xc1\xc8\xa1)
 * \xca\xb9\xd3\xc3 volatile \xc8\xb7\xb1\xa3\xd6\xd0\xb6\xcf\xd3\xeb\xd6\xf7\xd1\xad\xbb\xb7\xbd\xe2\xc2\xcb\xb2\xbb\xbf\xc9\xd3\xc5\xbb\xaf
 * ---------------------------------------------------------------- */
volatile uint8_t  g_enc_key_flag       = 0;  /* short press */
volatile uint8_t  g_enc_key_long_flag  = 0;  /* long press 2s */
volatile uint8_t  g_key2_flag          = 0;
volatile uint8_t  g_key3_flag          = 0;
volatile uint8_t  g_key1_flag          = 0;
volatile int8_t   g_enc_diff           = 0;
volatile uint32_t g_enc_key_press_tick = 0;
volatile uint8_t  g_enc_key_held       = 0;

/* ----------------------------------------------------------------
 * \xb6\xa8\xca\xb1\xc6\xf7\xd6\xdc\xc6\xda\xd6\xd0\xb6\xcf
 * ---------------------------------------------------------------- */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim6) {
        ADCSAP.GetAdcSampleCode();
        BD.TMP.GetBoardTemperature();
    } else if (htim == &htim7) {
        CTRL_Run();
    } else if (htim == &htim16) {
        printf("Vin=%.2f Vout=%.2f Ref=%.2f Mode=%d TE_CMP=%lu TF_CMP=%lu Err=%.2f Intg=%.1f Out=%.1f\r\n",
               ADCSAP.Conv.VoltageIn, ADCSAP.Conv.VoltageOut,
               CTRL.PIDVout.Ref, CTRL.Mode, CTRL.DutyTE, CTRL.DutyTF,
               CTRL.PIDVout.Err, CTRL.PIDVout.Integral, CTRL.PIDVout.Out);
    } else if (htim == &htim17) {
        /* g_fan_auto:1=AUTO(temp-based), 0=Manual */
        if (g_fan_auto) {
            BD.FAN.TempAdjSpeed_Fan();
        } else {
            BD.FAN.Status = (g_fan_speed > 0) ? ON : OFF;
            BD.FAN.Speed  = (uint16_t)(g_fan_speed * 10U);
            BD.FAN.SetSpeed_Fan(BD.FAN.Speed);
        }
    }
}

/* ----------------------------------------------------------------
 * \xb1\xe0\xc2\xeb\xc6\xf7\xca\xe4\xc8\xeb\xc6\xc6\xc6\xc6\xbb\xd8\xb5\xf7 (TIM1 \xb6\xc1\xc8\xa1\xd5\xfd/\xb7\xb4\xd7\xaa\xd6\xb8\xd1\xf9, \xd6\xb7\xc9\xe8 diff \xd1\xf9\xd7\xe9)
 * ---------------------------------------------------------------- */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    if (htim == &htim1) {
        uint32_t cnt = __HAL_TIM_GetCounter(&htim1);
        if (cnt == 0) return;
        uint8_t dir = __HAL_TIM_DIRECTION_STATUS(&htim1);
        /* REV(\xb7\xb4\xd7\xaa)=+1, FWD(\xd5\xfd\xd7\xaa)=-1 (\xd3\xc3\xbb\xa7\xd7\xaa\xd7\xe8\xd3\xd0\xd0\xa7\xb7\xba\xb6\xae) */
        g_enc_diff += (dir == FWD) ? -1 : 1;
        __HAL_TIM_SET_COUNTER(&htim1, 0);
    }
}

/* ----------------------------------------------------------------
 * ADC DMA \xd7\xaa\xd2\xbb\xcd\xea\xd5\xfb\xbb\xd8\xb5\xf7
 * ---------------------------------------------------------------- */
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    ADCSAP.ConvAdcSampleCode(hadc);
}

/* ----------------------------------------------------------------
 * GPIO EXTI \xb0\xb4\xbc\xfc\xd6\xd0\xb6\xcf\xbb\xd8\xb5\xf7 (\xd6\xbb\xc9\xe8\xd1\xf9\xd7\xe9, \xb2\xbb\xd5\xfe\xd4\xda\xd6\xd0\xb6\xcf\xd0\xf2\xd7\xf7 LVGL)
 * ---------------------------------------------------------------- */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    static uint32_t s_key1_tick = 0;
    static uint32_t s_key2_tick = 0;
    static uint32_t s_key3_tick = 0;
    static uint32_t s_enc_key_tick = 0;
    uint32_t now = HAL_GetTick();

    if (GPIO_Pin == ENCODE1_KEY_Pin) {
        if ((now - s_enc_key_tick) < 80U) return;
        s_enc_key_tick = now;
        g_enc_key_press_tick = now;
        g_enc_key_held = 1;
    }
    else if (GPIO_Pin == KEY2_Pin) {
        if ((now - s_key2_tick) < 80U) return;
        s_key2_tick = now;
        g_key2_flag = 1;
    }
    else if (GPIO_Pin == KEY3_Pin) {
        if ((now - s_key3_tick) < 80U) return;
        s_key3_tick = now;
        g_key3_flag = 1;
    }
    else if (GPIO_Pin == KEY1_Pin) {
        if ((now - s_key1_tick) < 120U) return;
        s_key1_tick = now;
        g_key1_flag = 1;
    }
}

/* ----------------------------------------------------------------
 * I2C \xc4\xda\xb4\xe6\xb6\xc1\xc8\xa1\xcd\xea\xd5\xfb\xbb\xd8\xb5\xf7
 * ---------------------------------------------------------------- */
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
    if (hi2c == &hi2c2)
        BD.TMP.CovBoardTemperature();
}

/* ----------------------------------------------------------------
 * SPI DMA \xb7\xa2\xcb\xcd\xcd\xea\xd5\xfb\xbb\xd8\xb5\xf7
 * ---------------------------------------------------------------- */
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
        lv_port_disp_flush_done();
}
