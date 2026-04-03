/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : STM32G474 BUCK-BOOST Digital Power Supply Main Program
  * @board          : STM32G474 BUCK-BOOST V1.0
  * @author         : Robot
  * @version        : V1.1
  * @date           : 2024/07/29
  ******************************************************************************
  * Hardware Resources:
  *   HRTIM1 TimerE/F  BUCK-BOOST carrier PWM 200kHz
  *   ADC1 IN3  PA2  + OPAMP1  -> Vin  input voltage sampling
  *   ADC2 IN3  PA6  + OPAMP2  -> Iin  input current sampling
  *   ADC3 IN1  PB1  + OPAMP3  -> Vout output voltage sampling
  *   ADC4 IN3  PB12 + OPAMP4  -> Iout output current sampling
  *   ADC5 internal             -> MCU junction temperature
  *   TIM6  100ms IRQ  ADC DMA trigger + TMP112 temperature read
  *   TIM7   1ms IRQ  PID control loop (CTRL_Run) - highest priority
  *   TIM16  50ms IRQ  UART debug data print
  *   TIM17 200ms IRQ  FAN speed adjust + WS2812B RGB refresh
  *   TIM1  encoder    rotary encoder pulse count (input device)
  *   TIM2  CH1 PWM    FAN PWM drive output
  *   TIM3  CH1 PWM    LCD backlight brightness
  *   TIM5  CH1 PWM    WS2812B RGB LED data signal
  *   I2C2  DMA        TMP112 board temperature sensor
  *   SPI1  1LINE DMA  LCD 1.69inch display, 42.5MHz
  *   SPI3  DMA        W25Qxx SPI NOR Flash
  *   USART1           debug UART, printf redirect, 115200bps
  *   USART3           reserved UART port
  *
  * Task Scheduling Architecture:
  *   IRQ layer (time-critical):
  *     TIM7 10ms -> PID control (highest priority, must not be delayed)
  *     TIM6 100ms-> ADC DMA + TMP112 I2C DMA (non-blocking)
  *     DMA  cplt -> ADC conversion + SPI LCD flush notify LVGL
  *   Main loop (background):
  *     lv_timer_handler() -> LVGL render, animation, widget timers
  *     ui_main_update()   -> refresh Vin/Vout/Iin/Iout every 200ms
  *
  * Copyright (c) 2024 STMicroelectronics. All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"

#include "adc.h"

#include "dma.h"

#include "hrtim.h"

#include "i2c.h"

#include "opamp.h"

#include "spi.h"

#include "tim.h"

#include "usart.h"

#include "gpio.h"



/* Private includes ----------------------------------------------------------*/

/* USER CODE BEGIN Includes */

/* LCD_1in69.h included via function.h */
#include "function.h"
#include "callback.h"        /* board device init and hardware abstraction */

#include "config.h"          /* global config, data structs, extern declarations */

#include "spiflash.h"        /* W25Qxx SPI NOR Flash driver */

#include "lvgl.h"            /* LVGL 9.x graphics library */

#include "lv_port_disp.h"    /* LVGL display port: SPI1 DMA flush */

#include "lv_port_indev.h"   /* LVGL input device port: rotary encoder */

#include "ui_main.h"         /* LVGL dashboard UI create and update */

/* USER CODE END Includes */



/* Private typedef -----------------------------------------------------------*/

/* USER CODE BEGIN PTD */

/* See config.h: BuckBoost_ModeTypeDef / ADC_SampleTypeDef / BD_TypeDef */

/* USER CODE END PTD */



/* Private define ------------------------------------------------------------*/

/* USER CODE BEGIN PD */

/* See config.h: ADC_SampleNumber / ADC_VREF / ADC_VMULT / ADC_IMULT */

/* USER CODE END PD */



/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PM */

/**

  * @brief  Redirect printf to USART1 (blocking). Replace with DMA for production.

  */

int fputc(int ch, FILE *f)

{

    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);

    return ch;

}

/* USER CODE END PM */



/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* Globals defined in function.c: ADC_SampleTypeDef ADCSAP; BD_TypeDef BD; */

/* USER CODE END PV */



