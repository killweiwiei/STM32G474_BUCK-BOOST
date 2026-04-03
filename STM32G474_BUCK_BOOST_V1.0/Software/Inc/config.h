/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     config.c
* Version:  V1.0
* Date:     2024/07/29
* Author:   Robot
*================================================================*/

/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "hrtim.h"
#include "i2c.h"
//#include "iwdg.h"
#include "opamp.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"
	
#include "math.h"	
#include "control.h"
	
#include "DEV_Config.h"	
#include "LCD_1in69.h"	
#include "image.h"	
#include "GUI_Paint.h"
#include "spiflash.h"
#include "tmp11x.h"

/* Private defines ------------------------------------------------------------------*/
#define TS_CAL1 *((__IO uint16_t *)TEMPSENSOR_CAL1_ADDR) // 内部温度传感器在30度和VREF为3V时的校准数据
#define TS_CAL2 *((__IO uint16_t *)TEMPSENSOR_CAL2_ADDR) // 内部温度传感器在130度和VREF为3V时的校准数据

#define W25QXX_SPI				hspi3
#define TMP1112x_I2C			hi2c2

#define ADC_SampleNumber		4		// ADC采样次数
#define ADC_VREF				3.30f	// ADC Vref
#define ADC_VMULT				75.0f/4.7f
#define ADC_IMULT				0.1f/6.2f
#define ADC_SAMPRES				0.005f

#define BIT_1               	145           //1码比较值为145-->848us
#define BIT_0               	68            //0码比较值为68-->400us

#define PIXEL_SIZE			    4             	//灯的数量
#define RGBLED_PWM_hTIMER	  	htim5         	//定时器5
#define RGBLED_PWM_Chaneel  	TIM_CHANNEL_1	//通道1	



/* Prototypes ------------------------------------------------------------------*/

/* ADC采样变量结构体 */
typedef enum 
{
	NA = 0,// 未定义
	Buck,  // BUCK模式
	Boost, // BOOST模式
	Mix	   // MIX混合模式
} BuckBoost_ModeTypeDef;
	
/* ADC采样变量结构体 */
typedef struct 
{
	__IO uint32_t Vin[ADC_SampleNumber];	   		// 输入电电压
	__IO uint32_t VinAvg;  	// 输入电电压平均值
	__IO uint32_t Iin[ADC_SampleNumber];	   		// 输入电流
	__IO uint32_t IinAvg;  	// 输入电流平均值	
	__IO uint32_t Vout[ADC_SampleNumber];	   	// 输出电电压
	__IO uint32_t VoutAvg; 	// 输出电电压平均值
	__IO uint32_t Iout[ADC_SampleNumber];	   	// 输出电流
	__IO uint32_t IoutAvg; 	// 输出电流平均值
	__IO uint32_t MTemp[ADC_SampleNumber];	   	// MCU温度
	__IO uint32_t MTempAvg;  	// MCU温度平均值	
} ADC_SampleCode_DataTypeDef;	

/* ADC转换结构体 */
typedef struct 
{
	__IO float VoltageIn;	   	// 输入电压值
	__IO float CurrentIn;	   	// 输入电流值
	__IO float VoltageOut;	   	// 输出电压值
	__IO float CurrentOut;	   	// 输出电流值
	__IO float TempMcu;			// MCU温度值
} ADC_SampleConv_DataTypeDef;
	
/* ADC结构体 */
typedef struct 
{
	ADC_SampleCode_DataTypeDef Code;
	ADC_SampleConv_DataTypeDef Conv;
	
	void (*GetAdcSampleCode)(void);
	void (*ConvAdcSampleCode)( ADC_HandleTypeDef *hadc );
	
} ADC_SampleTypeDef;
/*********************************************************************/
/* 设备状态枚举体 */
typedef enum
{
	OFF = 0,
	ON	= 1,
} Device_Turn_StatusTypeDef;

/* TMP112结构体 */
typedef struct 
{
	__IO uint8_t BTCode[2];		// 温度code
	__IO float BoardTemp;		// Board温度值	
	
	void (*GetBoardTemperature)(void);	// 获取Board温度值
	void (*CovBoardTemperature)(void);	// 转换Board温度值
} TMP112_TypeDef;

/* BEEP结构体 */
typedef struct 
{
	Device_Turn_StatusTypeDef Status;//蜂鸣器状态
	__IO uint16_t Time;		//蜂鸣器鸣叫时间
	__IO uint16_t Number;		//蜂鸣器鸣叫次数
} BEEP_TypeDef;


