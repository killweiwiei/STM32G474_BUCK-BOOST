/******************** (C) COPYRIGHT 2024 Robot ********************
* File:     config_store.h
* Brief:    Persistent configuration storage via SPI Flash W25Qxx
*
* Layout (sector 0, addr 0x000000, 4KB):
*   [0x00~0x03] Magic  = 0xBB000002  (version tag)
*   [0x04~0x07] CRC32  of payload
*   [0x08~0x0B] Kp     float
*   [0x0C~0x0F] Ki     float
*   [0x10~0x13] Kd     float
*   [0x14~0x17] VoutRef float
*   [0x18~0x1B] fan_auto  uint32 (0=Manual, 1=AUTO)
*   [0x1C~0x1F] fan_speed uint32 (0-100 %)
*   [0x20~0x23] work_mode uint32 (0=AUTO,1=BUCK,2=BOOST,3=BB)
*   [0x24~0x27] ctrl_cc   uint32 (0=CV, 1=CC)
*================================================================*/
#ifndef __CONFIG_STORE_H__
#define __CONFIG_STORE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* Flash sector used for config */
#define CFG_FLASH_ADDR      0x000000UL  /* sector 0, byte 0 */
#define CFG_MAGIC           0xBB000003UL  /* bump version when layout changes */

/* Config payload structure (must be multiple of 4 bytes) */
typedef struct __attribute__((packed)) {
    float    Kp;         /* PID proportional gain  */
    float    Ki;         /* PID integral gain      */
    float    Kd;         /* PID derivative gain    */
    float    VoutRef;    /* output voltage setpoint */
    float    IoutLimit;  /* output current setpoint/limit */
    uint32_t fan_auto;   /* 1=AUTO, 0=Manual       */
    uint32_t fan_speed;  /* manual speed 0-100 %   */
    uint32_t work_mode;  /* 0=AUTO 1=BUCK 2=BOOST 3=BB */
    uint32_t ctrl_cc;    /* 0=CV, 1=CC             */
} Config_Payload_t;

/* Full NVM record stored in flash */
typedef struct __attribute__((packed)) {
    uint32_t         magic;    /* CFG_MAGIC */
    uint32_t         crc32;    /* CRC32 of payload */
    Config_Payload_t payload;
} Config_Record_t;

/* ----------------------------------------------------------------
 * API
 * ---------------------------------------------------------------- */

/**
 * Load config from flash sector 0.
 * Returns 1 if valid record found and applied, 0 if CRC fail / blank.
 */
uint8_t Config_Load(void);

/**
 * Save current PID + VoutRef + FAN + WorkMode + CV/CC to flash sector 0.
 */
void Config_Save(void);

/**
 * Simple CRC32 (no lookup table, fits in RAM-constrained target)
 */
uint32_t Config_CRC32(const uint8_t *data, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* __CONFIG_STORE_H__ */
