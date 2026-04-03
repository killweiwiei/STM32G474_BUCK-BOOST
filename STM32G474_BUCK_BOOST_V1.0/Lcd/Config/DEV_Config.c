/*****************************************************************************
* | File      	:   DEV_Config.c
* | Author      :   Waveshare team / Modified for STM32G474 SPI1 1LINE DMA
* | Function    :   Hardware underlying interface
* | Info        :
*   SPI1 is configured as 1-LINE (half-duplex) master.
*   - DEV_SPI_WRite: BLOCKING single-byte transmit (HAL_SPI_Transmit)
*     MUST be blocking because _dat is a stack variable; DMA would
*     read garbage after the function returns.
*   - DEV_SPI_WRiteBuffer: DMA multi-byte transmit, caller must ensure
*     the buffer stays valid until DMA completes (static/global buf only).
******************************************************************************/
#include "DEV_Config.h"

/* Delay wrapper */
void DEV_delay_ms(uint16_t xms)
{
    HAL_Delay(xms);
}

/**
 * @brief  Single-byte SPI write (BLOCKING)
 * @note   _dat is a stack variable -- MUST use blocking transmit.
 *         HAL_SPI_Transmit_DMA would read freed stack memory.
 *         SPI1 is 1-LINE half-duplex: HAL_SPI_Transmit works correctly.
 */
void DEV_SPI_WRite(UBYTE _dat)
{
    /* Wait for any ongoing DMA transfer to finish first */
    while (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_READY);
    while (HAL_SPI_GetState(&hspi1)        != HAL_SPI_STATE_READY);
    /* Blocking 1-byte transmit -- safe with stack variable */
    HAL_SPI_Transmit(&hspi1, (uint8_t *)&_dat, 1, 100);
}

/**
 * @brief  Multi-byte SPI write via DMA (NON-BLOCKING)
 * @note   Caller must pass a static or global buffer.
 *         Waits for previous DMA to finish before starting a new one.
 */
void DEV_SPI_WRiteBuffer(UBYTE *_dat, UWORD _size)
{
    while (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_READY);
    while (HAL_SPI_GetState(&hspi1)        != HAL_SPI_STATE_READY);
    HAL_SPI_Transmit_DMA(&hspi1, _dat, _size);
}

/**
 * @brief  Module hardware init
 * @note   Set DC=1, CS=1, RST=1 idle states; start TIM3 backlight PWM
 */
int DEV_Module_Init(void)
{
    DEV_Digital_Write(DEV_DC_PIN,  1);
    DEV_Digital_Write(DEV_CS_PIN,  1);
	DEV_Digital_Write(DEV_RST_PIN, 0);
	DEV_Delay_ms(20);
    DEV_Digital_Write(DEV_RST_PIN, 1);
	DEV_Delay_ms(20);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    return 0;
}

void DEV_Module_Exit(void)
{
    DEV_Digital_Write(DEV_DC_PIN,  0);
    DEV_Digital_Write(DEV_CS_PIN,  0);
    DEV_Digital_Write(DEV_RST_PIN, 0);
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
}
