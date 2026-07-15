#include <string.h>
#include <stdint.h>

#include "flags.h"
#include "blectf.h"

/*
 * Scaffold flag table. Only a handful are implemented so every *class* of
 * mechanic has a working example: direct read, descriptor, write-to-unlock,
 * notify, and advertising. The full 38-flag catalog lives in
 * docs/04-flag-catalog.md; port rows here as you build each mechanic.
 */
const blectf_flag_t BLECTF_FLAGS[] = {
    { 1,  "gift / read char",     TIER_T0_GATT,   "flag{welcome_bletterctf}", "read characteristic 0xFF03" },
    { 3,  "hidden in descriptor", TIER_T0_GATT,   "flag{read_the_0x2901}",    "read the User Description (0x2901) of 0xFF04" },
    { 8,  "write to unlock",      TIER_T0_GATT,   "flag{write_then_read}",    "write d34dbeef to 0xFF05, then read it back" },
    { 10, "notify subscribe",     TIER_T1_EVENTS, "flag{cccd_subscribe}",     "enable notifications on 0xFF06" },
    { 15, "advertising mfg data", TIER_T2_ADV,    "advf1337",                 "scan and parse the manufacturer-specific data" },
};
const int BLECTF_FLAG_COUNT = sizeof(BLECTF_FLAGS) / sizeof(BLECTF_FLAGS[0]);

/* solved[id] != 0 once captured. Kept in RAM for the scaffold; persist to NVS
 * for a real event so a reconnect keeps the score. */
static uint8_t g_solved[128];

static const blectf_flag_t *find_by_id(int id)
{
    for (int i = 0; i < BLECTF_FLAG_COUNT; i++) {
        if (BLECTF_FLAGS[i].id == id) {
            return &BLECTF_FLAGS[i];
        }
    }
    return NULL;
}

int blectf_flag_count(void)
{
    return BLECTF_FLAG_COUNT;
}

int blectf_solved_count(void)
{
    int n = 0;
    for (int i = 0; i < BLECTF_FLAG_COUNT; i++) {
        if (g_solved[BLECTF_FLAGS[i].id]) {
            n++;
        }
    }
    return n;
}

int blectf_submit(const char *flag, size_t len)
{
    for (int i = 0; i < BLECTF_FLAG_COUNT; i++) {
        const blectf_flag_t *f = &BLECTF_FLAGS[i];
        if (strlen(f->secret) == len && memcmp(f->secret, flag, len) == 0) {
            if (g_solved[f->id]) {
                return 0; /* already captured */
            }
            g_solved[f->id] = 1;
            return 1;
        }
    }
    return 0;
}

const char *blectf_flag_secret(int id)
{
    const blectf_flag_t *f = find_by_id(id);
    return f ? f->secret : "";
}

/*
 * T2: manufacturer-specific advertising data.
 *   bytes 0..1 : company identifier (0xFFFF = test / unassigned)
 *   bytes 2..  : flag 15 secret
 * Kept short so the whole legacy advertising PDU stays under 31 bytes together
 * with the complete device name.
 */
static uint8_t g_mfg[2 + 20];
static uint8_t g_mfg_len;

uint8_t *blectf_mfg_data(void)
{
    if (g_mfg_len == 0) {
        const char *s = blectf_flag_secret(15);
        size_t n = strlen(s);
        if (n > sizeof(g_mfg) - 2) {
            n = sizeof(g_mfg) - 2;
        }
        g_mfg[0] = 0xFF;
        g_mfg[1] = 0xFF;
        memcpy(&g_mfg[2], s, n);
        g_mfg_len = (uint8_t)(2 + n);
    }
    return g_mfg;
}

uint8_t blectf_mfg_data_len(void)
{
    blectf_mfg_data();
    return g_mfg_len;
}
