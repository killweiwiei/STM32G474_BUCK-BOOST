/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     math.c
* Version:  V1.1
* Date:     2024/07/29
* Author:   Robot
* Brief:    数学算法库
*           包含: 中位值平均滤波、HRTIM占空比计算(预留)
*================================================================*/

#include "math.h"

/**
  * @brief  中位值平均滤波
  * @note   算法步骤:
  *           1. 将输入样本拷贝到本地缓冲区(保护原始数据)
  *           2. 冒泡排序从小到大
  *           3. 去掉最大值(index num-1)和最小值(index 0)
  *           4. 对剩余(num-2)个数据取算术平均值
  *
  *         适用场景: ADC采样噪声抑制，对偶发尖峰干扰有较好抑制效果。
  *
  *         性能说明: 冒泡排序时间复杂度O(n^2)，ADC_SampleNumber=10时
  *         在100ms中断中调用，CPU占用可忽略。
  *         若num增大(>50)，建议改用插入排序或快速选择算法。
  *
  *         注意: num必须>=3，否则去掉首尾后无数据可平均。
  * @param  value: 指向样本数组的常量指针
  * @param  num:   样本数量(建议>=3)
  * @retval 滤波后的平均值
  */
uint32_t MiddleAverageFilter(const uint32_t *value, uint16_t num)
{
    uint16_t i, j, k;
    uint32_t temp, sum = 0;
    uint32_t value_buf[num]; /* VLA，num较小时栈上分配安全 */

    /* 拷贝原始数据，避免修改调用方缓冲区 */
    for (i = 0; i < num; i++)
        value_buf[i] = value[i];

    /* 冒泡排序(从小到大) */
    for (j = 0; j < num - 1; j++)
    {
        for (k = 0; k < num - j - 1; k++)
        {
            if (value_buf[k] > value_buf[k + 1])
            {
                temp             = value_buf[k];
                value_buf[k]     = value_buf[k + 1];
                value_buf[k + 1] = temp;
            }
        }
    }

    /* 去掉最小值和最大值，对中间值求和 */
    for (i = 1; i < num - 1; i++)
        sum += value_buf[i];

    return sum / (uint32_t)(num - 2); /* 返回算术平均值 */
}

/*
 * HRTIM四开关BUCK-BOOST占空比和相位计算(预留)
 *
 * 四开关BUCK-BOOST拓扑:
 *   BUCK  模式: Q1/Q2斩波，Q3关断，Q4常通
 *   BOOST 模式: Q3/Q4斩波，Q1常通，Q2关断
 *   BB    模式: Q1/Q2/Q3/Q4均参与调制
 *
 * HRTIM参数计算:
 *   period = HRTIM_CLK / fsw = (170MHz*32) / fsw
 *   cmp1   = period * duty_pct / 100  (上升沿比较)
 *   相移   = cmp1 + period * angle_deg / 360
 *
 * 此函数待PID控制器接入后启用。
 */
/* void HRTIM_CalcDutyPhase(tim_struct *tm, pwm_para *pwm) { ... } */
