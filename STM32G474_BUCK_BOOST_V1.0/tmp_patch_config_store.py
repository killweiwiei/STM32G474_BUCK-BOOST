from pathlib import Path
p = Path(r"E:\User_Data\Project\BUCK_BOOST\STM32G474-BUCK_BOOST_V1.0\STM32G474_BUCK_BOOST_V1.0\Software\Src\config_store.c")
t = p.read_text(encoding="utf-8", errors="ignore")
t = t.replace("""    /* Default output state is always OFF at power-up.
     * Do not restore work mode into CTRL.Mode here; mode will be
     * re-evaluated on KEY1/CTRL_Start using current Vref and Vin. */
    CTRL.Mode = CTRL_MODE_OFF;
""", """    /* Apply work strategy */
    CTRL.WorkMode = (rec.payload.work_mode == 1U) ? CTRL_WORK_MODE_BB : CTRL_WORK_MODE_AUTO;

    /* Default output state is always OFF at power-up.
     * Actual run mode will be re-evaluated on KEY1/CTRL_Start. */
    CTRL.Mode = CTRL_MODE_OFF;
""")
t = t.replace("""    /* Work Mode: map CTRL.Mode to index */
    switch (CTRL.Mode) {
        case CTRL_MODE_BUCK:  rec.payload.work_mode = 1; break;
        case CTRL_MODE_BOOST: rec.payload.work_mode = 2; break;
        case CTRL_MODE_BB:    rec.payload.work_mode = 3; break;
        default:              rec.payload.work_mode = 0; break; /* AUTO */
    }
""", """    /* Work Mode: 0=AUTO 1=BB */
    rec.payload.work_mode = (CTRL.WorkMode == CTRL_WORK_MODE_BB) ? 1U : 0U;
""")
p.write_text(t, encoding="utf-8")
