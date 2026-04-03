/**

 * @file    ui_main.c

 * @brief   LVGL9 BUCK-BOOST UI - START/HOME/MENU three-screen state machine

 * @note    Screen: 240x280, RGB565

 *          START: splash + progress bar (auto switch to HOME when 100%)

 *          HOME : real-time voltage/current/temp/mode display

 *          MENU : reserved for key-controlled settings

 */



#include "ui_main.h"

#include "config.h"

#include "control.h"

#include <stdio.h>
#include <string.h>
#include "config_store.h"



/* Page enum */

/* ui_page_t uses public ui_page_id_t from ui_main.h */
typedef ui_page_id_t ui_page_t;

static ui_page_t s_page = UI_PAGE_START;



/* START page handles */

static lv_obj_t *s_scr_start;

static lv_obj_t *s_bar_prog;

static lv_obj_t *s_lbl_pct;

static lv_obj_t *s_lbl_step;



/* HOME page handles */

static lv_obj_t *s_scr_home;

static lv_obj_t *s_lbl_vin;

static lv_obj_t *s_lbl_vout;

static lv_obj_t *s_lbl_iin;

static lv_obj_t *s_lbl_iout;

static lv_obj_t *s_lbl_mtemp;

static lv_obj_t *s_lbl_btemp;

static lv_obj_t *s_lbl_mode;

static lv_obj_t *s_lbl_status;



/* MENU page handles */

static lv_obj_t *s_scr_menu;
static lv_obj_t *s_scr_pid;
static lv_obj_t *s_scr_mode;   /* CV/CC output mode page */
static lv_obj_t *s_scr_work;   /* Work Mode (BUCK/BOOST/BB/AUTO) page */
static lv_obj_t *s_scr_lang;
static lv_obj_t *s_scr_about;
static lv_obj_t *s_scr_screen = NULL;
static lv_obj_t *s_lbl_brightness = NULL;
static uint8_t   s_brightness = 80;
static uint8_t   s_menu_sel  = 0;
static uint8_t   s_ctrl_cc   = 0;  /* 0=CV(constant voltage), 1=CC(constant current) */
static lv_obj_t *s_lbl_cvcc  = NULL; /* HOME CV/CC indicator label */
/* s_menu_items declared above with MENU_COUNT */
static lv_obj_t *s_lbl_kp_val;
static lv_obj_t *s_lbl_ki_val;
static lv_obj_t *s_lbl_kd_val;
static lv_obj_t *s_lbl_kp_cur;  /* Kp cursor row */
static lv_obj_t *s_lbl_ki_cur;  /* Ki cursor row */
static lv_obj_t *s_lbl_kd_cur;  /* Kd cursor row */
static uint8_t   s_pid_sel    = 0;   /* 0=Kp 1=Ki 2=Kd */
static uint8_t   s_pid_cursor = 0;   /* digit cursor position */
/* Kp/Ki: XXXXX.X format, 6 digits, cursor 0..5 */
/* Kd:    X.XXXX  format, 5 digits, cursor 0..4 */
static lv_obj_t *s_lbl_mode_cur;




/* ================================================================

 * Common helpers

 * ================================================================ */

static void ui_sep(lv_obj_t *p, int32_t y)

