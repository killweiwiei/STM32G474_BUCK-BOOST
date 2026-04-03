/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
	
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */
int fputc(int ch,FILE *f);
/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define FLASH_NSS_Pin GPIO_PIN_13
#define FLASH_NSS_GPIO_Port GPIOC
#define ENCODE1_A_Pin GPIO_PIN_0
#define ENCODE1_A_GPIO_Port GPIOC
#define ENCODE1_B_Pin GPIO_PIN_1
#define ENCODE1_B_GPIO_Port GPIOC
#define ENCODE1_KEY_Pin GPIO_PIN_2
#define ENCODE1_KEY_GPIO_Port GPIOC
#define ENCODE1_KEY_EXTI_IRQn EXTI2_IRQn
#define KEY2_Pin GPIO_PIN_3
#define KEY2_GPIO_Port GPIOC
#define KEY2_EXTI_IRQn EXTI3_IRQn
#define RGB_LED_Pin GPIO_PIN_0
#define RGB_LED_GPIO_Port GPIOA
#define OP1_VIP_Pin GPIO_PIN_1
#define OP1_VIP_GPIO_Port GPIOA
#define OP1_VOUT_Pin GPIO_PIN_2
#define OP1_VOUT_GPIO_Port GPIOA
#define BEEP_Pin GPIO_PIN_4
#define BEEP_GPIO_Port GPIOA
#define OP2_VIM_Pin GPIO_PIN_5
#define OP2_VIM_GPIO_Port GPIOA
#define OP2_VOUT_Pin GPIO_PIN_6
#define OP2_VOUT_GPIO_Port GPIOA
#define OP2_VIP_Pin GPIO_PIN_7
#define OP2_VIP_GPIO_Port GPIOA
#define OP3_VIP_Pin GPIO_PIN_0
#define OP3_VIP_GPIO_Port GPIOB
#define OP3_VOUT_Pin GPIO_PIN_1
#define OP3_VOUT_GPIO_Port GPIOB
#define OP3_VIM_Pin GPIO_PIN_2
#define OP3_VIM_GPIO_Port GPIOB
#define OP4_VIM_Pin GPIO_PIN_10
#define OP4_VIM_GPIO_Port GPIOB
#define WIFI_RX_Pin GPIO_PIN_11
#define WIFI_RX_GPIO_Port GPIOB
#define OP4_VOUT_Pin GPIO_PIN_12
#define OP4_VOUT_GPIO_Port GPIOB
#define OP4_VIP_Pin GPIO_PIN_13
#define OP4_VIP_GPIO_Port GPIOB
#define LCD_CS_Pin GPIO_PIN_14
#define LCD_CS_GPIO_Port GPIOB
#define LCD_RS_Pin GPIO_PIN_15
#define LCD_RS_GPIO_Port GPIOB
#define PWM2_L_Pin GPIO_PIN_6
#define PWM2_L_GPIO_Port GPIOC
#define PWM2_H_Pin GPIO_PIN_7
#define PWM2_H_GPIO_Port GPIOC
#define PWM1_L_Pin GPIO_PIN_8
#define PWM1_L_GPIO_Port GPIOC
#define PWM1_H_Pin GPIO_PIN_9
#define PWM1_H_GPIO_Port GPIOC
#define TEMP_SDA_Pin GPIO_PIN_8
#define TEMP_SDA_GPIO_Port GPIOA
#define TEMP_SCL_Pin GPIO_PIN_9
#define TEMP_SCL_GPIO_Port GPIOA
#define KEY1_Pin GPIO_PIN_10
#define KEY1_GPIO_Port GPIOA
#define KEY1_EXTI_IRQn EXTI15_10_IRQn
#define FAN_Pin GPIO_PIN_15
#define FAN_GPIO_Port GPIOA
#define FLASH_SCK_Pin GPIO_PIN_10
#define FLASH_SCK_GPIO_Port GPIOC
#define FLASH_MISO_Pin GPIO_PIN_11
#define FLASH_MISO_GPIO_Port GPIOC
#define FLASH_MOSI_Pin GPIO_PIN_12
#define FLASH_MOSI_GPIO_Port GPIOC
#define LCD_RST_Pin GPIO_PIN_2
#define LCD_RST_GPIO_Port GPIOD
#define LCD_SCK_Pin GPIO_PIN_3
#define LCD_SCK_GPIO_Port GPIOB
#define LCD_BLK_Pin GPIO_PIN_4
#define LCD_BLK_GPIO_Port GPIOB
#define LCD_SDA_Pin GPIO_PIN_5
#define LCD_SDA_GPIO_Port GPIOB
#define KEY3_Pin GPIO_PIN_6
#define KEY3_GPIO_Port GPIOB
#define KEY3_EXTI_IRQn EXTI9_5_IRQn
#define KEY4_Pin GPIO_PIN_7
#define KEY4_GPIO_Port GPIOB
#define KEY4_EXTI_IRQn EXTI9_5_IRQn
#define WIFI_TX_Pin GPIO_PIN_9
#define WIFI_TX_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define CCMRAM __attribute__((section("ccmram")))
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
