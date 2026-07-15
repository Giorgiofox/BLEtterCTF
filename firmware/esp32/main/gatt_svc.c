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
 * ships in gatt_names.py, but the CTF is fully solvable with any GATT client.
 */
#define UUID_SVC          0xFF00
#define UUID_SCORE        0xFF01  /* READ|NOTIFY : "flags captured N/M"           */
#define UUID_SUBMIT       0xFF02  /* WRITE       : submit a flag string           */
#define UUID_F1_READ      0xFF03  /* READ        : flag 1 (gift)                  */
#define UUID_F3_DESC      0xFF04  /* READ + 0x2901 desc holds flag 3             */
#define UUID_F8_WRITE     0xFF05  /* READ|WRITE  : write key -> flag 8            */
#define UUID_F10_NOTIFY   0xFF06  /* NOTIFY      : flag 10                        */
#define UUID_F2_READ      0xFF07  /* READ        : flag 2 (plain)                 */
#define UUID_F5_HEX       0xFF08  /* READ        : flag 5 hex-encoded             */
#define UUID_F6_B64       0xFF09  /* READ        : flag 6 base64-encoded          */
#define UUID_F7_A         0xFF0A  /* READ        : flag 7 piece 1/3               */
#define UUID_F7_B         0xFF0B  /* READ        : flag 7 piece 2/3               */
#define UUID_F7_C         0xFF0C  /* READ        : flag 7 piece 3/3               */
#define UUID_F9_STATE     0xFF0D  /* READ|WRITE  : flag 9 state machine (1,2,3)   */
#define UUID_F11_INDICATE 0xFF0E  /* INDICATE    : flag 11                        */
#define UUID_F12_MULTI    0xFF0F  /* NOTIFY      : flag 12, chunked               */
#define UUID_F21_ENC      0xFF13  /* READ (enc)  : flag 21, needs pairing         */
#define UUID_F36_LONG     0xFF14  /* READ        : flag 36, longer than one MTU   */

#define UNLOCK_KEY "d34dbeef"

static uint16_t g_score_handle;
static uint16_t g_f10_handle;
static uint16_t g_f11_handle;
static uint16_t g_f12_handle;

static bool g_write_unlocked;   /* flag 8  */
static int  g_seq_state;        /* flag 9: 0 -> 1 -> 2 -> 3(unlocked) */

static int gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg);

static const struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = BLE_UUID16_DECLARE(UUID_SVC),
        .characteristics = (struct ble_gatt_chr_def[]) {
            { .uuid = BLE_UUID16_DECLARE(UUID_SCORE), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
              .val_handle = &g_score_handle },
            { .uuid = BLE_UUID16_DECLARE(UUID_SUBMIT), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_WRITE },
            { .uuid = BLE_UUID16_DECLARE(UUID_F1_READ), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(UUID_F3_DESC), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ,
              .descriptors = (struct ble_gatt_dsc_def[]) {
                  { .uuid = BLE_UUID16_DECLARE(0x2901), .access_cb = gatt_access,
                    .att_flags = BLE_ATT_F_READ },
                  { 0 } } },
            { .uuid = BLE_UUID16_DECLARE(UUID_F8_WRITE), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE },
            { .uuid = BLE_UUID16_DECLARE(UUID_F10_NOTIFY), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_NOTIFY, .val_handle = &g_f10_handle },
            { .uuid = BLE_UUID16_DECLARE(UUID_F2_READ), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(UUID_F5_HEX), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(UUID_F6_B64), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(UUID_F7_A), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(UUID_F7_B), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(UUID_F7_C), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { .uuid = BLE_UUID16_DECLARE(UUID_F9_STATE), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE },
            { .uuid = BLE_UUID16_DECLARE(UUID_F11_INDICATE), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_INDICATE, .val_handle = &g_f11_handle },
            { .uuid = BLE_UUID16_DECLARE(UUID_F12_MULTI), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_NOTIFY, .val_handle = &g_f12_handle },
            { .uuid = BLE_UUID16_DECLARE(UUID_F21_ENC), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC },
            { .uuid = BLE_UUID16_DECLARE(UUID_F36_LONG), .access_cb = gatt_access,
              .flags = BLE_GATT_CHR_F_READ },
            { 0 }
        },
    },
    { 0 }
};

/* ------------------------------------------------------------------ helpers */

static uint16_t ctxt_uuid16(const struct ble_gatt_access_ctxt *ctxt)
{
    const ble_uuid_t *u = (ctxt->op == BLE_GATT_ACCESS_OP_READ_DSC ||
                           ctxt->op == BLE_GATT_ACCESS_OP_WRITE_DSC)
                          ? ctxt->dsc->uuid : ctxt->chr->uuid;
    return ble_uuid_u16(u);
}