{

    lv_obj_t *s = lv_obj_create(p);

    lv_obj_set_pos(s, 0, y); lv_obj_set_size(s, 240, 2);

    lv_obj_set_style_bg_color(s, lv_color_hex(0x21262D), 0);

    lv_obj_set_style_bg_opa(s, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(s, 0, 0);

    lv_obj_set_style_pad_all(s, 0, 0);

    lv_obj_clear_flag(s, LV_OBJ_FLAG_SCROLLABLE);

}



static void ui_vdiv(lv_obj_t *p, int32_t x, int32_t y, int32_t h)

{

    lv_obj_t *d = lv_obj_create(p);

    lv_obj_set_pos(d, x, y); lv_obj_set_size(d, 2, h);

    lv_obj_set_style_bg_color(d, lv_color_hex(0x21262D), 0);

    lv_obj_set_style_bg_opa(d, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(d, 0, 0);

    lv_obj_set_style_pad_all(d, 0, 0);

    lv_obj_clear_flag(d, LV_OBJ_FLAG_SCROLLABLE);

}



static void ui_cell(lv_obj_t *p, int32_t x, int32_t y,

                    const char *tag, uint32_t tc, const char *init, lv_obj_t **pp)

{

    lv_obj_t *lt = lv_label_create(p);

    lv_label_set_text(lt, tag);

    lv_obj_set_style_text_color(lt, lv_color_hex(tc), 0);

    lv_obj_set_style_text_font(lt, &lv_font_montserrat_12, 0);

    lv_obj_set_pos(lt, x, y);

    lv_obj_t *lv = lv_label_create(p);

    lv_label_set_text(lv, init);

    lv_obj_set_style_text_color(lv, lv_color_hex(0xE6EDF3), 0);

    lv_obj_set_style_text_font(lv, &lv_font_montserrat_28, 0);

    lv_obj_set_pos(lv, x, y + 14);

    *pp = lv;

}



static lv_obj_t *ui_screen(void)

{

    lv_obj_t *scr = lv_obj_create(NULL);

    lv_obj_set_style_bg_color(scr, lv_color_hex(0x0D1117), 0);

    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    return scr;

}



static void ui_titlebar(lv_obj_t *p, const char *text, uint32_t color)

{

    lv_obj_t *bg = lv_obj_create(p);

    lv_obj_set_pos(bg, 0, 0); lv_obj_set_size(bg, 240, 28);

    lv_obj_set_style_bg_color(bg, lv_color_hex(0x161B22), 0);

    lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);

    lv_obj_set_style_border_width(bg, 0, 0);

    lv_obj_set_style_pad_all(bg, 0, 0);

    lv_obj_clear_flag(bg, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl = lv_label_create(bg);

    lv_label_set_text(lbl, text);

    lv_obj_set_style_text_color(lbl, lv_color_hex(color), 0);

    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);

    lv_obj_align(lbl, LV_ALIGN_CENTER, 0, 0);

}

/* ================================================================

 * START page - splash + progress bar

 * ================================================================ */

static void start_page_create(void)

{

    s_scr_start = ui_screen();



    /* Logo */

    lv_obj_t *l1 = lv_label_create(s_scr_start);

    lv_label_set_text(l1, "BUCK-BOOST");

    lv_obj_set_style_text_color(l1, lv_color_hex(0x58A6FF), 0);

    lv_obj_set_style_text_font(l1, &lv_font_montserrat_32, 0);

    lv_obj_align(l1, LV_ALIGN_CENTER, 0, -60);



    lv_obj_t *l2 = lv_label_create(s_scr_start);

    lv_label_set_text(l2, "STM32G474RET6");

    lv_obj_set_style_text_color(l2, lv_color_hex(0x8B949E), 0);

    lv_obj_set_style_text_font(l2, &lv_font_montserrat_14, 0);

    lv_obj_align(l2, LV_ALIGN_CENTER, 0, -20);



    lv_obj_t *l3 = lv_label_create(s_scr_start);

    lv_label_set_text(l3, "V1.1  by Robot");

    lv_obj_set_style_text_color(l3, lv_color_hex(0x30363D), 0);

    lv_obj_set_style_text_font(l3, &lv_font_montserrat_12, 0);

    lv_obj_align(l3, LV_ALIGN_CENTER, 0, 0);



    /* Progress bar */

    s_bar_prog = lv_bar_create(s_scr_start);

    lv_obj_set_size(s_bar_prog, 200, 12);

    lv_obj_align(s_bar_prog, LV_ALIGN_CENTER, 0, 50);

    lv_bar_set_range(s_bar_prog, 0, 100);

    lv_bar_set_value(s_bar_prog, 0, LV_ANIM_OFF);

    lv_obj_set_style_bg_color(s_bar_prog, lv_color_hex(0x58A6FF), LV_PART_INDICATOR);

    lv_obj_set_style_bg_color(s_bar_prog, lv_color_hex(0x21262D), LV_PART_MAIN);

    lv_obj_set_style_radius(s_bar_prog, 6, LV_PART_MAIN);

    lv_obj_set_style_radius(s_bar_prog, 6, LV_PART_INDICATOR);



    /* Percentage label */

    s_lbl_pct = lv_label_create(s_scr_start);

    lv_label_set_text(s_lbl_pct, "0%");

    lv_obj_set_style_text_color(s_lbl_pct, lv_color_hex(0x58A6FF), 0);

    lv_obj_set_style_text_font(s_lbl_pct, &lv_font_montserrat_14, 0);

    lv_obj_align(s_lbl_pct, LV_ALIGN_CENTER, 0, 70);



    /* Step description */

    s_lbl_step = lv_label_create(s_scr_start);

    lv_label_set_text(s_lbl_step, "Initializing...");

    lv_obj_set_style_text_color(s_lbl_step, lv_color_hex(0x6C7086), 0);

    lv_obj_set_style_text_font(s_lbl_step, &lv_font_montserrat_12, 0);

    lv_obj_align(s_lbl_step, LV_ALIGN_CENTER, 0, 88);

}



/* ================================================================

 * HOME page - real-time data dashboard

 * ================================================================ */



/* ================================================================

 * Vout cursor edit state

 * digits[0]=tens, digits[1]=ones, digits[2]=tenth, digits[3]=hundredth, digits[4]=thousandth

 * cursor_pos: 0-4

 * adjust_mode: 0=normal, 1=editing

 * ================================================================ */

static uint8_t  s_digits[5]   = {0, 0, 0, 0, 0}; /* Vout setpoint digits */

static uint8_t  s_cursor      = 1;                /* default: ones position */

static uint8_t  s_adj_mode    = 0;                /* 0=normal 1=adjust */



/* Individual digit labels for cursor highlight */

static lv_obj_t *s_vout_digits[5]; /* tens ones tenth hundredth thousandth */

static lv_obj_t *s_vout_dot;       /* '.' label */

static lv_obj_t *s_lbl_adj_hint;   /* hint label */



/* Other HOME labels */

static lv_obj_t *s_lbl_vin;

static lv_obj_t *s_lbl_iin;

static lv_obj_t *s_lbl_iout;

static lv_obj_t *s_lbl_mtemp;

static lv_obj_t *s_lbl_btemp;

static lv_obj_t *s_lbl_mode;

static lv_obj_t *s_lbl_status;

static lv_obj_t *s_lbl_vout_actual;

static lv_obj_t *s_lbl_vout_unit;



/* MENU page */




/* ================================================================

 * Internal: sync s_digits from CTRL.VoutRef

 * ================================================================ */

static void vout_ref_to_digits(void)

{

    float v = CTRL.VoutRef;

    if (v < 0.0f)  v = 0.0f;

    if (v > 19.999f) v = 19.999f;

    uint16_t iv = (uint16_t)(v * 1000.0f + 0.5f); /* e.g. 12.345 -> 12345 */

    s_digits[0] = (iv / 10000) % 10;

    s_digits[1] = (iv / 1000)  % 10;

    s_digits[2] = (iv / 100)   % 10;

    s_digits[3] = (iv / 10)    % 10;

    s_digits[4] = (iv)         % 10;

}



static float digits_to_vout_ref(void)

{

    return s_digits[0]*10.0f + s_digits[1]*1.0f +

           s_digits[2]*0.1f  + s_digits[3]*0.01f + s_digits[4]*0.001f;

}

static void output_ref_to_digits(void)

{

    float v = s_ctrl_cc ? CTRL.IoutLimit : CTRL.VoutRef;

    if (v < 0.0f) v = 0.0f;

    if (s_ctrl_cc) { if (v > 9.999f) v = 9.999f; }

    else { if (v > 19.999f) v = 19.999f; }

    {

        uint16_t iv = (uint16_t)(v * 1000.0f + 0.5f);

        s_digits[0] = (iv / 10000) % 10;

        s_digits[1] = (iv / 1000)  % 10;

        s_digits[2] = (iv / 100)   % 10;

        s_digits[3] = (iv / 10)    % 10;

        s_digits[4] = (iv)         % 10;

    }

}



/* ================================================================

 * Internal: refresh Vout setpoint digit labels

 * ================================================================ */

static void output_ref_step(int8_t dir)

{

    static const uint16_t step_table[5] = {10000, 1000, 100, 10, 1};

    uint16_t iv;

    uint16_t max_iv = s_ctrl_cc ? 9999U : 19999U;

    uint16_t step = step_table[s_cursor];

    iv = (uint16_t)(s_digits[0] * 10000U +
                    s_digits[1] * 1000U +
                    s_digits[2] * 100U +
                    s_digits[3] * 10U +
                    s_digits[4]);

    if (dir > 0) {

        if (iv + step <= max_iv) iv += step;
        else                     iv = max_iv;

    } else {

        if (iv >= step) iv -= step;
        else            iv = 0;

    }

    s_digits[0] = (iv / 10000U) % 10U;

    s_digits[1] = (iv / 1000U)  % 10U;

    s_digits[2] = (iv / 100U)   % 10U;

    s_digits[3] = (iv / 10U)    % 10U;

    s_digits[4] = (iv)          % 10U;

}


static void vout_digits_refresh(void)

{

    char ch[2] = {0, 0};

    uint8_t i;

    static const int32_t pos_cv[5] = {8, 28, 54, 74, 94};
    static const int32_t pos_cc[5] = {-20, 8, 34, 54, 74};

    for (i = 0; i < 5; i++) {

        ch[0] = '0' + s_digits[i];

        lv_label_set_text(s_vout_digits[i], ch);

        if (s_ctrl_cc) {
            lv_obj_set_x(s_vout_digits[i], pos_cc[i]);
            if (i == 0) lv_obj_add_flag(s_vout_digits[i], LV_OBJ_FLAG_HIDDEN);
            else        lv_obj_clear_flag(s_vout_digits[i], LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_set_x(s_vout_digits[i], pos_cv[i]);
            lv_obj_clear_flag(s_vout_digits[i], LV_OBJ_FLAG_HIDDEN);
        }

        if (s_adj_mode && i == s_cursor) {

            lv_obj_set_style_text_color(s_vout_digits[i], lv_color_hex(0xFFD700), 0);

            lv_obj_set_style_text_decor(s_vout_digits[i], LV_TEXT_DECOR_UNDERLINE, 0);

        } else {

            lv_obj_set_style_text_color(s_vout_digits[i], lv_color_hex(s_ctrl_cc ? 0xFF1493 : 0x56D4F8), 0);

            lv_obj_set_style_text_decor(s_vout_digits[i], LV_TEXT_DECOR_NONE, 0);

        }

    }

    if (s_vout_dot) {
        lv_obj_set_x(s_vout_dot, s_ctrl_cc ? 28 : 48);
    }
    if (s_lbl_vout_unit) {
        lv_obj_set_x(s_lbl_vout_unit, s_ctrl_cc ? 98 : 118);
    }

}


/* ================================================================

 * HOME page create

 * Layout: 240x280

 *   y=0   title bar

 *   y=28  INPUT | OUTPUT column headers

 *   y=46  Vin xx.xxxV | Vout setpoint edit

 *   y=90  sep

 *   y=94  Iin xx.xxxA | Iout xx.xxxA

 *   y=138 sep

 *   y=142 MCU temp | BRD temp

 *   y=182 sep

 *   y=186 Mode | State

 *   y=240 sep

 *   y=244 Vout actual (measured)

 * ================================================================ */

static void home_page_create(void)
{
    s_scr_home = ui_screen();
    ui_titlebar(s_scr_home, "BUCK-BOOST", 0x58A6FF);

    /* ADJ hint: below title right */
    s_lbl_adj_hint = lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_adj_hint, "");
    lv_obj_set_style_text_color(s_lbl_adj_hint, lv_color_hex(0xFFD700), 0);
    lv_obj_set_style_text_font(s_lbl_adj_hint, &lv_font_montserrat_12, 0);
    lv_obj_align(s_lbl_adj_hint, LV_ALIGN_TOP_RIGHT, -6, 22);

    /* OUTPUT section */
    lv_obj_t *lo_hdr = lv_label_create(s_scr_home);
    lv_label_set_text(lo_hdr, "OUTPUT");
    lv_obj_set_style_text_color(lo_hdr, lv_color_hex(0x3FB950), 0);
    lv_obj_set_style_text_font(lo_hdr, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(lo_hdr, 8, 22);
    { int32_t dx=8,dy=36; int32_t xs[5]={dx,dx+20,dx+46,dx+66,dx+86}; uint8_t i;
      for(i=0;i<5;i++){
        s_vout_digits[i]=lv_label_create(s_scr_home);
        lv_label_set_text(s_vout_digits[i],"0");
        lv_obj_set_style_text_color(s_vout_digits[i],lv_color_hex(0x40C9FF),0);
        lv_obj_set_style_text_font(s_vout_digits[i],&lv_font_montserrat_32,0);
        lv_obj_set_pos(s_vout_digits[i],xs[i],dy);}
      s_vout_dot=lv_label_create(s_scr_home);
      lv_label_set_text(s_vout_dot,".");
      lv_obj_set_style_text_color(s_vout_dot,lv_color_hex(0x8B949E),0);
      lv_obj_set_style_text_font(s_vout_dot,&lv_font_montserrat_32,0);
      lv_obj_set_pos(s_vout_dot,dx+40,dy);
      s_lbl_vout_unit=lv_label_create(s_scr_home);
      lv_label_set_text(s_lbl_vout_unit,"V");
      lv_obj_set_style_text_color(s_lbl_vout_unit,lv_color_hex(0x40C9FF),0);
      lv_obj_set_style_text_font(s_lbl_vout_unit,&lv_font_montserrat_20,0);
      lv_obj_set_pos(s_lbl_vout_unit,dx+110,dy+10); }
    /* Vout actual - no tag */
    s_lbl_vout_actual=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_vout_actual,"00.000V");
    lv_obj_set_style_text_color(s_lbl_vout_actual,lv_color_hex(0x7FFF00),0);
    lv_obj_set_style_text_font(s_lbl_vout_actual,&lv_font_montserrat_20,0);
    lv_obj_set_pos(s_lbl_vout_actual,8,74);
    /* Iout - no tag */
    s_lbl_iout=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_iout,"00.000A");
    lv_obj_set_style_text_color(s_lbl_iout,lv_color_hex(0xFF0A96),0);
    lv_obj_set_style_text_font(s_lbl_iout,&lv_font_montserrat_20,0);
    lv_obj_set_pos(s_lbl_iout,132,74);
    ui_sep(s_scr_home,100);

    /* INPUT section */
    lv_obj_t *li_hdr=lv_label_create(s_scr_home);
    lv_label_set_text(li_hdr,"INPUT");
    lv_obj_set_style_text_color(li_hdr,lv_color_hex(0x79C0FF),0);
    lv_obj_set_style_text_font(li_hdr,&lv_font_montserrat_12,0);
    lv_obj_set_pos(li_hdr,8,104);
    s_lbl_vin=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_vin,"00.000V");
    lv_obj_set_style_text_color(s_lbl_vin,lv_color_hex(0x7FFF00),0);
    lv_obj_set_style_text_font(s_lbl_vin,&lv_font_montserrat_20,0);
    lv_obj_set_pos(s_lbl_vin,8,114);
    s_lbl_iin=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_iin,"00.000A");
    lv_obj_set_style_text_color(s_lbl_iin,lv_color_hex(0xFF0A96),0);
    lv_obj_set_style_text_font(s_lbl_iin,&lv_font_montserrat_20,0);
    lv_obj_set_pos(s_lbl_iin,132,114);
    ui_sep(s_scr_home,140);

    /* TEMP section */
    lv_obj_t *lt_hdr=lv_label_create(s_scr_home);
    lv_label_set_text(lt_hdr,"TEMP");
    lv_obj_set_style_text_color(lt_hdr,lv_color_hex(0xF0883E),0);
    lv_obj_set_style_text_font(lt_hdr,&lv_font_montserrat_12,0);
    lv_obj_set_pos(lt_hdr,8,144);
    lv_obj_t *ltm_tag=lv_label_create(s_scr_home);
    lv_label_set_text(ltm_tag,"MCU");
    lv_obj_set_style_text_color(ltm_tag,lv_color_hex(0x8B949E),0);
    lv_obj_set_style_text_font(ltm_tag,&lv_font_montserrat_12,0);
    lv_obj_set_pos(ltm_tag,8,156);
    s_lbl_mtemp=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_mtemp,"00.00C");
    lv_obj_set_style_text_color(s_lbl_mtemp,lv_color_hex(0xFF9F43),0);
    lv_obj_set_style_text_font(s_lbl_mtemp,&lv_font_montserrat_16,0);
    lv_obj_set_pos(s_lbl_mtemp,8,168);
    lv_obj_t *ltb_tag=lv_label_create(s_scr_home);
    lv_label_set_text(ltb_tag,"BRD");
    lv_obj_set_style_text_color(ltb_tag,lv_color_hex(0x8B949E),0);
    lv_obj_set_style_text_font(ltb_tag,&lv_font_montserrat_12,0);
    lv_obj_set_pos(ltb_tag,132,156);
    s_lbl_btemp=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_btemp,"00.00C");
    lv_obj_set_style_text_color(s_lbl_btemp,lv_color_hex(0xEE5A24),0);
    lv_obj_set_style_text_font(s_lbl_btemp,&lv_font_montserrat_16,0);
    lv_obj_set_pos(s_lbl_btemp,132,168);
    ui_sep(s_scr_home,190);

    /* MODE/STATE/CVCC section */
    lv_obj_t *lmd_tag=lv_label_create(s_scr_home);
    lv_label_set_text(lmd_tag,"Mode");
    lv_obj_set_style_text_color(lmd_tag,lv_color_hex(0x8B949E),0);
    lv_obj_set_style_text_font(lmd_tag,&lv_font_montserrat_12,0);
    lv_obj_align(lmd_tag,LV_ALIGN_TOP_MID,-78,194);
    s_lbl_mode=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_mode,"BUCK");
    lv_obj_set_style_text_color(s_lbl_mode,lv_color_hex(0x0000EE),0);
    lv_obj_set_style_text_font(s_lbl_mode,&lv_font_montserrat_20,0);
    lv_obj_align(s_lbl_mode,LV_ALIGN_TOP_MID,-78,208);
    /* CV/CC: same layout as Mode/State */
    lv_obj_t *lcvcc_tag=lv_label_create(s_scr_home);
    lv_label_set_text(lcvcc_tag,"CV/CC");
    lv_obj_set_style_text_color(lcvcc_tag,lv_color_hex(0x8B949E),0);
    lv_obj_set_style_text_font(lcvcc_tag,&lv_font_montserrat_12,0);
    lv_obj_align(lcvcc_tag,LV_ALIGN_TOP_MID,0,194);
    s_lbl_cvcc=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_cvcc,"CV");
    lv_obj_set_style_text_color(s_lbl_cvcc,lv_color_hex(0x00FFFF),0);
    lv_obj_set_style_text_font(s_lbl_cvcc,&lv_font_montserrat_20,0);
    lv_obj_align(s_lbl_cvcc,LV_ALIGN_TOP_MID,0,208);
    lv_obj_t *lst_tag=lv_label_create(s_scr_home);
    lv_label_set_text(lst_tag,"State");
    lv_obj_set_style_text_color(lst_tag,lv_color_hex(0x8B949E),0);
    lv_obj_set_style_text_font(lst_tag,&lv_font_montserrat_12,0);
    lv_obj_align(lst_tag,LV_ALIGN_TOP_MID,78,194);
    s_lbl_status=lv_label_create(s_scr_home);
    lv_label_set_text(s_lbl_status,"STOP");
    lv_obj_set_style_text_color(s_lbl_status,lv_color_hex(0xFF0000),0);
    lv_obj_set_style_text_font(s_lbl_status,&lv_font_montserrat_20,0);
    lv_obj_align(s_lbl_status,LV_ALIGN_TOP_MID,78,208);
    ui_sep(s_scr_home,234);

    /* Bottom hint CENTERED */
    lv_obj_t *lhint=lv_label_create(s_scr_home);
    lv_label_set_text(lhint,"[HOLD]Menu  [ENC]Adj  [K2]< [K3]>");
    lv_obj_set_style_text_color(lhint,lv_color_hex(0x6E7681),0);
    lv_obj_set_style_text_font(lhint,&lv_font_montserrat_12,0);
    lv_obj_align(lhint,LV_ALIGN_TOP_MID,0,248);
}

