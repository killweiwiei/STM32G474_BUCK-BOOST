/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     callback.h
* Version:  V1.1
* Date:     2024/07/29
* Author:   Robot
* Brief:    HAL interrupt callback declarations.
*================================================================*/

#ifndef __CALLBACK_H__
#define __CALLBACK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "tim.h"
#include "adc.h"
#include "i2c.h"
#include "spi.h"

/* HAL callback prototypes (implemented in callback.c) */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c);
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);

/* ----------------------------------------------------------------
 * Input event flags (set in ISR, cleared in main loop)
 * ---------------------------------------------------------------- */
extern volatile uint8_t  g_enc_key_flag;       /* ENCODE1_KEY short press   */
extern volatile uint8_t  g_enc_key_long_flag;  /* ENCODE1_KEY long press 2s */
extern volatile uint8_t  g_key2_flag;          /* KEY2 pressed              */
extern volatile uint8_t  g_key3_flag;          /* KEY3 pressed              */
extern volatile uint8_t  g_key1_flag;          /* KEY1 pressed (spare)      */
extern volatile int8_t   g_enc_diff;           /* encoder delta +1/-1       */
extern volatile uint32_t g_enc_key_press_tick; /* HAL_GetTick at press down */
extern volatile uint8_t  g_enc_key_held;       /* 1=currently held down     */

#ifdef __cplusplus
}
#endif

#endif /* __CALLBACK_H__ */
