from pathlib import Path
p = Path(r"E:\User_Data\Project\BUCK_BOOST\STM32G474-BUCK_BOOST_V1.0\STM32G474_BUCK_BOOST_V1.0\Software\Src\ui_main.c")
t = p.read_text(encoding="utf-8", errors="ignore")
old = '''/* ================================================================
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
'''
new = '''/* ================================================================
 * Work Mode page: AUTO / BB
 * ================================================================ */
static lv_obj_t *s_lbl_work[2];   /* AUTO, BB */
static uint8_t   s_work_sel = 0;  /* 0=AUTO 1=BB */
static const char *s_work_labels[2] = {
    "  AUTO  (auto select)",
    "  BB    (force buck-boost)",
};

static void work_page_refresh(void)
{
    uint8_t i;
    for (i = 0; i < 2; i++) {
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
    for (i = 0; i < 2; i++) {
        s_lbl_work[i] = lv_label_create(s_scr_work);
        lv_label_set_text(s_lbl_work[i], s_work_labels[i]);
        lv_obj_set_style_text_font(s_lbl_work[i], &lv_font_montserrat_16, 0);
        lv_obj_set_pos(s_lbl_work[i], 16, 68 + i * 52);
    }
    ui_sep(s_scr_work, 230);
    lv_obj_t *l = lv_label_create(s_scr_work);
    lv_label_set_text(l, "[ENC]Select  [KEY]Confirm  [K2]Back");
    lv_obj_set_style_text_color(l, lv_color_hex(0x30363D), 0);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_12, 0);
    lv_obj_set_pos(l, 4, 244);
    s_work_sel = (CTRL.WorkMode == CTRL_WORK_MODE_BB) ? 1 : 0;
    work_page_refresh();
}

void ui_work_toggle(int8_t dir)
{
    if (s_page != UI_PAGE_MODE) return;
    if (dir > 0) { if (s_work_sel < 1) s_work_sel++; }
    else         { if (s_work_sel > 0) s_work_sel--; }
    work_page_refresh();
}

void ui_work_confirm(void)
{
    if (s_page != UI_PAGE_MODE) return;
    CTRL_SetWorkMode((s_work_sel == 1) ? CTRL_WORK_MODE_BB : CTRL_WORK_MODE_AUTO);
    Config_Save();
    ui_subpage_back();
}
'''
if old not in t:
    raise SystemExit('ui_main work mode block not found')
t = t.replace(old, new, 1)
p.write_text(t, encoding='utf-8')