#define MENU_COUNT 7
static const char *s_menu_labels[MENU_COUNT] = {
    "1. Output Mode",
    "2. PID Params",
    "3. Work Mode",
    "4. Fan Control",
    "5. Screen",
    "6. Language",    /* fixed penultimate */
    "7. About",       /* fixed last */
};
static lv_obj_t *s_menu_items[MENU_COUNT];  /* redeclare with new size */

static void menu_items_refresh(void)
{
    uint8_t i;
    for (i = 0; i < MENU_COUNT; i++) {
        if (!s_menu_items[i]) continue;
        lv_obj_set_style_text_color(s_menu_items[i],
            (i == s_menu_sel) ? lv_color_hex(0xF7C948) : lv_color_hex(0xCDD6F4), 0);
    }
}

static void menu_page_create(void)
{
    s_scr_menu = ui_screen();
    ui_titlebar(s_scr_menu, "MENU", 0xD2A8FF);
    uint8_t i;
    for (i = 0; i < MENU_COUNT; i++) {
        s_menu_items[i] = lv_label_create(s_scr_menu);
        lv_label_set_text(s_menu_items[i], s_menu_labels[i]);
        lv_obj_set_style_text_font(s_menu_items[i], &lv_font_montserrat_14, 0);
        lv_obj_set_pos(s_menu_items[i], 16, 36 + i * 36);
    }
    menu_items_refresh();
    lv_obj_t *lb = lv_label_create(s_scr_menu);
    lv_label_set_text(lb, "[ENC]Sel  [KEY]Enter  [K2]Back");
    lv_obj_set_style_text_color(lb, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(lb, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(lb, 4, 264);
}


/* pid_page_refresh */
static void pid_fmt_cursor(char *buf, size_t sz, float val,
                           const char *fmt, uint8_t cur)
{
    char num[16]; int i, dc=0, j=0;
    snprintf(num, sizeof(num), fmt, (double)val);
    for (i = 0; num[i] && j < (int)sz-1; i++) {
        char c = num[i];
        if (c != '.') {
            buf[j++]=c;
            dc++;
        } else { buf[j++]=c; }
    }
    buf[j]='\0';
}

static void pid_fmt_recolor(char *buf, size_t sz, float val,
                            const char *fmt, uint8_t cur)
{
    char num[24]; int i, dc=0, j=0, total=0;
    snprintf(num, sizeof(num), fmt, (double)val);
    for (i = 0; num[i]; i++) {
        if (num[i] != '.') total++;
    }
    /* cur: 0=rightmost lowest decimal, increasing to left */
    int target = total - 1 - (int)cur;
    for (i = 0; num[i] && j < (int)sz-16; i++) {
        char c = num[i];
        if (c != '.') {
            if (dc == target) {
                const char *tag1 = "#F7C948 ";
                const char *tag2 = "#";
                for (int k = 0; tag1[k] && j < (int)sz-1; k++) buf[j++] = tag1[k];
                if (j < (int)sz-1) buf[j++] = c;
                for (int k = 0; tag2[k] && j < (int)sz-1; k++) buf[j++] = tag2[k];
            } else {
                if (j < (int)sz-1) buf[j++] = c;
            }
            dc++;
        } else {
            if (j < (int)sz-1) buf[j++] = c;
        }
    }
    buf[j] = '\0';
}


static void pid_page_refresh(void)
{
    char buf[32], cur_ind[32];
    if (!s_lbl_kp_val) return;
    if (CTRL.PIDVout.Kp >  9999.0f) CTRL.PIDVout.Kp =  9999.0f;
    if (CTRL.PIDVout.Ki > 99999.0f) CTRL.PIDVout.Ki = 99999.0f;
    if (CTRL.PIDVout.Kd >    99.9f) CTRL.PIDVout.Kd =    99.9f;
    if (CTRL.PIDVout.Kp < 0.0f) CTRL.PIDVout.Kp = 0.0f;
    if (CTRL.PIDVout.Ki < 0.0f) CTRL.PIDVout.Ki = 0.0f;
    if (CTRL.PIDVout.Kd < 0.0f) CTRL.PIDVout.Kd = 0.0f;
    /* Kp */
    if (s_pid_sel == 0) {
        pid_fmt_recolor(buf, sizeof(buf), CTRL.PIDVout.Kp, "%.1f", s_pid_cursor);
        lv_label_set_recolor(s_lbl_kp_val, true);
        lv_obj_set_style_text_color(s_lbl_kp_val, lv_color_hex(0x4DA6FF), 0);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", CTRL.PIDVout.Kp);
        lv_label_set_recolor(s_lbl_kp_val, false);
        lv_obj_set_style_text_color(s_lbl_kp_val, lv_color_hex(0x2D6B8A), 0);
    }
    lv_label_set_text(s_lbl_kp_val, buf);
    if (s_lbl_kp_cur) lv_label_set_text(s_lbl_kp_cur, "");
    /* Ki */
    if (s_pid_sel == 1) {
        pid_fmt_recolor(buf, sizeof(buf), CTRL.PIDVout.Ki, "%.1f", s_pid_cursor);
        lv_label_set_recolor(s_lbl_ki_val, true);
        lv_obj_set_style_text_color(s_lbl_ki_val, lv_color_hex(0x3DFFB5), 0);
    } else {
        snprintf(buf, sizeof(buf), "%.1f", CTRL.PIDVout.Ki);
        lv_label_set_recolor(s_lbl_ki_val, false);
        lv_obj_set_style_text_color(s_lbl_ki_val, lv_color_hex(0x1A4A25), 0);
    }
    lv_label_set_text(s_lbl_ki_val, buf);
    if (s_lbl_ki_cur) lv_label_set_text(s_lbl_ki_cur, "");
    /* Kd */
    if (s_pid_sel == 2) {
        pid_fmt_recolor(buf, sizeof(buf), CTRL.PIDVout.Kd, "%.3f", s_pid_cursor);
        lv_label_set_recolor(s_lbl_kd_val, true);
        lv_obj_set_style_text_color(s_lbl_kd_val, lv_color_hex(0xC77DFF), 0);
    } else {
        snprintf(buf, sizeof(buf), "%.3f", CTRL.PIDVout.Kd);
        lv_label_set_recolor(s_lbl_kd_val, false);
        lv_obj_set_style_text_color(s_lbl_kd_val, lv_color_hex(0x6B3A1A), 0);
    }
    lv_label_set_text(s_lbl_kd_val, buf);
    if (s_lbl_kd_cur) lv_label_set_text(s_lbl_kd_cur, "");
}


void ui_pid_adjust(int8_t dir)
{
    if (s_page != UI_PAGE_PID) return;
    /* Step table: Kp/Ki cursor 0..4 -> 1000,100,10,1,0.1 */
    /* Kd cursor 0..3 -> 1,0.1,0.01,0.001 */
    /* Kp: 4 digits 100,10,1,0.1 | Ki: 5 digits 1000,100,10,1,0.1 | Kd: 4 digits */
    static const float kp_steps[5] = {0.1f,1.0f,10.0f,100.0f,1000.0f};
    static const float ki_steps[6] = {0.1f,1.0f,10.0f,100.0f,1000.0f,10000.0f};
    static const float kd_steps[5] = {0.001f,0.01f,0.1f,1.0f,10.0f};
    float step;
    if (s_pid_sel == 0) {
        uint8_t c = s_pid_cursor < 5 ? s_pid_cursor : 4;
        step = kp_steps[c];
        CTRL.PIDVout.Kp += dir * step;
    } else if (s_pid_sel == 1) {
        uint8_t c = s_pid_cursor < 6 ? s_pid_cursor : 5;
        step = ki_steps[c];
        CTRL.PIDVout.Ki += dir * step;
    } else {
        uint8_t c = s_pid_cursor < 5 ? s_pid_cursor : 4;
        step = kd_steps[c];
        CTRL.PIDVout.Kd += dir * step;
    }
    /* Clamp and wrap (handled by pid_page_refresh) */
    pid_page_refresh();
    Config_Save(); /* persist PID changes to Flash */
}

void ui_pid_next_param(void)
{
    if (s_page != UI_PAGE_PID) return;
    s_pid_sel = (s_pid_sel + 1) % 3;
    s_pid_cursor = 0; /* reset to lowest decimal (rightmost) */
    pid_page_refresh();
}

/* ENC_KEY short press in PID page: move selection from right to left, wrap to lowest decimal */
void ui_pid_cursor_move(void)
{
    if (s_page != UI_PAGE_PID) return;
    /* Kp: 0..9999 = 4 digits (0..3); Ki: 0..99999 = 5 digits (0..4); Kd: 0..99.9 = 3 digits (0..2) */
    uint8_t maxcur = (s_pid_sel==0) ? 4 : (s_pid_sel==1) ? 5 : 4;  /* Kp:0-4, Ki:0-5, Kd:0-4 */
    s_pid_cursor = (s_pid_cursor < maxcur) ? (s_pid_cursor + 1) : 0;
    pid_page_refresh();
}

static void screen_page_refresh(void)
{
    if (!s_lbl_brightness) return;
    char buf[20];
    lv_snprintf(buf, sizeof(buf), "Brightness: %d%%", s_brightness);
    lv_label_set_text(s_lbl_brightness, buf);
    /* Apply brightness via TIM3 PWM */
    extern TIM_HandleTypeDef htim3;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(&htim3);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (arr * s_brightness) / 100U);
}

void ui_screen_enc(int8_t dir)
{
    int16_t v = (int16_t)s_brightness + dir * 5;
    if (v < 5)  v = 5;
    if (v > 100) v = 100;
    s_brightness = (uint8_t)v;
    screen_page_refresh();
}

void ui_subpage_back(void)
{
    if (s_page <= UI_PAGE_MENU || s_page > UI_PAGE_ABOUT) return; /* valid sub-pages */
    lv_screen_load(s_scr_menu);
    s_page = UI_PAGE_MENU;
    menu_items_refresh();
}




/* ================================================================

 * Cursor editor API (called from KEY/Encoder handlers)

 * ================================================================ */



/** Toggle adjust mode on/off */

void ui_vout_toggle_adjust(void)

{

    /* Update PID sub-page values periodically */
    if (s_page == UI_PAGE_PID) { pid_page_refresh(); return; }
    if (s_page != UI_PAGE_HOME) return;

    s_adj_mode = !s_adj_mode;

    if (s_adj_mode) {

        output_ref_to_digits();       /* sync from current CV/CC target */

        s_cursor = 1;               /* default: ones position */

        lv_label_set_text(s_lbl_adj_hint, "ADJ");

    } else {

        /* Commit: write digits back to current CV/CC target */

        if (s_ctrl_cc) {

            CTRL.IoutLimit = digits_to_vout_ref();

            if (CTRL.IoutLimit < 0.001f) CTRL.IoutLimit = 0.001f;

        } else {

            CTRL.VoutRef = digits_to_vout_ref();

            CTRL_SetVoutRef(CTRL.VoutRef);

        }

        Config_Save(); /* persist current output target */

        lv_label_set_text(s_lbl_adj_hint, "");

    }

    vout_digits_refresh();

}


/** Move cursor left (KEY2) */

void ui_vout_cursor_left(void)

{

    if (!s_adj_mode || s_page != UI_PAGE_HOME) return;

    if (s_cursor > 0) s_cursor--;

    vout_digits_refresh();

}



/** Move cursor right (KEY3) */

void ui_vout_cursor_right(void)

{

    if (!s_adj_mode || s_page != UI_PAGE_HOME) return;

    if (s_cursor < 4) s_cursor++;

    vout_digits_refresh();

}



/** Increment digit at cursor (encoder right) */

void ui_vout_digit_inc(void)

{

    if (!s_adj_mode || s_page != UI_PAGE_HOME) return;

    output_ref_step(1);

    vout_digits_refresh();

}


/** Decrement digit at cursor (encoder left) */

void ui_vout_digit_dec(void)

{

    if (!s_adj_mode || s_page != UI_PAGE_HOME) return;

    output_ref_step(-1);

    vout_digits_refresh();

}


/* ================================================================

 * ui_main_update - called every 200ms

 * ================================================================ */

void ui_main_update(void)

{

    char buf[16];

    if (s_page != UI_PAGE_HOME) return;



    /* Vin: fixed 2 int + 3 dec */

    snprintf(buf, sizeof(buf), "%05.3fV", ADCSAP.Conv.VoltageIn);

    lv_label_set_text(s_lbl_vin, buf);



    /* Vout actual measured */

    snprintf(buf, sizeof(buf), "%05.3fV", ADCSAP.Conv.VoltageOut);

    lv_label_set_text(s_lbl_vout_actual, buf);



    /* Iin */

    snprintf(buf, sizeof(buf), "%05.3fA", ADCSAP.Conv.CurrentIn);

    lv_label_set_text(s_lbl_iin, buf);



    /* Iout */

    snprintf(buf, sizeof(buf), "%05.3fA", ADCSAP.Conv.CurrentOut);

    lv_label_set_text(s_lbl_iout, buf);



    /* Temperatures */

    snprintf(buf, sizeof(buf), "%05.2fC", ADCSAP.Conv.TempMcu);

    lv_label_set_text(s_lbl_mtemp, buf);

    snprintf(buf, sizeof(buf), "%05.2fC", BD.TMP.BoardTemp);

    lv_label_set_text(s_lbl_btemp, buf);



    /* Mode */

    const char *ms; uint32_t mc;

    switch (CTRL.Mode) {

        case CTRL_MODE_BUCK:  ms="BUCK";  mc=0x0000FF; break;

        case CTRL_MODE_BOOST: ms="BOOST"; mc=0x0000FF; break;

        case CTRL_MODE_BB:    ms="BB";    mc=0x0000FF; break;

        default:              ms="OFF";   mc=0xFFE4E1; break;

    }

    lv_label_set_text(s_lbl_mode, ms);

    lv_obj_set_style_text_color(s_lbl_mode, lv_color_hex(mc), 0);



    /* State */

    if (CTRL.HRTIMActive) {

        lv_label_set_text(s_lbl_status, "RUN");

        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0x00FF00), 0);

    } else {

        lv_label_set_text(s_lbl_status, "STOP");

        lv_obj_set_style_text_color(s_lbl_status, lv_color_hex(0xFF0000), 0);

    }



    /* Vout setpoint digits - only refresh when NOT in adjust mode

       (in adjust mode, digits are updated by encoder/key handlers) */

    if (!s_adj_mode) {

        vout_ref_to_digits();

        vout_digits_refresh();

    }

}





