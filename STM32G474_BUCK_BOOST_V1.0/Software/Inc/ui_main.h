#ifndef UI_MAIN_H
#define UI_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include <stdint.h>

/* Page IDs returned by ui_get_page() */
#define UI_PAGE_ID_START  0
#define UI_PAGE_ID_HOME   1
#define UI_PAGE_ID_MENU   2
#define UI_PAGE_ID_PID    3   /* PID Params sub-page */
#define UI_PAGE_ID_MODE   4   /* Work Mode sub-page */
#define UI_PAGE_ID_LANG   5   /* Language sub-page */
#define UI_PAGE_ID_SCREEN 7   /* Screen Settings sub-page */
#define UI_PAGE_ID_ABOUT  8   /* About sub-page */

/* Page enum (mirrors internal ui_page_t) */
typedef enum {
    UI_PAGE_START = 0,
    UI_PAGE_HOME,
    UI_PAGE_MENU,
    UI_PAGE_PID,
    UI_PAGE_MODE,   /* Work Mode: BUCK/BOOST/BB/AUTO */
    UI_PAGE_CVCC,   /* Output Mode: CV/CC */
    UI_PAGE_LANG,
    UI_PAGE_FAN,    /* Fan Control */
    UI_PAGE_SCREEN, /* Screen Settings */
    UI_PAGE_ABOUT
} ui_page_id_t;

/* ----------------------------------------------------------------
 * Lifecycle
 * ---------------------------------------------------------------- */
void    ui_main_init(void);
void    ui_main_update(void);

/* ----------------------------------------------------------------
 * START page progress
 * ---------------------------------------------------------------- */
void    ui_splash_set_progress(uint8_t pct, const char *step);

/* ----------------------------------------------------------------
 * Page navigation
 * ---------------------------------------------------------------- */
void    ui_goto_home(void);
void    ui_goto_menu(void);
uint8_t ui_get_page(void);
uint8_t ui_get_ctrl_cc(void);
void    ui_set_ctrl_cc(uint8_t cc);
uint8_t ui_get_output_mode(void);
void    ui_set_output_mode(uint8_t mode);

/* ----------------------------------------------------------------
 * Vout cursor editor (call from KEY / Encoder handlers)
 * ---------------------------------------------------------------- */

/** ENC_KEY pressed: toggle adjust mode on/off */
void    ui_vout_toggle_adjust(void);

/** KEY2: move cursor left */
void    ui_vout_cursor_left(void);

/** KEY3: move cursor right */
void    ui_vout_cursor_right(void);

/** Encoder CW (right): increment digit at cursor */
void    ui_vout_digit_inc(void);

/** Encoder CCW (left): decrement digit at cursor */
void    ui_vout_digit_dec(void);

/* ----------------------------------------------------------------
 * Menu navigation (call from KEY / Encoder handlers)
 * ---------------------------------------------------------------- */

/** Encoder CW in MENU page: move selection down */
void    ui_menu_next(void);

/** Encoder CCW in MENU page: move selection up */
void    ui_menu_prev(void);

/** ENC_KEY in MENU page: enter selected item */
void    ui_menu_enter(void);

/** PID page: encoder adjusts selected param, dir=+1 or -1 */
void    ui_pid_adjust(int8_t dir);

/** PID page: KEY2 switches to next param (Kp->Ki->Kd->Kp) */
void    ui_pid_next_param(void);

/** PID page: ENC_KEY short press moves cursor to next digit */
void    ui_pid_cursor_move(void);

/** CV/CC mode page: encoder toggle selection */
void    ui_cvcc_toggle(int8_t dir);

/** CV/CC mode page: ENC_KEY confirm selection */
void    ui_cvcc_confirm(void);

/** Work Mode page: encoder move selection */
void    ui_work_toggle(int8_t dir);

/** Work Mode page: ENC_KEY confirm selection */
void    ui_work_confirm(void);

/** Fan page: encoder adjusts mode or speed */
void    ui_fan_enc(int8_t dir);

/** Fan page: ENC_KEY toggles speed edit mode */
void    ui_fan_key(void);

/** Fan global state (used by Config_Save) */
extern uint8_t g_fan_auto;
extern uint8_t g_fan_speed;

/** Screen Settings page: encoder adjusts brightness */
void    ui_screen_enc(int8_t dir);

/** Any sub-page: KEY2 back to MENU */
void    ui_subpage_back(void);

/** Splash screen: update progress bar and status text */
void    ui_splash_set_progress(uint8_t pct, const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* UI_MAIN_H */
