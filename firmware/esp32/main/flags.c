#include <string.h>
#include <stdint.h>

#include "flags.h"
#include "blectf.h"

/*
 * Implemented flag table. The score is honest: it reports N / (rows here), and
 * grows as more mechanics are wired up. The FULL 38-flag catalog (including the
 * advanced tiers that need the nRF probe or a sniffer) lives in
 * docs/04-flag-catalog.md and the wiki.
 *
 * Secrets are plain readable strings in this build so the mechanics are easy to
 * follow. The production build salts them per client so they cannot be googled
 * or copied between players.
 *
 * Every flag here is solvable with standard tools - no BLEtterCap needed.
 */
const blectf_flag_t BLECTF_FLAGS[] = {
    /* T0 - GATT base */
    {  1, "the gift",                 TIER_T0_GATT,   "flag{welcome_you_absolute_legend}", "read 0xFF03. yes, it is that easy. savour it." },
    {  2, "hello, world",             TIER_T0_GATT,   "flag{plain_and_simple}",            "read the value of 0xFF07." },
    {  3, "read the fine print",      TIER_T0_GATT,   "flag{descriptors_are_not_decor}",   "characteristics carry descriptors. read the 0x2901 of 0xFF04." },
    {  5, "hex marks the spot",       TIER_T0_GATT,   "flag{hex_is_just_base16}",          "0xFF08 looks like gibberish. it is hex. decode it." },
    {  6, "sixty-four shades",        TIER_T0_GATT,   "flag{base64_is_not_encryption}",    "0xFF09 is base64. decoding is not hacking, but nicely done." },
    {  7, "collect all three",        TIER_T0_GATT,   "flag{teamwork_dream_work}",         "0xFF0A, 0xFF0B, 0xFF0C each hold a third. glue them in order." },
    {  8, "knock knock",              TIER_T0_GATT,   "flag{write_then_read_classic}",     "write d34dbeef to 0xFF05, then read it back." },
    {  9, "the magic words, in order",TIER_T0_GATT,   "flag{state_machines_hold_grudges}", "write 1, then 2, then 3 to 0xFF0D. wrong order resets it." },
    /* T1 - Events */
    { 10, "ring ring",               TIER_T1_EVENTS,  "flag{notifications_never_sleep}",   "subscribe to 0xFF06 and wait for the call." },
    { 11, "read receipts on",        TIER_T1_EVENTS,  "flag{indicate_wants_an_ack}",       "subscribe to 0xFF0E. indications are notifications that need a reply." },
    { 12, "the flag that would not fit",TIER_T1_EVENTS,"flag{reassembly_required}",        "subscribe to 0xFF0F. it arrives in pieces. rebuild it." },
    /* T2 - Advertising */
    { 15, "hidden in plain sight",   TIER_T2_ADV,     "advf1337",                          "you need not even connect. scan and read the manufacturer data." },
    { 16, "ask nicely",             TIER_T2_ADV,      "flag{active_scanning_ftw}",         "the beacon holds back. do an active scan, read the scan response." },
    /* T3 - Security */
    { 21, "members only",            TIER_T3_SEC,     "flag{encryption_is_a_feature}",     "0xFF13 will not talk until you pair. Just Works pairing is enough." },
    /* T6 - Advanced (host-solvable) */
    { 36, "too big for one bite",    TIER_T6_ADV2,    "flag{read_blobs_are_a_thing_yeah}", "0xFF14 is longer than one MTU. read the whole thing, not the first chunk." },
};
const int BLECTF_FLAG_COUNT = sizeof(BLECTF_FLAGS) / sizeof(BLECTF_FLAGS[0]);

/* solved[id] != 0 once captured. In RAM for now; persist to NVS for a real event
 * so a reconnect keeps the score. */
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
 * T2 flag 15: manufacturer-specific data in the ADV PDU.
 *   bytes 0..1 : company id 0xFFFF (test / unassigned)
 *   bytes 2..  : flag 15 secret
 * Kept short so ADV stays under 31 bytes with the complete device name.
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

/*
 * T2 flag 16: manufacturer-specific data in the SCAN RESPONSE PDU. Only visible
 * to a client doing an ACTIVE scan (one that sends SCAN_REQ), which teaches the
 * difference between passive and active scanning.
 *   bytes 0..1 : company id 0xFFFE
 *   bytes 2..  : flag 16 secret
 */
static uint8_t g_rsp[2 + 28];
static uint8_t g_rsp_len;

uint8_t *blectf_scanrsp_data(void)
{
    if (g_rsp_len == 0) {
        const char *s = blectf_flag_secret(16);
        size_t n = strlen(s);
        if (n > sizeof(g_rsp) - 2) {
            n = sizeof(g_rsp) - 2;
        }
        g_rsp[0] = 0xFE;
        g_rsp[1] = 0xFF;
        memcpy(&g_rsp[2], s, n);
        g_rsp_len = (uint8_t)(2 + n);
    }
    return g_rsp;
}

uint8_t blectf_scanrsp_data_len(void)
{
    blectf_scanrsp_data();
    return g_rsp_len;
}
