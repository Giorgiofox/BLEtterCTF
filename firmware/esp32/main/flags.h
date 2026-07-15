#pragma once

#include <stdint.h>

/*
 * Flag registry. Every challenge is a row in BLECTF_FLAGS. Adding a flag is:
 *   1. add a row here (id, name, tier, secret, hint)
 *   2. expose whatever GATT/advertising mechanic reveals its `secret`
 *      (see gatt_svc.c and docs/07-adding-flags.md)
 *
 * `secret` is the exact string the player must write to the submit
 * characteristic (0xFF02). In the production build these are salted per client;
 * in this scaffold they are plain readable strings so the mechanics are easy to
 * follow.
 */

typedef enum {
    TIER_T0_GATT,     /* GATT base: read/write/descriptors/state          */
    TIER_T1_EVENTS,   /* notify / indicate / CCCD                          */
    TIER_T2_ADV,      /* advertising: mfg data, scan resp, ext/periodic    */
    TIER_T3_SEC,      /* SMP: pairing, bonding, LESC                       */
    TIER_T4_PRIV,     /* privacy: RPA, directed adv, address filtering     */
    TIER_T5_PHY,      /* PHY: 2M, coded/long range, conn params            */
    TIER_T6_ADV2,     /* L2CAP CoC, caching, fuzzing, capstone             */
} blectf_tier_t;

typedef struct {
    int            id;
    const char    *name;
    blectf_tier_t  tier;
    const char    *secret;
    const char    *hint;
} blectf_flag_t;

extern const blectf_flag_t BLECTF_FLAGS[];
extern const int           BLECTF_FLAG_COUNT;
