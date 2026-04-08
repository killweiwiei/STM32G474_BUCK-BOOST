from pathlib import Path
p = Path(r"E:\User_Data\Project\BUCK_BOOST\STM32G474-BUCK_BOOST_V1.0\STM32G474_BUCK_BOOST_V1.0\Software\Src\ui_main.c")
t = p.read_text(encoding="utf-8", errors="ignore")
start = t.index("void ui_vout_toggle_adjust(void)")
end = t.index("/** Move cursor left (KEY2) */", start)
new_block = '''void ui_vout_toggle_adjust(void)

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

        /* DCM 模式下确认即停机，避免新设定值先短暂作用到输出。 */
        if ((CTRL.OutputMode == CTRL_OUTPUT_MODE_DCM) && CTRL.HRTIMActive) {
            CTRL_Stop();
        }

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


'''
t = t[:start] + new_block + t[end:]
p.write_text(t, encoding="utf-8")
