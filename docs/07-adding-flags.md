# Adding a flag

A flag is two things: a **row in the registry** and a **mechanic** that reveals
its secret. Here is the full loop, using a new T0 "read" flag as the example.

## 1. Add the registry row

In `firmware/esp32/main/flags.c`, add to `BLECTF_FLAGS[]`:

```c
{ 2, "flag in the value", TIER_T0_GATT, "flag{plain_read_2}", "read characteristic 0xFF07" },
```

Fields: `id`, `name`, `tier`, `secret` (what the player submits), `hint` (one
line, surfaced by the BLEtterCap UI). The scoreboard total updates automatically
from the table size.

## 2. Expose the mechanic

In `firmware/esp32/main/gatt_svc.c`:

Add the UUID and a characteristic entry to `gatt_svcs[]`:

```c
#define UUID_FLAG_READ2 0xFF07
...
{
    .uuid = BLE_UUID16_DECLARE(UUID_FLAG_READ2),
    .access_cb = gatt_access,
    .flags = BLE_GATT_CHR_F_READ,
},
```

Handle it in `gatt_access()` under `BLE_GATT_ACCESS_OP_READ_CHR`:

```c
case UUID_FLAG_READ2:
    return append_str(ctxt, blectf_flag_secret(2));
```

## 3. Document it

Flip the status of the flag in `docs/04-flag-catalog.md` and, if the mechanic is
new, add a note.

## Mechanic recipes

| Mechanic | Where | How |
|----------|-------|-----|
| Direct read | `gatt_access` READ_CHR | `append_str(ctxt, blectf_flag_secret(id))` |
| Descriptor (0x2901) | `gatt_svcs` `.descriptors` + READ_DSC | see 0xFF04 |
| Write-to-unlock | READ_CHR + WRITE_CHR + a flag | see 0xFF05 / `g_write_unlocked` |
| Notify on subscribe | `.val_handle` + `blectf_on_subscribe` | see 0xFF06 |
| Advertising payload | `flags.c` `blectf_mfg_data` + `main.c` adv fields | see flag 15 |
| Encryption required (T3) | add `BLE_GATT_CHR_F_READ_ENC` to `.flags` | pairing triggers automatically |
| Authenticated (T3) | add `BLE_GATT_CHR_F_READ_AUTHEN` | forces MITM protection |

## Encryption example (T3)

To require pairing before a characteristic can be read:

```c
{
    .uuid = BLE_UUID16_DECLARE(0xFF10),
    .access_cb = gatt_access,
    .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC,
},
```

SMP is already configured in `main.c` (`sm_bonding`, `sm_sc`). The host stack
initiates pairing when the client reads the encrypted characteristic.

## Guidelines

- One primitive per flag. Do not combine concepts (except the capstone).
- Keep advertising payloads under 31 bytes total (name + flags + your data) for
  legacy advertising; use extended advertising for larger BLE5 payloads.
- Prefer `observe via sniffer` over `set it to X` for MTU / PHY / conn params.
- Keep secrets short if they ride in an advertisement.