/* ================================================================

 * pid_page_create - PID parameter editing sub-page

 * ================================================================ */

static void pid_page_create(void)

{

    s_scr_pid = ui_screen();

    ui_titlebar(s_scr_pid, "PID Params", 0x58A6FF);



    lv_obj_t *l;

    /* Kp row */

    l = lv_label_create(s_scr_pid);

    lv_label_set_text(l, "Kp:");

    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);

    lv_obj_set_style_text_color(l, lv_color_hex(0xCDD6F4), 0);

    lv_obj_set_pos(l, 20, 46);

    s_lbl_kp_val = lv_label_create(s_scr_pid);

    lv_obj_set_style_text_font(s_lbl_kp_val, &lv_font_montserrat_20, 0);

    lv_obj_set_pos(s_lbl_kp_val, 100, 46);

    s_lbl_kp_cur = lv_label_create(s_scr_pid);

    lv_obj_set_style_text_font(s_lbl_kp_cur, &lv_font_montserrat_20, 0);

    lv_obj_set_style_text_color(s_lbl_kp_cur, lv_color_hex(0xF7C948), 0);

    lv_obj_set_pos(s_lbl_kp_cur, 100, 68);

    lv_label_set_text(s_lbl_kp_cur, "");

    /* Ki row */

    l = lv_label_create(s_scr_pid);

    lv_label_set_text(l, "Ki:");

    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);

    lv_obj_set_style_text_color(l, lv_color_hex(0xCDD6F4), 0);

    lv_obj_set_pos(l, 20, 106);

    s_lbl_ki_val = lv_label_create(s_scr_pid);

    lv_obj_set_style_text_font(s_lbl_ki_val, &lv_font_montserrat_20, 0);

    lv_obj_set_pos(s_lbl_ki_val, 100, 106);

    s_lbl_ki_cur = lv_label_create(s_scr_pid);

    lv_obj_set_style_text_font(s_lbl_ki_cur, &lv_font_montserrat_20, 0);

    lv_obj_set_style_text_color(s_lbl_ki_cur, lv_color_hex(0xF7C948), 0);

    lv_obj_set_pos(s_lbl_ki_cur, 100, 128);

    lv_label_set_text(s_lbl_ki_cur, "");

    /* Kd row */

    l = lv_label_create(s_scr_pid);

    lv_label_set_text(l, "Kd:");

    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);

    lv_obj_set_style_text_color(l, lv_color_hex(0xCDD6F4), 0);

    lv_obj_set_pos(l, 20, 166);

    s_lbl_kd_val = lv_label_create(s_scr_pid);

    lv_obj_set_style_text_font(s_lbl_kd_val, &lv_font_montserrat_20, 0);

    lv_obj_set_pos(s_lbl_kd_val, 100, 166);

    s_lbl_kd_cur = lv_label_create(s_scr_pid);

    lv_obj_set_style_text_font(s_lbl_kd_cur, &lv_font_montserrat_20, 0);

    lv_obj_set_style_text_color(s_lbl_kd_cur, lv_color_hex(0xF7C948), 0);

    lv_obj_set_pos(s_lbl_kd_cur, 100, 188);

    lv_label_set_text(s_lbl_kd_cur, "");

    ui_sep(s_scr_pid, 210);

    l = lv_label_create(s_scr_pid);

    lv_label_set_text(l, "[ENC]Adj [KEY]Cur [K3]Next [K2]Back");

    lv_obj_set_style_text_color(l, lv_color_hex(0x30363D), 0);

    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);

    lv_obj_set_pos(l, 4, 220);

    pid_page_refresh();

}



