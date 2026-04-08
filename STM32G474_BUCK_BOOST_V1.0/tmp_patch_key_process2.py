from pathlib import Path

p = Path(r"E:\User_Data\Project\BUCK_BOOST\STM32G474-BUCK_BOOST_V1.0\STM32G474_BUCK_BOOST_V1.0\Software\Src\function.c")
t = p.read_text(encoding="utf-8", errors="ignore")
start = t.index("CCMRAM void KEY_Process(uint16_t GPIO_Pin)")
end = t.index("/* ==============================================================", start)
new_block = '''CCMRAM void KEY_Process(uint16_t GPIO_Pin)
{
    switch (GPIO_Pin)
    {
        case ENCODE1_KEY_Pin:
            ui_vout_toggle_adjust();
            break;

        case KEY2_Pin:
            ui_vout_cursor_left();
            break;

        case KEY3_Pin:
            ui_vout_cursor_right();
            break;

        case KEY1_Pin:
            if (CTRL.HRTIMActive) {
                CTRL_Stop();
                printf("KEY1: output OFF\\r\\n");
            } else {
                CTRL_Start();
                printf("KEY1: output ON, auto mode by Vref/Vin (Vref=%.3fV Vin=%.3fV)\\r\\n",
                       CTRL.VoutRef, ADCSAP.Conv.VoltageIn);
            }
            break;

        case KEY4_Pin:
            printf("KEY4 pressed\\r\\n");
            break;

        default: break;
    }
}


'''
t = t[:start] + new_block + t[end:]
p.write_text(t, encoding="utf-8")
