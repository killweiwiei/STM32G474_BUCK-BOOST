#ifndef LV_PORT_INDEV_H
#define LV_PORT_INDEV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"

/* LVGL 输入端口初始化（编码器） */
void lv_port_indev_init(void);

/* 在 KEY1 外部中断中调用：上报按键事件 */
void lv_port_encoder_key(void);

/* 在 Encoder_Process 中调用：上报旋转步进（+1/-1） */
void lv_port_encoder_diff(int8_t diff);

#ifdef __cplusplus
}
#endif

#endif /* LV_PORT_INDEV_H */