/* ================================================================

 * Navigation functions

 * ================================================================ */

/* ================================================================
 * Work Mode page: AUTO/BUCK/BOOST/BB manual selection
 * ================================================================ */
static lv_obj_t *s_lbl_work[4];   /* AUTO, BUCK, BOOST, BB */
static uint8_t   s_work_sel = 0;  /* 0=AUTO 1=BUCK 2=BOOST 3=BB */
static const char *s_work_labels[4] = {
    "  AUTO  (auto select)",
    "  BUCK  (Vin > Vout)",
    "  BOOST (Vin < Vout)",
    "  BB    (Vin ~ Vout)",
};

static void work_page_refresh(void)
{
    uint8_t i;
    for (i = 0; i < 4; i++) {
        if (!s_lbl_work[i]) continue;
        lv_obj_set_style_text_color(s_lbl_work[i],
            (i == s_work_sel) ? lv_color_hex(0xF7C948) : lv_color_hex(0x8B949E), 0);
    }
}

static void work_page_create(void)
{
    s_scr_work = ui_screen();
    ui_titlebar(s_scr_work, "Work Mode", 0xFFA657);
    uint8_t i;
    for (i = 0; i < 4; i++) {
        s_lbl_work[i] = lv_label_create(s_scr_work);
        lv_label_set_text(s_lbl_work[i], s_work_labels[i]);
        lv_obj_set_style_text_font(s_lbl_work[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(s_lbl_work[i], 16, 50 + i * 44);
    }
    ui_sep(s_scr_work, 230);
    lv_obj_t *l = lv_label_create(s_scr_work);
    lv_label_set_text(l, "[ENC]Select  [KEY]Confirm  [K2]Back");
    lv_obj_set_style_text_color(l, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(l, 4, 244);
    /* init selection from current mode */
    s_work_sel = 0; /* default AUTO */
    work_page_refresh();
}

void ui_work_toggle(int8_t dir)
{
    if (s_page != UI_PAGE_MODE) return;
    if (dir > 0) { if (s_work_sel < 3) s_work_sel++; }
    else         { if (s_work_sel > 0) s_work_sel--; }
    work_page_refresh();
}

void ui_work_confirm(void)
{
    if (s_page != UI_PAGE_MODE) return;
    switch (s_work_sel) {
        case 0: CTRL_SetMode(CTRL_MODE_OFF); break; /* AUTO: let CTRL_Run select */
        case 1: CTRL_SetMode(CTRL_MODE_BUCK);  break;
        case 2: CTRL_SetMode(CTRL_MODE_BOOST); break;
        case 3: CTRL_SetMode(CTRL_MODE_BB);    break;
    }
    /* AUTO mode: CTRL_Run will call CTRL_AutoSelectMode each cycle */
    ui_subpage_back();
}

/* ================================================================
 * CV/CC Output Mode page
 * ================================================================ */
static lv_obj_t *s_lbl_cvcc_cv = NULL;
static lv_obj_t *s_lbl_cvcc_cc = NULL;
static uint8_t   s_cvcc_sel    = 0;

static void cvcc_page_refresh(void)
{
    char buf[40];
    if (!s_lbl_cvcc_cv) return;
    lv_snprintf(buf, sizeof(buf), "  CV  %.3fV", CTRL.VoutRef);
    lv_label_set_text(s_lbl_cvcc_cv, buf);
    lv_snprintf(buf, sizeof(buf), "  CC  %.3fA", CTRL.IoutLimit);
    lv_label_set_text(s_lbl_cvcc_cc, buf);
    lv_obj_set_style_text_color(s_lbl_cvcc_cv,
        s_cvcc_sel==0?lv_color_hex(0xF7C948):lv_color_hex(0x4A5568), 0);
    lv_obj_set_style_text_color(s_lbl_cvcc_cc,
        s_cvcc_sel==1?lv_color_hex(0xF7C948):lv_color_hex(0x4A5568), 0);
}

static void cvcc_page_create(void)
{
    s_scr_mode = ui_screen();
    ui_titlebar(s_scr_mode, "Output Mode", 0xF0883E);
    lv_obj_t *l;
    s_lbl_cvcc_cv = lv_label_create(s_scr_mode);
    lv_label_set_text(s_lbl_cvcc_cv, "  CV  0.000V");
    lv_obj_set_style_text_font(s_lbl_cvcc_cv, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(s_lbl_cvcc_cv, 16, 70);
    s_lbl_cvcc_cc = lv_label_create(s_scr_mode);
    lv_label_set_text(s_lbl_cvcc_cc, "  CC  0.000A");
    lv_obj_set_style_text_font(s_lbl_cvcc_cc, &lv_font_montserrat_16, 0);
    lv_obj_set_pos(s_lbl_cvcc_cc, 16, 120);
    ui_sep(s_scr_mode, 180);
    l = lv_label_create(s_scr_mode);
    lv_label_set_text(l, "[ENC]Toggle  [KEY]Confirm  [K2]Back");
    lv_obj_set_style_text_color(l, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(l, 4, 190);
    s_cvcc_sel = s_ctrl_cc;
    cvcc_page_refresh();
}

void ui_cvcc_confirm(void)
{
    ui_set_ctrl_cc(s_cvcc_sel);
    Config_Save();
    ui_goto_home();
}

void ui_cvcc_toggle(int8_t dir)
{
    (void)dir;
    if (s_page != UI_PAGE_CVCC) return;
    s_cvcc_sel = s_cvcc_sel ? 0 : 1;
    cvcc_page_refresh();
}

/* ================================================================
 * Language page (reserved, Chinese only)
 * ================================================================ */
static void lang_page_create(void)
{
    s_scr_lang = ui_screen();
    ui_titlebar(s_scr_lang, "Language", 0x3FB950);
    lv_obj_t *l;
    l = lv_label_create(s_scr_lang);
    lv_label_set_text(l, "Chinese (Simplified)");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xF7C948), 0);
    lv_obj_set_pos(l, 16, 80);
    l = lv_label_create(s_scr_lang);
    lv_label_set_text(l, "(More languages coming soon)");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x8B949E), 0);
    lv_obj_set_pos(l, 16, 110);
    ui_sep(s_scr_lang, 200);
    l = lv_label_create(s_scr_lang);
    lv_label_set_text(l, "[K2] Back");
    lv_obj_set_style_text_color(l, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(l, 4, 210);
}

/* ================================================================
 * About page
 * ================================================================ */
static void screen_page_create(void)
{
    s_scr_screen = ui_screen();
    ui_titlebar(s_scr_screen, "Screen", 0x58A6FF);
    lv_obj_t *lbr_tag = lv_label_create(s_scr_screen);
    lv_label_set_text(lbr_tag, "Backlight");
    lv_obj_set_style_text_color(lbr_tag, lv_color_hex(0x79C0FF), 0);
    lv_obj_set_style_text_font(lbr_tag, &lv_font_montserrat_14, 0);
    lv_obj_align(lbr_tag, LV_ALIGN_TOP_MID, 0, 50);
    s_lbl_brightness = lv_label_create(s_scr_screen);
    lv_obj_set_style_text_color(s_lbl_brightness, lv_color_hex(0xF7C948), 0);
    lv_obj_set_style_text_font(s_lbl_brightness, &lv_font_montserrat_20, 0);
    lv_obj_align(s_lbl_brightness, LV_ALIGN_TOP_MID, 0, 70);
    lv_obj_t *lhint = lv_label_create(s_scr_screen);
    lv_label_set_text(lhint, "[ENC] Adjust  [K2] Back");
    lv_obj_set_style_text_color(lhint, lv_color_hex(0x6E7681), 0);
    lv_obj_set_style_text_font(lhint, &lv_font_montserrat_12, 0);
    lv_obj_align(lhint, LV_ALIGN_TOP_MID, 0, 248);
    screen_page_refresh();
}

static void about_page_create(void)
{
    s_scr_about = ui_screen();
    ui_titlebar(s_scr_about, "About", 0x8B949E);
    lv_obj_t *l;
    l = lv_label_create(s_scr_about);
    lv_label_set_text(l, "STM32G474 BUCK-BOOST");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x58A6FF), 0);
    lv_obj_set_pos(l, 16, 50);
    l = lv_label_create(s_scr_about);
    lv_label_set_text(l, "HW: V1.0  SW: V1.1");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_pos(l, 16, 80);
    l = lv_label_create(s_scr_about);
    lv_label_set_text(l, "Author: Robot");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0xCDD6F4), 0);
    lv_obj_set_pos(l, 16, 105);
    l = lv_label_create(s_scr_about);
    lv_label_set_text(l, "HRTIM: 200kHz  Ts: 10ms");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x8B949E), 0);
    lv_obj_set_pos(l, 16, 135);
    l = lv_label_create(s_scr_about);
    lv_label_set_text(l, "OCP: 1.5x limit  Slew: 50V/s");
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(0x8B949E), 0);
    lv_obj_set_pos(l, 16, 155);
    ui_sep(s_scr_about, 200);
    l = lv_label_create(s_scr_about);
    lv_label_set_text(l, "[K2] Back");
    lv_obj_set_style_text_color(l, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(l, 4, 210);
}

