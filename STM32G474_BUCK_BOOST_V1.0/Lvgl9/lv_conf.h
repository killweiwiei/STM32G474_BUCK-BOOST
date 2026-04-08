/**
 * @file lv_conf.h
 * LVGL v9.3.0 configuration for STM32G474 BUCK-BOOST
 * LCD: 240x280 RGB565 SPI1
 */
/* clang-format off */
#if 1
#ifndef LV_CONF_H
#define LV_CONF_H

/* Color depth: 16 = RGB565 */
#define LV_COLOR_DEPTH 16

/* Stdlib */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_STRING    LV_STDLIB_BUILTIN
#define LV_USE_STDLIB_SPRINTF   LV_STDLIB_BUILTIN
#define LV_STDINT_INCLUDE       <stdint.h>
#define LV_STDDEF_INCLUDE       <stddef.h>
#define LV_STDBOOL_INCLUDE      <stdbool.h>
#define LV_INTTYPES_INCLUDE     <inttypes.h>
#define LV_LIMITS_INCLUDE       <limits.h>
#define LV_STDARG_INCLUDE       <stdarg.h>

/* Memory: 48KB heap */
#define LV_MEM_SIZE             (60 * 1024U)
    #define LV_MEM_POOL_EXPAND_SIZE 0
#define LV_MEM_ADR              0

/* OS */
#define LV_USE_OS               LV_OS_NONE
#define LV_USE_IDLE_CB          0
#define LV_DRAW_BUF_STRIDE_ALIGN 1
#define LV_DRAW_BUF_ALIGN        4
#define LV_USE_MATRIX            0
#define LV_DRAW_THREAD_STACK_SIZE (8 * 1024) /*  RTOS�此����� warning */

/* Draw */
#define LV_USE_DRAW_SW          1
    #define LV_DRAW_SW_DRAW_UNIT_CNT    1
        #define LV_DRAW_SW_SHADOW_CACHE_SIZE 0
#define LV_DRAW_SW_CIRCLE_CACHE_CNT  4
#define LV_DRAW_SW_COMPLEX           1

/* Display */
#define LV_DISPLAY_DEF_REFR_PERIOD  20
#define LV_DPI_DEF                  130
#define LV_DEF_REFR_PERIOD          20

/* Input */
#define LV_INDEV_DEF_READ_PERIOD        20
#define LV_INDEV_DEF_SCROLL_THROW       10
#define LV_INDEV_DEF_SCROLL_LIMIT       10
#define LV_INDEV_DEF_LONG_PRESS_TIME    400
#define LV_INDEV_DEF_LONG_PRESS_REP_TIME 100

/* Log: off */
#define LV_USE_LOG              0
#define LV_USE_ASSERT_NULL      1
#define LV_USE_ASSERT_MALLOC    1
#define LV_USE_ASSERT_STYLE     0
#define LV_USE_ASSERT_MEM_INTEGRITY 0
#define LV_USE_ASSERT_OBJ       0
#define LV_USE_PERF_MONITOR     0
#define LV_USE_MEM_MONITOR      0

/* Fonts */
#define LV_FONT_MONTSERRAT_12   1
#define LV_FONT_MONTSERRAT_14   1
#define LV_FONT_MONTSERRAT_16   1
#define LV_FONT_MONTSERRAT_20   1
#define LV_FONT_MONTSERRAT_28   1
#define LV_FONT_MONTSERRAT_32   1
#define LV_FONT_MONTSERRAT_24   1
#define LV_FONT_DEFAULT         &lv_font_montserrat_14
#define LV_FONT_FMT_TXT_LARGE   0
#define LV_USE_FONT_PLACEHOLDER 1

/* Widgets */
#define LV_USE_ANIMIMG      0
#define LV_USE_ARC          1
#define LV_USE_BAR          1
#define LV_USE_BUTTON       1
#define LV_USE_BUTTONMATRIX 0
#define LV_USE_CALENDAR     0
#define LV_USE_CANVAS       0
#define LV_USE_CHART        1
#define LV_USE_CHECKBOX     0
#define LV_USE_DROPDOWN     1
#define LV_USE_IMAGE        1  /* arc/bar��件��image�� */
#define LV_USE_IMAGEBUTTON  0
#define LV_USE_KEYBOARD     0
#define LV_USE_LABEL        1
#define LV_USE_LED          1
#define LV_USE_LINE         1
#define LV_USE_LIST         0
#define LV_USE_LOTTIE       0
#define LV_USE_MENU         0
#define LV_USE_MSGBOX       0
#define LV_USE_ROLLER       0
#define LV_USE_SCALE        1
#define LV_USE_SLIDER       1
#define LV_USE_SPAN         0
#define LV_USE_SPINBOX      0
#define LV_USE_SPINNER      1
#define LV_USE_SWITCH       1
#define LV_USE_TABLE        0
#define LV_USE_TABVIEW      1
#define LV_USE_TEXTAREA     0
#define LV_USE_TILEVIEW     0
#define LV_USE_WIN          0

/* Theme: dark */
#define LV_USE_THEME_DEFAULT    1
#define LV_THEME_DEFAULT_DARK        1
#define LV_THEME_DEFAULT_GROW        1
    #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#define LV_USE_THEME_SIMPLE     0
#define LV_USE_THEME_MONO       0

/* Layout */
#define LV_USE_FLEX     1
#define LV_USE_GRID     0

/* 3rd party: all off */
#define LV_USE_LODEPNG      0
#define LV_USE_LIBPNG       0
#define LV_USE_BMP          0
#define LV_USE_TJPGD        0
#define LV_USE_LIBJPEG_TURBO 0
#define LV_USE_GIF          0
#define LV_USE_QRCODE       0
#define LV_USE_BARCODE      0
#define LV_USE_FREETYPE     0
#define LV_USE_TINY_TTF     0
#define LV_USE_RLOTTIE      0
#define LV_USE_FFMPEG       0

/* Others */
#define LV_USE_SNAPSHOT     0
#define LV_USE_MONKEY       0
#define LV_USE_GRIDNAV      0
#define LV_USE_FRAGMENT     0
#define LV_USE_IMGFONT      0
#define LV_USE_OBSERVER     1
#define LV_USE_GUIBUILDER   0
#define LV_USE_IME_PINYIN   0
#define LV_USE_FILE_EXPLORER 0
#define LV_USE_OBJ_ID       0
#define LV_USE_OBJ_ID_BUILTIN 0
#define LV_USE_OBJ_PROPERTY 0
#define LV_USE_OBJ_PROPERTY_NAME 0
#define LV_USE_XML          0

#endif /* LV_CONF_H */
#endif /* if 1 */