static int append_str(struct ble_gatt_access_ctxt *ctxt, const char *s)
{
    int rc = os_mbuf_append(ctxt->om, s, strlen(s));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int append_buf(struct ble_gatt_access_ctxt *ctxt, const void *p, size_t n)
{
    int rc = os_mbuf_append(ctxt->om, p, n);
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int read_flat(struct ble_gatt_access_ctxt *ctxt, char *buf, size_t bufsz)
{
    uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
    if (len >= bufsz) {
        len = bufsz - 1;
    }
    if (ble_hs_mbuf_to_flat(ctxt->om, buf, len, &len) != 0) {
        return -1;
    }
    buf[len] = '\0';
    return len;
}

static void to_hex(const char *in, char *out, size_t outsz)
{
    static const char h[] = "0123456789abcdef";
    size_t o = 0;
    for (size_t i = 0; in[i] && o + 2 < outsz; i++) {
        out[o++] = h[(uint8_t)in[i] >> 4];
        out[o++] = h[(uint8_t)in[i] & 0x0f];
    }
    out[o] = '\0';
}

static void to_base64(const char *in, char *out, size_t outsz)
{
    static const char b[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t len = strlen(in), o = 0;
    for (size_t i = 0; i < len; i += 3) {
        uint32_t n = (uint8_t)in[i] << 16;
        if (i + 1 < len) n |= (uint8_t)in[i + 1] << 8;
        if (i + 2 < len) n |= (uint8_t)in[i + 2];
        if (o + 4 >= outsz) break;
        out[o++] = b[(n >> 18) & 63];
        out[o++] = b[(n >> 12) & 63];
        out[o++] = (i + 1 < len) ? b[(n >> 6) & 63] : '=';
        out[o++] = (i + 2 < len) ? b[n & 63] : '=';
    }
    out[o] = '\0';
}

/* flag 7 is split into three thirds; return the i-th third (0..2) */
static int append_third(struct ble_gatt_access_ctxt *ctxt, int part)
{
    const char *s = blectf_flag_secret(7);
    size_t len = strlen(s);
    size_t third = len / 3;
    size_t start = part * third;
    size_t n = (part == 2) ? (len - start) : third;
    return append_buf(ctxt, s + start, n);
}

/* ------------------------------------------------------------ access router */

static int gatt_access(uint16_t conn_handle, uint16_t attr_handle,
                       struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    char buf[128];
    char enc[192];
    int  len;

    switch (ctxt->op) {
    case BLE_GATT_ACCESS_OP_READ_CHR:
        switch (ctxt_uuid16(ctxt)) {
        case UUID_SCORE:
            snprintf(buf, sizeof buf, "flags captured %d/%d",
                     blectf_solved_count(), blectf_flag_count());
            return append_str(ctxt, buf);
        case UUID_F1_READ:
            return append_str(ctxt, blectf_flag_secret(1));
        case UUID_F2_READ:
            return append_str(ctxt, blectf_flag_secret(2));
        case UUID_F5_HEX:
            to_hex(blectf_flag_secret(5), enc, sizeof enc);
            return append_str(ctxt, enc);
        case UUID_F6_B64:
            to_base64(blectf_flag_secret(6), enc, sizeof enc);
            return append_str(ctxt, enc);
        case UUID_F7_A: return append_third(ctxt, 0);
        case UUID_F7_B: return append_third(ctxt, 1);
        case UUID_F7_C: return append_third(ctxt, 2);
        case UUID_F8_WRITE:
            return append_str(ctxt, g_write_unlocked
                                    ? blectf_flag_secret(8)
                                    : "write " UNLOCK_KEY " here");
        case UUID_F9_STATE:
            return append_str(ctxt, g_seq_state >= 3
                                    ? blectf_flag_secret(9)
                                    : "write 1, 2, 3 in order");
        case UUID_F21_ENC:
            return append_str(ctxt, blectf_flag_secret(21));
        case UUID_F36_LONG:
            return append_str(ctxt, blectf_flag_secret(36));
        }
        return BLE_ATT_ERR_UNLIKELY;

    case BLE_GATT_ACCESS_OP_READ_DSC:
        /* 0x2901 user description of 0xFF04 carries flag 3 */
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
        case UUID_F8_WRITE:
            if (strcmp(buf, UNLOCK_KEY) == 0) {
                g_write_unlocked = true;
            }
            return 0;
        case UUID_F9_STATE:
            /* accept 1, then 2, then 3; any wrong step resets the sequence */
            if (strcmp(buf, "1") == 0 && g_seq_state == 0)      g_seq_state = 1;
            else if (strcmp(buf, "2") == 0 && g_seq_state == 1) g_seq_state = 2;
            else if (strcmp(buf, "3") == 0 && g_seq_state == 2) g_seq_state = 3;
            else if (g_seq_state < 3)                           g_seq_state = 0;
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    return BLE_ATT_ERR_UNLIKELY;
}

/* ------------------------------------------------------------- notifications */

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
    if (!cur_notify) {
        return;
    }

    /* flag 10: one notification with the whole flag */
    if (attr_handle == g_f10_handle) {
        const char *s = blectf_flag_secret(10);
        struct os_mbuf *om = ble_hs_mbuf_from_flat(s, strlen(s));
        if (om) ble_gatts_notify_custom(conn_handle, g_f10_handle, om);
    }

    /* flag 11: an indication (needs an ACK from the client) */
    if (attr_handle == g_f11_handle) {
        const char *s = blectf_flag_secret(11);
        struct os_mbuf *om = ble_hs_mbuf_from_flat(s, strlen(s));
        if (om) ble_gatts_indicate_custom(conn_handle, g_f11_handle, om);
    }

    /* flag 12: split the flag into three notifications; reassemble client-side */
    if (attr_handle == g_f12_handle) {
        const char *s = blectf_flag_secret(12);
        size_t len = strlen(s), off = 0;
        size_t chunk = (len + 2) / 3;
        while (off < len) {
            size_t n = (len - off < chunk) ? (len - off) : chunk;
            struct os_mbuf *om = ble_hs_mbuf_from_flat(s + off, n);
            if (om) ble_gatts_notify_custom(conn_handle, g_f12_handle, om);
            off += n;
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