/* ================================================================
 * FAN Control page: AUTO (temp-based) or Manual (0-100%)
 * ================================================================ */
static lv_obj_t *s_scr_fan     = NULL;
static lv_obj_t *s_lbl_fan_mode[2]; /* AUTO / Manual */
static lv_obj_t *s_lbl_fan_spd = NULL;
static lv_obj_t *s_lbl_fan_hint= NULL;
uint8_t  g_fan_auto  = 1;   /* 1=AUTO, 0=Manual */
uint8_t  g_fan_speed = 50;  /* Manual speed 0-100 (%) */
static uint8_t s_fan_editing = 0; /* 0=mode sel, 1=speed edit */

static void fan_page_refresh(void)
{
    char buf[16];
    if (!s_lbl_fan_mode[0]) return;
    lv_obj_set_style_text_color(s_lbl_fan_mode[0],
        (g_fan_auto==1)?lv_color_hex(0xF7C948):lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_color(s_lbl_fan_mode[1],
        (g_fan_auto==0)?lv_color_hex(0xF7C948):lv_color_hex(0x8B949E), 0);
    if (g_fan_auto) {
        lv_label_set_text(s_lbl_fan_spd, "Speed: AUTO");
        lv_obj_set_style_text_color(s_lbl_fan_spd, lv_color_hex(0x3FB950), 0);
    } else {
        snprintf(buf, sizeof(buf), "Speed: %3d%%", g_fan_speed);
        lv_label_set_text(s_lbl_fan_spd, buf);
        lv_obj_set_style_text_color(s_lbl_fan_spd,
            s_fan_editing?lv_color_hex(0x00FFFF):lv_color_hex(0xCDD6F4), 0);
    }
    if (s_fan_editing && !g_fan_auto)
        lv_label_set_text(s_lbl_fan_hint, "[ENC]Adjust  [KEY]Done  [K2]Back");
    else
        lv_label_set_text(s_lbl_fan_hint, "[ENC]Mode  [KEY]Edit Spd  [K2]Back");
    /* Apply FAN setting immediately */
    if (g_fan_auto) {
        BD.FAN.TempAdjSpeed_Fan();
    } else {
        BD.FAN.Status = (g_fan_speed > 0) ? ON : OFF;
        BD.FAN.Speed  = (uint16_t)(g_fan_speed * 10); /* 0-100 -> 0-1000 */
        BD.FAN.SetSpeed_Fan(BD.FAN.Speed);
    }
}

static void fan_page_create(void)
{
    s_scr_fan = ui_screen();
    ui_titlebar(s_scr_fan, "Fan Control", 0x79C0FF);
    lv_obj_t *l;
    /* Mode labels */
    s_lbl_fan_mode[0] = lv_label_create(s_scr_fan);
    lv_label_set_text(s_lbl_fan_mode[0], "  AUTO  (temp-based)");
    lv_obj_set_style_text_font(s_lbl_fan_mode[0], &lv_font_montserrat_16, 0);
    lv_obj_set_pos(s_lbl_fan_mode[0], 16, 50);
    s_lbl_fan_mode[1] = lv_label_create(s_scr_fan);
    lv_label_set_text(s_lbl_fan_mode[1], "  Manual (0-100%)");
    lv_obj_set_style_text_font(s_lbl_fan_mode[1], &lv_font_montserrat_16, 0);
    lv_obj_set_pos(s_lbl_fan_mode[1], 16, 90);
    /* Speed display */
    s_lbl_fan_spd = lv_label_create(s_scr_fan);
    lv_obj_set_style_text_font(s_lbl_fan_spd, &lv_font_montserrat_20, 0);
    lv_obj_set_pos(s_lbl_fan_spd, 16, 140);
    ui_sep(s_scr_fan, 200);
    s_lbl_fan_hint = lv_label_create(s_scr_fan);
    lv_obj_set_style_text_color(s_lbl_fan_hint, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(s_lbl_fan_hint, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(s_lbl_fan_hint, 4, 210);
    s_fan_editing = 0;
    fan_page_refresh();
}

void ui_fan_enc(int8_t dir)
{
    if (s_page != UI_PAGE_FAN) return;
    if (s_fan_editing) {
        /* Adjust speed 0-100 */
        int16_t spd = (int16_t)g_fan_speed + dir;
        if (spd < 0)   spd = 0;
        if (spd > 100) spd = 100;
        g_fan_speed = (uint8_t)spd;
    } else {
        /* Toggle AUTO/Manual */
        g_fan_auto = g_fan_auto ? 0 : 1;
    }
    fan_page_refresh();
}

void ui_fan_key(void)
{
    if (s_page != UI_PAGE_FAN) return;
    if (!g_fan_auto) s_fan_editing = !s_fan_editing;
    fan_page_refresh();
}

void ui_goto_home(void)

{

    lv_screen_load(s_scr_home);

    s_page = UI_PAGE_HOME;

    vout_ref_to_digits();

    vout_digits_refresh();

}



void ui_goto_menu(void)

{

    lv_screen_load(s_scr_menu);

    s_page = UI_PAGE_MENU;

    menu_items_refresh();

}



void ui_menu_next(void)

{

    if (s_page != UI_PAGE_MENU) return;

    if (s_menu_sel < MENU_COUNT-1) s_menu_sel++;

    menu_items_refresh();

}



void ui_menu_prev(void)

{

    if (s_page != UI_PAGE_MENU) return;

    if (s_menu_sel > 0) s_menu_sel--;

    menu_items_refresh();

}



void ui_menu_enter(void)

{

    if (s_page != UI_PAGE_MENU) return;

    switch (s_menu_sel) {

        case 0: /* Output Mode -> CV/CC sub-page */

            if (!s_scr_mode) cvcc_page_create(); if (s_scr_mode) { lv_screen_load(s_scr_mode); s_page = UI_PAGE_CVCC; s_cvcc_sel = s_ctrl_cc; cvcc_page_refresh(); }

            break;

        case 1: /* PID Params */

            if (!s_scr_pid) pid_page_create();

            s_pid_sel = 0; s_pid_cursor = 0;

            lv_screen_load(s_scr_pid);

            s_page = UI_PAGE_PID;

            pid_page_refresh();

            break;

        case 2: /* Work Mode sub-page */

            if (!s_scr_work) work_page_create(); if (s_scr_work) { lv_screen_load(s_scr_work); s_page = UI_PAGE_MODE; s_work_sel=0; work_page_refresh(); }

            break;

        case 3: /* Fan Control */

            if (!s_scr_fan) fan_page_create(); if (s_scr_fan) { lv_screen_load(s_scr_fan); s_page = UI_PAGE_FAN; s_fan_editing=0; fan_page_refresh(); }

            break;

        case 4: /* Screen Settings */

            if (!s_scr_screen) screen_page_create(); if (s_scr_screen) { lv_screen_load(s_scr_screen); s_page = UI_PAGE_SCREEN; screen_page_refresh(); }

            break;

        case 5: /* Language - penultimate fixed */

            if (!s_scr_lang) lang_page_create(); if (s_scr_lang) { lv_screen_load(s_scr_lang); s_page = UI_PAGE_LANG; }

            break;

        case 6: /* About - last fixed */

            if (!s_scr_about) about_page_create(); if (s_scr_about) { lv_screen_load(s_scr_about); s_page = UI_PAGE_ABOUT; }

            break;

        default: break;

    }

}



/* ================================================================

 * ui_main_init - create all pages

 * ================================================================ */

void ui_splash_set_progress(uint8_t pct, const char *msg)
{
    if (s_bar_prog) lv_bar_set_value(s_bar_prog, pct, LV_ANIM_OFF);
    if (s_lbl_pct)  { char buf[8]; snprintf(buf,sizeof(buf),"%d%%",pct); lv_label_set_text(s_lbl_pct, buf); }
    if (s_lbl_step && msg) lv_label_set_text(s_lbl_step, msg);
}

void ui_main_init(void)

{

    start_page_create();
    home_page_create();
    menu_page_create();
    /* Remaining pages created on first entry (lazy init) */
    /* pid / cvcc / work / fan / screen / lang / about */
    s_scr_pid    = NULL;
    s_scr_screen = NULL;

    lv_screen_load(s_scr_start);

    s_page = UI_PAGE_START;

}



uint8_t ui_get_page(void)

{

    return (uint8_t)s_page;

}

uint8_t ui_get_ctrl_cc(void) { return s_ctrl_cc; }
void    ui_set_ctrl_cc(uint8_t cc) {
    s_ctrl_cc = cc & 1;
    if (s_lbl_cvcc) {
        lv_label_set_text(s_lbl_cvcc, s_ctrl_cc ? "CC" : "CV");
        lv_obj_set_style_text_color(s_lbl_cvcc,
            s_ctrl_cc ? lv_color_hex(0xFF1493) : lv_color_hex(0x00FFFF), 0);
    }
    if (s_lbl_vout_unit) {
        lv_label_set_text(s_lbl_vout_unit, s_ctrl_cc ? "A" : "V");
        lv_obj_set_style_text_color(s_lbl_vout_unit,
            lv_color_hex(s_ctrl_cc ? 0xFF1493 : 0x40C9FF), 0);
    }
    output_ref_to_digits();
    vout_digits_refresh();
}
