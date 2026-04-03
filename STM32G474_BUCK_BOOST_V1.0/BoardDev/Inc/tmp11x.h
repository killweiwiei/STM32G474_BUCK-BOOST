/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     tmp11x.c
* Version:  V1.0
* Date:     2024/08/29
* Author:   Robot
*================================================================*/

/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __TMP11x_H__
#define __TMP11x_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "config.h"
	
/* Define ------------------------------------------------------------------*/	
#define TMP112_DEVADDR			0x90			// TMP112Éè±¸µØÖ·
#define TMP112_TMP_REG			0x00			// TMP112ÎÂ¶È¼Ä´æÆ÷
#define TMP112_CFG_REG			0x01			// TMP112ÅäÖÃ¼Ä´æÆ÷
#define TMP112_TLOW_REG			0x02			// TMP112TLOW¼Ä´æÆ÷
#define TMP112_THIGH_REG		0x03			// TMP112THIGH¼Ä´æÆ÷
	

void TMP112_GetTemperature(__IO uint8_t *Tcode );
float TMP112_ConvTemperature( const __IO uint8_t *Tcode );
	
#ifdef __cplusplus
}
#endif

#endif /* __TMP1112x_H__ */
