/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     tmp11x.c
* Version:  V1.0
* Date:     2024/08/29
* Author:   Robot
*================================================================*/

/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "tmp11x.h"

void TMP112_GetTemperature(__IO uint8_t *Tcode )
{
	
//	HAL_I2C_Mem_Read(&TMP1112x_I2C, TMP112_DEVADDR, TMP112_TMP_REG, I2C_MEMADD_SIZE_8BIT, (uint8_t *)Tcode, 2, 100);	
//	HAL_I2C_Mem_Read_DMA(&TMP1112x_I2C, TMP112_DEVADDR, TMP112_TMP_REG, I2C_MEMADD_SIZE_8BIT, (uint8_t *)Tcode, 2);	
	HAL_I2C_Mem_Read_DMA(&TMP1112x_I2C, TMP112_DEVADDR, TMP112_TMP_REG, I2C_MEMADD_SIZE_8BIT, (uint8_t *)Tcode, 2);	

}

float TMP112_ConvTemperature( const __IO uint8_t *Tcode )
{
	__IO uint16_t Tdata = (uint16_t)(((Tcode[0]<<8) + Tcode[1])>>4);	// 组合为12位
	__IO float FinalTemp;
	
	if(Tdata & 0x800)	
		FinalTemp = (float)(-1.0f) * (((~Tdata)&0x0FFF + 1) * ( 0.0625f ));	//转换为负温度值
	else
		FinalTemp = (float)(Tdata) * ( 0.0625f ); //转换为正温度值
	
	return FinalTemp;
}
