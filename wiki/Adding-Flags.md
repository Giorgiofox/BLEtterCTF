# Adding Flags

Want to author your own challenges, or help finish the catalog? A flag is two
things: a **row in the registry** and a **mechanic** that reveals its secret.
Here is the full loop, using a new "read" flag as the example. (Full source
reference: `firmware/esp32/main/`.)

## 1. Add the registry row

In `firmware/esp32/main/flags.c`, add to `BLECTF_FLAGS[]`:

```c
{ 2, "hello, world", TIER_T0_GATT, "flag{plain_and_simple}", "read 0xFF07." },
```

Fields: `id`, `name`, `tier`, `secret` (what the player submits), `hint` (one
line, surfaced by the BLEtterCap UI). The scoreboard total updates automatically
from the table size, so the score stays honest.

## 2. Expose the mechanic

In `firmware/esp32/main/gatt_svc.c`, add the UUID and a characteristic entry to
`gatt_svcs[]`:

```c
#define UUID_F2_READ 0xFF07
...
{ .uuid = BLE_UUID16_DECLARE(UUID_F2_READ), .access_cb = gatt_access,
  .flags = BLE_GATT_CHR_F_READ },
```

Then handle it in `gatt_access()` under `BLE_GATT_ACCESS_OP_READ_CHR`:

```c
case UUID_F2_READ:
    return append_str(ctxt, blectf_flag_secret(2));
```

## 3. Document it

Flip the flag's Status in [Flag Catalog](Flag-Catalog) and, if the mechanic is
new, add a note to the relevant walkthrough.

## Mechanic recipes

| Mechanic | Where | How (see reference) |
|----------|-------|--------------------|
| Direct read | READ_CHR | `append_str(ctxt, blectf_flag_secret(id))` |
| Encoded value | READ_CHR | encode in the callback (see `to_hex`, `to_base64`) |
| Split across chars | READ_CHR x N | see 0xFF0A/0B/0C and `append_third` |
| Descriptor (0x2901) | `.descriptors` + READ_DSC | see 0xFF04 |
| Write-to-unlock | READ + WRITE + flag | see 0xFF05 / `g_write_unlocked` |
| State machine | WRITE + counter | see 0xFF0D / `g_seq_state` |
| Notify on subscribe | `.val_handle` + `blectf_on_subscribe` | see 0xFF06 |
| Indicate on subscribe | `F_INDICATE` + `ble_gatts_indicate_custom` | see 0xFF0E |
| Chunked notify | loop `notify_custom` | see 0xFF0F |
| Advertising payload | `blectf_mfg_data` + `main.c` | see flag 15 |
| Scan response payload | `blectf_scanrsp_data` + `main.c` | see flag 16 |
| Encryption required (T3) | add `BLE_GATT_CHR_F_READ_ENC` | see 0xFF13 |
| Authenticated (T3) | add `BLE_GATT_CHR_F_READ_AUTHEN` + passkey handler | planned |
| Long value (Read Blob) | value longer than one MTU | see 0xFF14 |

## Encryption example (T3)

To require pairing before a characteristic can be read:

```c
{ .uuid = BLE_UUID16_DECLARE(0xFF13), .access_cb = gatt_access,
  .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_READ_ENC },
```

SMP is already configured in `main.c` (`sm_bonding`, `sm_sc`, `sm_io_cap`). The
host stack initiates pairing when the client reads the encrypted characteristic.
For Passkey / Numeric Comparison, change `sm_io_cap` and add a
`BLE_GAP_EVENT_PASSKEY_ACTION` handler in the GAP event callback.

## Guidelines

- **One primitive per flag.** Do not combine concepts (except the capstone).
- **Advertising fits in 31 bytes.** Name + flags + your data must fit the legacy
  ADV PDU; use the scan response for a second 31 bytes, or extended advertising
  (BLE5) for more.
- **Prefer observe over set** for MTU / PHY / connection parameters. Hosts rarely
  let apps set them; make those flags sniffer observations.
- **Keep it fun.** A good hint teaches and makes the player smile. See the
  existing `hint` fields for the house style.
