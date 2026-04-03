/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     spiflash.c
* Version:  V1.0
* Date:     2024/08/29
* Author:   Robot
*================================================================*/

/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SPIFLASH_H__
#define __SPIFLASH_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "config.h"
	
/* Define ------------------------------------------------------------------*/	
#define SPIFLASH_Delay1us(x)	HAL_Delay(x)
#define SPIFLASH_CS(x) 			HAL_GPIO_WritePin( FLASH_NSS_GPIO_Port, FLASH_NSS_Pin, x == 0? GPIO_PIN_RESET:GPIO_PIN_SET)

#define W25Q08 	0XEF13 	
#define W25Q16 	0XEF14
#define W25Q32 	0XEF15
#define W25Q64 	0XEF16
#define W25Q128	0XEF17
	
#define W25X_WriteEnable		0x06 
#define W25X_WriteDisable		0x04 
#define W25X_ReadStatusReg		0x05 
#define W25X_WriteStatusReg		0x01 
#define W25X_ReadData			0x03 
#define W25X_FastReadData		0x0B 
#define W25X_FastReadDual		0x3B 
#define W25X_PageProgram		0x02 
#define W25X_BlockErase			0xD8 
#define W25X_SectorErase		0x20 
#define W25X_ChipErase			0xC7 
#define W25X_PowerDown			0xB9 
#define W25X_ReleasePowerDown	0xAB 
#define W25X_DeviceID			0xAB 
#define W25X_ManufactDeviceID	0x90 
#define W25X_JedecDeviceID		0x9F 
	
/* Exported functions prototypes ---------------------------------------------*/

void W25QXX_Init(void);
uint16_t W25QXX_ReadID(void);
void W25QXX_WriteSR(uint8_t sr);  
uint8_t W25QXX_ReadSR(void); 
void W25QXX_WaitBusy(void); 
void W25QXX_Power_Down(void);  
void W25QXX_Power_Up(void);  
void W25QXX_Write_Enable(void);   
void W25QXX_Write_Disable(void);   

void W25QXX_Erase_Sector(uint32_t Dst_Addr);
void W25QXX_Erase_Block( uint32_t Dst_Addr );
uint32_t W25QXX_Read32bit(uint32_t ReadAddr);
uint8_t W25QXX_ReadByte(uint32_t ReadAddr);   
uint8_t W25QXX_WriteByte(uint8_t data,uint32_t WriteAddr);


#ifdef __cplusplus
}
#endif

#endif /* __SPIFLASH_H__ */

