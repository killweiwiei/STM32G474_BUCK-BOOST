/**
 * @file    lv_port_indev.c
 * @brief   LVGL9 编码器输入设备移植
 * @note    输入来源：TIM1 正交编码器 + KEY1 按键
 *          旋转：用于焦点导航（+1 右旋，-1 左旋）
 *          按下：用于确认/点击
 */

#include "lv_port_indev.h"

/* 内部状态变量 */
static volatile int8_t  enc_diff  = 0;  /* 累计旋转步数 */
static volatile uint8_t enc_press = 0;  /* 1=按键按下   */

static lv_indev_t *indev_encoder;
static lv_group_t *enc_group;

/**
 * @brief  LVGL 编码器读回调
 */
static void encoder_read(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;

    data->enc_diff = enc_diff;
    enc_diff = 0;

    if (enc_press)
    {
        data->state = LV_INDEV_STATE_PRESSED;
        enc_press   = 0;
    }
    else
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/**
 * @brief  输入端口初始化
 * @note   在 lv_init() 后调用一次
 */
void lv_port_indev_init(void)
{
    indev_encoder = lv_indev_create();
    lv_indev_set_type(indev_encoder, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev_encoder, encoder_read);

    enc_group = lv_group_create();
    lv_group_set_default(enc_group);
    lv_indev_set_group(indev_encoder, enc_group);
}

/**
 * @brief  上报编码器按键事件
 * @note   在 KEY1 外部中断中调用
 */
void lv_port_encoder_key(void)
{
    enc_press = 1;
}

/**
 * @brief  上报编码器旋转步进
 * @param  diff  +1=右旋，-1=左旋
 * @note   在 Encoder_Process() 中调用
 */
void lv_port_encoder_diff(int8_t diff)
{
    enc_diff += diff;
}