/* FAN结构体 */
typedef struct 
{
	Device_Turn_StatusTypeDef Status;//风扇状态
	__IO uint16_t Speed;		//风扇速度
	
	void (*SetSpeed_Fan)(uint16_t speed);
	void (*TempAdjSpeed_Fan)(void);  
} FAN_TypeDef;



//整个RGBLED_DataTypeStruct结构体将被以PWM方式发送
typedef struct
{						
	uint8_t ColorStartData[3];           //3个0等待PWM稳定			
	uint8_t ColorRealData[24*PIXEL_SIZE];//实际GRB数据(已经转换为PWM对应的值)
	uint8_t ColorEndData;             	 //结束位为0
} RGBLED_DataTypeStruct;


/* RGBLED结构体 */
typedef struct 
{
	//实际发送的数据
	RGBLED_DataTypeStruct 	RGBLED_Data;
	
	__IO uint16_t Pixel_size; 	//灯数量
	__IO uint16_t Color;		//RGBLED颜色
	__IO uint16_t Brightness;	//RGBLED亮度
    //单独设置index的RGB颜色
    void (*SetPixelColor_RGB)(uint16_t index,uint8_t r,uint8_t g,uint8_t b);
    //从RGB数据读出：设置index的RGB颜色
    void (*SetPixelColor_From_RGB_Buffer)( uint16_t pixelIndex,uint8_t pRGB_Buffer[][3],uint16_t DataCount);
    //设置所有为RGB颜色
    void (*SetALLColor_RGB)(uint8_t r,uint8_t g,uint8_t b);
    //获取某个位置的RGB
    void (*GetPixelColor_RGB)(uint16_t index,uint8_t *r,uint8_t *g,uint8_t *b);
    //显示（发出数据）
    void (*show)(void);
} RGBLED_TypeDef;

/* LCD界面枚举体 */
typedef enum
{
	HOME = 0,
	START = 1,
	MENU = 2,
	SETTING	= 3,
} LCD_ScreenTypeDef;

/* LCD结构体 */
typedef struct 
{
	LCD_ScreenTypeDef Screen;	//LCD画面
	uint8_t AdjustMode;         //调节模式标志: 0=正常显示, 1=调节输出电压
	void (*InitModule_LCD)(void);
	void (*DisplayScreen_LCD)(void);	
} LCD_TypeDef;

/* KEY结构体 */
typedef struct 
{
	__IO Device_Turn_StatusTypeDef LeftFlag;	//按键左标志
	__IO Device_Turn_StatusTypeDef RightFlag;	//按键右标志
	__IO Device_Turn_StatusTypeDef UpFlag;		//按键上标志
	__IO Device_Turn_StatusTypeDef DownFlag;	//按键下标志
} KEY_TypeDef;

/* ENCODER旋转状态枚举体 */
typedef enum
{
	FWD = 0,	//正转
	REV	= 1		//反转
} ECDR_Direction_StatusTypeDef;

/* ENCODER结构体 */
typedef struct 
{
	__IO ECDR_Direction_StatusTypeDef Dir;		//方向
	__IO Device_Turn_StatusTypeDef KeyShot;		//按键短按
	__IO Device_Turn_StatusTypeDef KeyLong;		//按键长按
	__IO uint16_t KeyHoldonTime;				//按键持续时间
	__IO uint16_t Cnt;							//旋转脉冲数
	__IO uint8_t FwdFlag;
	__IO uint8_t RevFlag;
	__IO uint8_t FwdNum;
	__IO uint8_t RevNum;				
} ENCODER_TypeDef;

/* FLASH结构体 */
typedef struct 
{
	__IO uint16_t FlashID;		// FlashID
	__IO uint32_t BoardUUID;	 // BoardUUID
//	__IO float BoardTemp;		// Board温度值	
	
	void (*FlashSelfcheck)(void);	// Flash 自检
//	void (*ConvBoardTemperature)(void);// 转换Board温度值
} SPIFLASH_TypeDef;


/* 板设结构体 */
typedef struct 
{
	TMP112_TypeDef TMP;	// 温度传感器
	BEEP_TypeDef BEEP;		// 蜂鸣器
	FAN_TypeDef FAN;		// 风扇
	RGBLED_TypeDef RGBLED;	// RGBLED	
	LCD_TypeDef LCD;		// LCD
	KEY_TypeDef KEY;		// KEY
	ENCODER_TypeDef ECDR;	// ENCODER
	SPIFLASH_TypeDef SPIFLASH; // SPIFLASH
} BD_TypeDef;		


extern ADC_SampleTypeDef ADCSAP;
extern BD_TypeDef BD;

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_H__ */

