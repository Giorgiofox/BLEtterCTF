#pragma once

#include <stdint.h>
#include <stddef.h>

/*
 * BLEtterCTF - shared firmware interface.
 *
 * Target: a single ESP32 (esp32 / esp32-s3 / esp32-c3 / esp32-c6) running
 * ESP-IDF + NimBLE. BLE5-capable chips (c3/c6/s3) unlock the advanced tiers
 * (extended/periodic advertising, coded PHY). The classic esp32 is BLE 4.2 and
 * covers the core tiers only.
 *
 * Everything here is solvable with standard tools (bluetoothctl, bleak, the
 * nRF Connect app) - no BLEtterCap required.
 */

#define BLECTF_DEVICE_NAME "BLEtterCTF"

/* GATT server (gatt_svc.c) */
int  blectf_gatt_init(void);
void blectf_on_subscribe(uint16_t conn_handle, uint16_t attr_handle,
                         int cur_notify, int cur_indicate);
void blectf_notify_score(uint16_t conn_handle);

/* Advertising payloads (flags.c) */
uint8_t *blectf_mfg_data(void);       /* T2 flag 15: manufacturer data in ADV     */
uint8_t  blectf_mfg_data_len(void);
uint8_t *blectf_scanrsp_data(void);   /* T2 flag 16: manufacturer data in SCAN RSP */
uint8_t  blectf_scanrsp_data_len(void);
uint8_t *blectf_svc_data(void);       /* T2 flag 17: service data in SCAN RSP      */
uint8_t  blectf_svc_data_len(void);
const char *blectf_full_device_name(void); /* T2 flag 4: full GAP name (0x2A00)    */

/* Scoreboard and flag registry (flags.c) */
int         blectf_flag_count(void);            /* flags currently implemented */
int         blectf_solved_count(void);          /* flags captured this session */
int         blectf_submit(const char *flag, size_t len); /* 1 if new valid flag */
const char *blectf_flag_secret(int id);         /* secret string for a flag id  */
