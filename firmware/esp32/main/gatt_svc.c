#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "esp_log.h"
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gatt/ble_svc_gatt.h"

#include "blectf.h"
#include "flags.h"

static const char *TAG = "blectf.gatt";

/*
 * 16-bit UUIDs in the 0xFFxx custom range. This mirrors the mapping BLEtterCap
 * already ships in gatt_names.py, so the analyzer labels the characteristics
 * automatically.
 */
#define UUID_SVC          0xFF00
#define UUID_SCORE        0xFF01  /* READ | NOTIFY : "flags captured N/M"     */
#define UUID_SUBMIT       0xFF02  /* WRITE         : submit a flag string     */
#define UUID_FLAG_READ    0xFF03  /* READ          : T0 - value is the flag   */
#define UUID_FLAG_DESC    0xFF04  /* READ          : T0 - flag in 0x2901 desc */
#define UUID_FLAG_WRITE   0xFF05  /* READ | WRITE  : T0 - write key to unlock */
#define UUID_FLAG_NOTIFY  0xFF06  /* NOTIFY        : T1 - subscribe to get it */

#define UNLOCK_KEY "d34dbeef"

static uint16_t g_score_handle;
static uint16_t g_notify_handle;
static bool     g_write_unlocked;

static int gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(UUID_SVC),
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = BLE_UUID16_DECLARE(UUID_SCORE),
                .access_cb = gatt_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &g_score_handle,
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_SUBMIT),
                .access_cb = gatt_access,
                .flags = BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_FLAG_READ),
                .access_cb = gatt_access,
                .flags = BLE_GATT_CHR_F_READ,
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_FLAG_DESC),
                .access_cb = gatt_access,
                .flags = BLE_GATT_CHR_F_READ,
                .descriptors = (struct ble_gatt_dsc_def[]) {
                    {
                        .uuid = BLE_UUID16_DECLARE(0x2901), /* User Description */
                        .access_cb = gatt_access,
                        .att_flags = BLE_ATT_F_READ,
                    },
                    { 0 }
                },
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_FLAG_WRITE),
                .access_cb = gatt_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
            },
            {
                .uuid = BLE_UUID16_DECLARE(UUID_FLAG_NOTIFY),
                .access_cb = gatt_access,
                .flags = BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &g_notify_handle,
            },
            { 0 }
        },
    },
    { 0 }
};

static uint16_t ctxt_uuid16(const struct ble_gatt_access_ctxt *ctxt)
{
    const ble_uuid_t *u = (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC ||
                           ctxt->op == BLE_GATT_ACCESS_OP_WRITE_DSC)
                          ? ctxt->dsc->uuid
                          : ctxt->chr->uuid;
    return ble_uuid_u16(u);
}

static int append_str(struct ble_gatt_access_ctxt *ctxt, const char *s)
{
    int rc = os_mbuf_append(ctxt->om, s, strlen(s));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int read_flat(struct ble_gatt_access_ctxt *ctxt, char *buf, size_t bufsz)
{
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len >= bufsz) {
        len = bufsz - 1;
    }
    int rc = ble_hs_mbuf_to_flat(ctxt->om, buf, len, &len);
    if (rc != 0) {
        return -1;
    }
    buf[len] = '\0';
    return len;
}

static int gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char buf[128];
    int  len;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        switch (ctxt_uuid16(ctxt)) {
        case UUID_SCORE:
            snprintf(buf, sizeof buf, "flags captured %d/%d",
                     blectf_solved_count(), blectf_flag_count());
            return append_str(ctxt, buf);
        case UUID_FLAG_READ:
            return append_str(ctxt, blectf_flag_secret(1));
        case UUID_FLAG_WRITE:
            return append_str(ctxt, g_write_unlocked
                                    ? blectf_flag_secret(8)
                                    : "write " UNLOCK_KEY " here");
        }
        return BLE_ATT_ERR_UNLIKELY;

    case BLE_GATT_ACCESS_OP_READ_DSC:
        /* 0x2901 User Description of 0xFF04 carries flag 3 */
        return append_str(ctxt, blectf_flag_secret(3));

    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        len = read_flat(ctxt, buf, sizeof buf);
        if (len < 0) {
            return BLE_ATT_ERR_UNLIKELY;
        }
        switch (ctxt_uuid16(ctxt)) {
        case UUID_SUBMIT:
            if (blectf_submit(buf, len)) {
                ESP_LOGI(TAG, "flag captured (%d/%d)",
                         blectf_solved_count(), blectf_flag_count());
                blectf_notify_score(conn_handle);
            }
            return 0;
        case UUID_FLAG_WRITE:
            if (strcmp(buf, UNLOCK_KEY) == 0) {
                g_write_unlocked = true;
            }
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

void blectf_notify_score(uint16_t conn_handle)
{
    char buf[64];
    snprintf(buf, sizeof buf, "flags captured %d/%d",
             blectf_solved_count(), blectf_flag_count());
    struct os_mbuf *om = ble_hs_mbuf_from_flat(buf, strlen(buf));
    if (om) {
        ble_gatts_notify_custom(conn_handle, g_score_handle, om);
    }
}

void blectf_on_subscribe(uint16_t conn_handle, uint16_t attr_handle, int cur_notify)
{
    /* T1: subscribing to 0xFF06 immediately pushes flag 10 as a notification */
    if (attr_handle == g_notify_handle && cur_notify) {
        const char *s = blectf_flag_secret(10);
        struct os_mbuf *om = ble_hs_mbuf_from_flat(s, strlen(s));
        if (om) {
            ble_gatts_notify_custom(conn_handle, g_notify_handle, om);
        }
    }
}

int blectf_gatt_init(void)
{
    int rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        return rc;
    }
    return ble_gatts_add_svcs(gatt_svcs);
}