/* Private function prototypes -----------------------------------------------*/

void SystemClock_Config(void);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */



/* Private user code ---------------------------------------------------------*/

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */



/**

  * @brief  Application entry point (main function)

  * @note   Init sequence:

  *           1. HAL_Init()           -- SysTick/Flash/NVIC

  *           2. SystemClock_Config() -- HSE 12MHz -> PLL -> SYSCLK 170MHz

  *           3. MX_xxx_Init()        -- HAL peripheral init (DMA first)

  *           4. BoardDevice_Init()   -- ADC/LCD/Flash/PID board init

  *           5. LVGL init           -- display + input device + dashboard UI

  *           6. while(1)             -- LVGL handler + ui update every 200ms

  * @retval int

  */

int main(void)

{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */



    HAL_Init();



    /* USER CODE BEGIN Init */

    /* USER CODE END Init */



    SystemClock_Config();



    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */



    MX_GPIO_Init();

    MX_DMA_Init();

    MX_HRTIM1_Init();

    MX_OPAMP1_Init();

    MX_OPAMP2_Init();

    MX_OPAMP3_Init();

    MX_OPAMP4_Init();

    MX_TIM1_Init();

    MX_TIM2_Init();

    MX_TIM5_Init();

    MX_I2C2_Init();

    MX_USART1_UART_Init();

    MX_SPI3_Init();

    MX_USART3_UART_Init();

    MX_SPI1_Init();         /* SPI1 1LINE DMA: LCD 1.69", 42.5MHz, must init before LVGL */

    MX_ADC1_Init();

    MX_ADC2_Init();

    MX_ADC5_Init();

    MX_ADC3_Init();

    MX_ADC4_Init();

    MX_TIM6_Init();

    MX_TIM7_Init();

    MX_TIM16_Init();

    MX_TIM3_Init();         /* TIM3 CH1 PWM: LCD backlight brightness control */

    MX_TIM17_Init();



    /* USER CODE BEGIN 2 */

    /* ---------------------------------------------------------------
     * Step1: SPI1 + LCD hardware init
     *   - BackLight OFF
     *   - LCD_1IN69_InitReg (register sequence)
     *   - LCD_1IN69_Clear(BLACK)  wait DMA done
     *   - BackLight ON  <- user first sees pure black screen
     * --------------------------------------------------------------- */
    LCD_HW_Init();

    /* ---------------------------------------------------------------
     * Step2: LVGL init - LCD is guaranteed pure BLACK at this point
     * --------------------------------------------------------------- */
    lv_init();
    lv_port_disp_init();     /* register SPI1 DMA display driver */
    lv_port_indev_init();    /* register rotary encoder input    */

    /* ---------------------------------------------------------------
     * Step3: Build all UI pages and show START splash screen
     * --------------------------------------------------------------- */
    ui_main_init();          /* creates START/HOME/MENU, loads START */
    lv_timer_handler();      /* render frame 1 */
    lv_timer_handler();      /* render frame 2 */

    /* ---------------------------------------------------------------
     * Step4: Init remaining board peripherals
     *   (ADC/OPAMP/Flash/TMP/TIM6/7/16/17/PWM/Encoder/PID)
     *   LCD is NOT re-initialized here
     * --------------------------------------------------------------- */
    BoardDevice_Init();

    /* ---------------------------------------------------------------
     * Step5: Show ready, switch to HOME
     * --------------------------------------------------------------- */
    ui_splash_set_progress(100, "System Ready !");
    lv_timer_handler();
    HAL_Delay(800);

    ui_goto_home();
    lv_timer_handler();
    lv_timer_handler();
    printf("UI Ready - HOME page\r\n");

    /* USER CODE END 2 */



    static uint32_t ui_tick = 0;



    /* USER CODE BEGIN WHILE */

    while (1)
    {
        /* ---- ENC_KEY long/short press detection ---- */
        if (g_enc_key_held) {
            if (HAL_GetTick() - g_enc_key_press_tick >= 1000U) {
                /* Long press 2s: enter MENU from HOME, or back to HOME from MENU */
                g_enc_key_held = 0;
                g_enc_key_long_flag = 1;
            } else if (HAL_GPIO_ReadPin(ENCODE1_KEY_GPIO_Port, ENCODE1_KEY_Pin) == GPIO_PIN_SET) {
                /* Released before 2s = short press */
                g_enc_key_held = 0;
                g_enc_key_flag = 1;
            }
        }

        /* ---- Process input events (ISR flags -> main loop -> LVGL) ---- */
        uint8_t page = ui_get_page();

        if (g_enc_key_long_flag) {
            g_enc_key_long_flag = 0;
            if (page == UI_PAGE_HOME)  ui_goto_menu();
            else if (page == UI_PAGE_MENU) ui_goto_home();
        }
        if (g_enc_key_flag) {
            g_enc_key_flag = 0;
            if (page == UI_PAGE_HOME)       { ui_vout_toggle_adjust(); lv_port_encoder_key(); }
            else if (page == UI_PAGE_MENU)  { ui_menu_enter(); }
            else if (page == UI_PAGE_PID)   { ui_pid_cursor_move(); }
            else if (page == UI_PAGE_MODE)  { ui_work_confirm(); }
            else if (page == UI_PAGE_CVCC)  { ui_cvcc_confirm(); }
            else if (page == UI_PAGE_FAN)   { ui_fan_key(); }
        }
        if (g_key2_flag) {
            g_key2_flag = 0;
            if (page == UI_PAGE_HOME)       ui_vout_cursor_left();
            else if (page == UI_PAGE_MENU)  ui_goto_home();
            else if (page == UI_PAGE_PID)   ui_subpage_back();
            else if (page == UI_PAGE_MODE)  ui_subpage_back();
            else if (page == UI_PAGE_CVCC)  ui_subpage_back();
            else if (page == UI_PAGE_FAN)   { Config_Save(); ui_subpage_back(); }
            else                            ui_subpage_back();
        }
        if (g_key3_flag) {
            g_key3_flag = 0;
            if (page == UI_PAGE_HOME)       ui_vout_cursor_right();
            else if (page == UI_PAGE_PID)   ui_pid_next_param();
        }
        if (g_key1_flag) {
            g_key1_flag = 0;
            /* KEY1: pure output toggle */
            if (CTRL.HRTIMActive) {
                CTRL_Stop();
            } else {
                CTRL_Start();
            }
        }
        if (g_enc_diff != 0) {
            int8_t d = g_enc_diff;
            g_enc_diff = 0;
            if (page == UI_PAGE_HOME)       { if (d > 0) ui_vout_digit_inc(); else ui_vout_digit_dec(); }
            else if (page == UI_PAGE_MENU)  { if (d > 0) ui_menu_next();      else ui_menu_prev(); }
            else if (page == UI_PAGE_PID)   { ui_pid_adjust(d); }
            else if (page == UI_PAGE_MODE)  { ui_work_toggle(d); }
            else if (page == UI_PAGE_CVCC)  { ui_cvcc_toggle(d); }
            else if (page == UI_PAGE_FAN)    { ui_fan_enc(d); }
            else if (page == UI_PAGE_SCREEN) { ui_screen_enc(d); }
        }


        /* ---- LVGL render ---- */
        lv_timer_handler();

        /* ---- Dashboard refresh every 200ms ---- */
        if (HAL_GetTick() - ui_tick >= 200U) {
            ui_main_update();
            ui_tick = HAL_GetTick();
        }



        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */

    }

    /* USER CODE END 3 */

}


/**
  * @brief  System clock config: HSE(12MHz)->PLL->SYSCLK=170MHz, CSS enabled
  * @note   HSE(12MHz) -> PLL -> SYSCLK = 170MHz
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM       = RCC_PLLM_DIV6;
    RCC_OscInitStruct.PLL.PLLN       = 85;
    RCC_OscInitStruct.PLL.PLLP       = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ       = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR       = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
    RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                                     | RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) Error_Handler();
    HAL_RCC_EnableCSS();
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  Error handler: disable IRQ and loop, waiting for hardware reset
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    __disable_irq();
    while (1) {}
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    printf("Assert failed: file %s, line %lu\r\n", file, (unsigned long)line);
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
