from pathlib import Path
p = Path(r"E:\User_Data\Project\BUCK_BOOST\STM32G474-BUCK_BOOST_V1.0\STM32G474_BUCK_BOOST_V1.0\Software\Inc\config_store.h")
t = p.read_text(encoding="utf-8", errors="ignore")
t = t.replace("work_mode uint32 (0=AUTO,1=BUCK,2=BOOST,3=BB)", "work_mode uint32 (0=AUTO,1=BB)")
t = t.replace("uint32_t work_mode;  /* 0=AUTO 1=BUCK 2=BOOST 3=BB */", "uint32_t work_mode;  /* 0=AUTO 1=BB */")
p.write_text(t, encoding="utf-8")
