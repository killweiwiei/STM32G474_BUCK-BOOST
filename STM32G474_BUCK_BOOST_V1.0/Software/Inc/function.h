/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     function.c
* Version:  V1.0
* Date:     2024/07/25
* Author:   Robot
*================================================================*/

/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __FUNCTION_H__
#define __FUNCTION_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "config.h"
#include "lv_port_indev.h"  /* LVGL encoder port - used in Encoder_Process */
	
/* Exported functions prototypes ---------------------------------------------*/

void LCD_HW_Init(void);
void BoardDevice_Init(void);
CCMRAM void ADC_SampleCodeGet(void);
CCMRAM void ADC_SampleConversion(ADC_HandleTypeDef *hadc);
CCMRAM void KEY_Process(uint16_t GPIO_Pin);
CCMRAM void FAN_Process(void);
CCMRAM void Encoder_Process(void);	
	
void LCD_Display(void);	
	
#ifdef __cplusplus
}
#endif

#endif /* __FUNCTION_H__ */

