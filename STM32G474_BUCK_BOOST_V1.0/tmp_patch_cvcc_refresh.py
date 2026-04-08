from pathlib import Path
p = Path(r"E:\User_Data\Project\BUCK_BOOST\STM32G474-BUCK_BOOST_V1.0\STM32G474_BUCK_BOOST_V1.0\Software\Src\ui_main.c")
t = p.read_text(encoding="utf-8", errors="ignore")
start = t.index("static void cvcc_page_refresh(void)")
end = t.index("static void cvcc_page_create(void)", start)
new_block = '''static void cvcc_page_refresh(void)
{
    char buf[48];
    uint8_t current_cvcc = ui_get_ctrl_cc();
    uint8_t current_out  = ui_get_output_mode();
    if (!s_lbl_cvcc_cv) return;

    lv_snprintf(buf, sizeof(buf), "%sCV  %.3fV",
        (s_cvcc_focus == 0 && s_cvcc_sel == 0) ? "[>] " : ((current_cvcc == 0) ? "[*] " : "[ ] "),
        CTRL.VoutRef);
    lv_label_set_text(s_lbl_cvcc_cv, buf);

    lv_snprintf(buf, sizeof(buf), "%sCC  %.3fA",
        (s_cvcc_focus == 0 && s_cvcc_sel == 1) ? "[>] " : ((current_cvcc == 1) ? "[*] " : "[ ] "),
        CTRL.IoutLimit);
    lv_label_set_text(s_lbl_cvcc_cc, buf);

    lv_snprintf(buf, sizeof(buf), "%sDCM  discontinuous",
        (s_cvcc_focus == 1 && s_output_sel == 0) ? "[>] " : ((current_out == 0) ? "[*] " : "[ ] "));
    lv_label_set_text(s_lbl_cvcc_dcm, buf);

    lv_snprintf(buf, sizeof(buf), "%sCCM  continuous",
        (s_cvcc_focus == 1 && s_output_sel == 1) ? "[>] " : ((current_out == 1) ? "[*] " : "[ ] "));
    lv_label_set_text(s_lbl_cvcc_ccm, buf);

    lv_obj_set_style_text_color(s_lbl_cvcc_cv,
        (s_cvcc_focus == 0 && s_cvcc_sel == 0) ? lv_color_hex(0xF7C948) : ((current_cvcc == 0) ? lv_color_hex(0x22D3EE) : lv_color_hex(0x4A5568)), 0);
    lv_obj_set_style_text_color(s_lbl_cvcc_cc,
        (s_cvcc_focus == 0 && s_cvcc_sel == 1) ? lv_color_hex(0xF7C948) : ((current_cvcc == 1) ? lv_color_hex(0x22D3EE) : lv_color_hex(0x4A5568)), 0);
    lv_obj_set_style_text_color(s_lbl_cvcc_dcm,
        (s_cvcc_focus == 1 && s_output_sel == 0) ? lv_color_hex(0xF7C948) : ((current_out == 0) ? lv_color_hex(0x3FB950) : lv_color_hex(0x4A5568)), 0);
    lv_obj_set_style_text_color(s_lbl_cvcc_ccm,
        (s_cvcc_focus == 1 && s_output_sel == 1) ? lv_color_hex(0xF7C948) : ((current_out == 1) ? lv_color_hex(0x3FB950) : lv_color_hex(0x4A5568)), 0);
}

'''
t = t[:start] + new_block + t[end:]
p.write_text(t, encoding="utf-8")
