/**
 * @file    lv_port_disp.c
 * @brief   LVGL9 display driver for LCD_1IN69 (240x280, RGB565, SPI1 1LINE)
 * @note    Command bytes use BLOCKING HAL_SPI_Transmit (safe for stack vars)
 *          Pixel data uses DMA (buffer is LVGL static heap, address stable)
 *          DMA complete callback: HAL_SPI_TxCpltCallback -> lv_port_disp_flush_done
 */

#include "lv_port_disp.h"
#include "LCD_1in69.h"
#include "DEV_Config.h"
#include "spi.h"
#include "draw/sw/lv_draw_sw_utils.h"

#define DISP_BUF_LINES  28
#define DISP_BUF_SIZE   (LCD_1IN69_WIDTH * DISP_BUF_LINES * 2)

static lv_display_t *s_disp = NULL;
static uint8_t buf1[DISP_BUF_SIZE];
static uint8_t buf2[DISP_BUF_SIZE];

/* DMA busy flag: 0=busy, 1=idle */
static volatile uint8_t s_dma_idle = 1;

/* ----------------------------------------------------------------
 * Called from HAL_SPI_TxCpltCallback in callback.c
 * Raises CS and notifies LVGL that flush is complete
 * ---------------------------------------------------------------- */
void lv_port_disp_flush_done(void)
{
    s_dma_idle = 1;
    LCD_1IN69_CS_1;
    if (s_disp != NULL)
        lv_display_flush_ready(s_disp);
}

/* ----------------------------------------------------------------
 * Send 1-byte command (BLOCKING - safe for local variable)
 * Sequence: wait DMA idle -> CS=0 -> DC=0 -> TX 1 byte -> CS stays low
 * ---------------------------------------------------------------- */
static void lcd_cmd(uint8_t cmd)
{
    /* Wait for any ongoing DMA pixel transfer */
    while (!s_dma_idle);
    while (HAL_DMA_GetState(&hdma_spi1_tx) != HAL_DMA_STATE_READY);
    while (HAL_SPI_GetState(&hspi1)        != HAL_SPI_STATE_READY);

    LCD_1IN69_CS_0;
    LCD_1IN69_DC_0;
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);  /* BLOCKING */
    /* CS stays low, ready for data bytes */
}

/* ----------------------------------------------------------------
 * Send data bytes (BLOCKING - d[] is local, must use blocking TX)
 * Sequence: DC=1 -> TX bytes -> CS=1
 * ---------------------------------------------------------------- */
static void lcd_data(uint8_t *d, uint8_t len)
{
    LCD_1IN69_DC_1;
    HAL_SPI_Transmit(&hspi1, d, len, 100);   /* BLOCKING */
    LCD_1IN69_CS_1;
}

/* ----------------------------------------------------------------
 * Set display window (VERTICAL mode, Y+20 panel offset)
 * ---------------------------------------------------------------- */
static void lcd_set_window(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint8_t d[4];

    /* Column address (0x2A) */
    lcd_cmd(0x2A);
    d[0] = x1 >> 8;  d[1] = x1 & 0xFF;
    d[2] = x2 >> 8;  d[3] = x2 & 0xFF;
    lcd_data(d, 4);

    /* Row address (0x2B) with +20 panel offset */
    lcd_cmd(0x2B);
    d[0] = (y1 + 20) >> 8;  d[1] = (y1 + 20) & 0xFF;
    d[2] = (y2 + 20) >> 8;  d[3] = (y2 + 20) & 0xFF;
    lcd_data(d, 4);

    /* RAM write command (0x2C): CS stays low, DC=1 for pixel data */
    lcd_cmd(0x2C);
    LCD_1IN69_DC_1;
    /* CS remains low - pixel DMA will follow immediately */
}

/* ----------------------------------------------------------------
 * LVGL flush callback
 * Commands/window: blocking SPI
 * Pixel data: DMA non-blocking (px_map is LVGL static buffer)
 * ---------------------------------------------------------------- */
static void disp_flush(lv_display_t *display, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t x1 = (uint16_t)area->x1;
    uint16_t y1 = (uint16_t)area->y1;
    uint16_t x2 = (uint16_t)area->x2;
    uint16_t y2 = (uint16_t)area->y2;
    uint32_t npix   = (uint32_t)(x2 - x1 + 1) * (y2 - y1 + 1);
    uint16_t nbytes = (uint16_t)(npix * 2);  /* max 40*240*2=19200 < 65535 */

    /* Swap bytes: LVGL RGB565 little-endian -> LCD big-endian */
    lv_draw_sw_rgb565_swap(px_map, npix);

    /* Set window (blocking): CS=0, DC=1 after 0x2C, ready for pixels */
    lcd_set_window(x1, y1, x2, y2);

    /* Send pixel data via DMA (non-blocking)
     * px_map points into buf1/buf2 (static globals) - address is stable
     * On complete: HAL_SPI_TxCpltCallback -> lv_port_disp_flush_done()
     *           -> CS=1, lv_display_flush_ready() -> LVGL renders next block */
    s_dma_idle = 0;
    HAL_SPI_Transmit_DMA(&hspi1, px_map, nbytes);
}

/* ----------------------------------------------------------------
 * Init: call after lv_init() and LCD_1IN69_Init()
 * ---------------------------------------------------------------- */
void lv_port_disp_init(void)
{
    s_disp = lv_display_create(LCD_1IN69_WIDTH, LCD_1IN69_HEIGHT);
    lv_display_set_buffers(s_disp, buf1, buf2, sizeof(buf1),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_color_format(s_disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(s_disp, disp_flush);
    lv_display_set_rotation(s_disp, LV_DISPLAY_ROTATION_0);
}
