/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     config_store.c
* Brief:    Persistent config read/write via W25Qxx SPI Flash
*           Sector 0 (addr 0x000000, 4KB) used as config page.
*================================================================*/
#include "config_store.h"
#include "spiflash.h"
#include "control.h"
#include "ui_main.h"   /* g_fan_auto, g_fan_speed */
#include "function.h"  /* BD.FAN */
#include <string.h>
#include <stdio.h>

/* ----------------------------------------------------------------
 * CRC32 (no lookup table)
 * ---------------------------------------------------------------- */
uint32_t Config_CRC32(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFFUL;
    uint32_t i, b;
    for (i = 0; i < len; i++) {
        crc ^= (uint32_t)data[i] << 24;
        for (b = 0; b < 8; b++)
            crc = (crc << 1) ^ ((crc & 0x80000000UL) ? 0x04C11DB7UL : 0UL);
    }
    return crc ^ 0xFFFFFFFFUL;
}

/* ----------------------------------------------------------------
 * Read 4-byte word from flash at byte address
 * ---------------------------------------------------------------- */
static uint32_t flash_read_u32(uint32_t addr)
{
    uint32_t val = 0;
    val  = (uint32_t)W25QXX_ReadByte(addr + 0) << 24;
    val |= (uint32_t)W25QXX_ReadByte(addr + 1) << 16;
    val |= (uint32_t)W25QXX_ReadByte(addr + 2) <<  8;
    val |= (uint32_t)W25QXX_ReadByte(addr + 3);
    return val;
}

/* ----------------------------------------------------------------
 * Write a block byte-by-byte
 * ---------------------------------------------------------------- */
static void flash_write_bytes(uint32_t addr, const uint8_t *buf, uint32_t len)
{
    uint32_t i;
    for (i = 0; i < len; i++)
        W25QXX_WriteByte(buf[i], addr + i);
}

/* ----------------------------------------------------------------
 * Config_Load
 * ---------------------------------------------------------------- */
uint8_t Config_Load(void)
{
    Config_Record_t rec;
    uint8_t *p = (uint8_t *)&rec;
    uint32_t i;

    /* Read record from flash byte by byte */
    for (i = 0; i < sizeof(Config_Record_t); i++)
        p[i] = W25QXX_ReadByte(CFG_FLASH_ADDR + i);

    /* Validate magic */
    if (rec.magic != CFG_MAGIC) {
        printf("[CFG] No valid config (magic=%08lX)\r\n", rec.magic);
        return 0;
    }

    /* Validate CRC */
    uint32_t crc = Config_CRC32((const uint8_t *)&rec.payload,
                                sizeof(Config_Payload_t));
    if (crc != rec.crc32) {
        printf("[CFG] CRC fail: stored=%08lX calc=%08lX\r\n",
               rec.crc32, crc);
        return 0;
    }

    /* Apply PID + Vref/Ilimit */
    CTRL.PIDVout.Kp = rec.payload.Kp;
    CTRL.PIDVout.Ki = rec.payload.Ki;
    CTRL.PIDVout.Kd = rec.payload.Kd;
    CTRL_SetVoutRef(rec.payload.VoutRef);
    CTRL.IoutLimit = rec.payload.IoutLimit;

    /* Apply FAN settings */
    g_fan_auto  = (uint8_t)(rec.payload.fan_auto  & 0x01);
    g_fan_speed = (uint8_t)(rec.payload.fan_speed <= 100 ?
                            rec.payload.fan_speed : 100);
    if (g_fan_auto) {
        BD.FAN.TempAdjSpeed_Fan();
    } else {
        BD.FAN.Status = (g_fan_speed > 0) ? ON : OFF;
        BD.FAN.Speed  = (uint16_t)(g_fan_speed * 10);
        BD.FAN.SetSpeed_Fan(BD.FAN.Speed);
    }

    /* Apply CV/CC mode */
    ui_set_ctrl_cc((uint8_t)(rec.payload.ctrl_cc & 1));

    /* Default output state is always OFF at power-up.
     * Do not restore work mode into CTRL.Mode here; mode will be
     * re-evaluated on KEY1/CTRL_Start using current Vref and Vin. */
    CTRL.Mode = CTRL_MODE_OFF;

    printf("[CFG] Loaded: Kp=%.1f Ki=%.1f Kd=%.3f Vref=%.3fV Fan=%s(%d%%) Mode=%lu CC=%lu\r\n",
           CTRL.PIDVout.Kp, CTRL.PIDVout.Ki, CTRL.PIDVout.Kd, CTRL.VoutRef,
           g_fan_auto ? "AUTO" : "MAN", g_fan_speed,
           rec.payload.work_mode, rec.payload.ctrl_cc);
    return 1;
}

/* ----------------------------------------------------------------
 * Config_Save
 * ---------------------------------------------------------------- */
void Config_Save(void)
{
    Config_Record_t rec;
    memset(&rec, 0xFF, sizeof(rec));

    /* PID + Vref/Ilimit */
    rec.payload.Kp        = CTRL.PIDVout.Kp;
    rec.payload.Ki        = CTRL.PIDVout.Ki;
    rec.payload.Kd        = CTRL.PIDVout.Kd;
    rec.payload.VoutRef   = CTRL.VoutRef;
    rec.payload.IoutLimit = CTRL.IoutLimit;

    /* FAN */
    rec.payload.fan_auto  = (uint32_t)g_fan_auto;
    rec.payload.fan_speed = (uint32_t)g_fan_speed;

    /* Work Mode: map CTRL.Mode to index */
    switch (CTRL.Mode) {
        case CTRL_MODE_BUCK:  rec.payload.work_mode = 1; break;
        case CTRL_MODE_BOOST: rec.payload.work_mode = 2; break;
        case CTRL_MODE_BB:    rec.payload.work_mode = 3; break;
        default:              rec.payload.work_mode = 0; break; /* AUTO */
    }

    /* CV/CC - read from ui_main extern */
    extern uint8_t s_ctrl_cc;
    rec.payload.ctrl_cc = (uint32_t)ui_get_ctrl_cc();

    /* Compute CRC */
    rec.crc32 = Config_CRC32((const uint8_t *)&rec.payload,
                             sizeof(Config_Payload_t));
    rec.magic = CFG_MAGIC;

    /* Erase sector 0 then write */
    W25QXX_Erase_Sector(CFG_FLASH_ADDR);
    flash_write_bytes(CFG_FLASH_ADDR, (const uint8_t *)&rec,
                      sizeof(Config_Record_t));

    printf("[CFG] Saved: Kp=%.1f Ki=%.1f Kd=%.3f Vref=%.3fV Fan=%s(%d%%) Mode=%lu CC=%lu\r\n",
           CTRL.PIDVout.Kp, CTRL.PIDVout.Ki, CTRL.PIDVout.Kd, CTRL.VoutRef,
           g_fan_auto ? "AUTO" : "MAN", g_fan_speed,
           rec.payload.work_mode, rec.payload.ctrl_cc);
}
