/**
 * @file    lv_port_disp.h
 * @brief   LVGL9 显示端口头文件（LCD_1IN69，240x280，RGB565，SPI1 单线 DMA）
 */

#ifndef LV_PORT_DISP_H
#define LV_PORT_DISP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/**
 * @brief  显示端口初始化，在 lv_init() 和 LCD_1IN69_Init() 之后调用
 */
void lv_port_disp_init(void);

/**
 * @brief  SPI1 DMA TX 完成回调，在 HAL_SPI_TxCpltCallback 中调用
 *         通知 LVGL 当前 flush 已完成
 */
void lv_port_disp_flush_done(void);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_DISP_H */
