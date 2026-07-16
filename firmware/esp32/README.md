# BLEtterCTF ESP32 firmware

ESP-IDF + NimBLE target firmware. This is what you flash onto the ESP32.

## Build

```sh
. ~/esp/esp-idf/export.sh          # ESP-IDF v5.x in your environment
idf.py set-target esp32c6          # or esp32 / esp32c3 / esp32s3
idf.py build
idf.py -p PORT flash monitor
```

Full instructions, port identification, and a no-toolchain prebuilt path are in
[../../docs/02-flashing.md](../../docs/02-flashing.md).

## Files

```
CMakeLists.txt        top-level ESP-IDF project
sdkconfig.defaults    NimBLE + SMP + partition config (edit here, not sdkconfig)
partitions.csv        nvs / phy / factory app layout
main/
  main.c              nimble init, GAP, advertising, SMP, app_main
  gatt_svc.c          0xFF00 GATT service + access callbacks
  flags.c             flag registry, scoreboard, submit, advertising payload
  flags.h             flag table type + tiers
  blectf.h            shared interface
```

## GATT map

| UUID | Properties | Purpose |
|------|-----------|---------|
| 0xFF00 | service | BLEtterCTF |
| 0xFF01 | READ, NOTIFY | scoreboard: `flags captured N/M` |
| 0xFF02 | WRITE | submit a flag string here |
| 0xFF03 | READ | T0 flag 1: gift value |
| 0xFF04 | READ (+0x2901 desc) | T0 flag 3: in the user description |
| 0xFF05 | READ, WRITE | T0 flag 8: write `d34dbeef`, then read |
| 0xFF06 | NOTIFY | T1 flag 10: subscribe to receive |
| 0xFF07 | READ | T0 flag 2: plain value |
| 0xFF08 | READ | T0 flag 5: hex-encoded |
| 0xFF09 | READ | T0 flag 6: base64-encoded |
| 0xFF0A/0B/0C | READ | T0 flag 7: three thirds, concatenate |
| 0xFF0D | READ, WRITE | T0 flag 9: state machine (write 1,2,3) |
| 0xFF0E | INDICATE | T1 flag 11: subscribe (acknowledged) |
| 0xFF0F | NOTIFY | T1 flag 12: chunked, reassemble |
| 0xFF13 | READ (enc) | T3 flag 21: pair (Just Works) to read |
| 0xFF14 | READ | T6 flag 36: longer than one MTU (read blob) |
| 0xFF15 | WRITE, NOTIFY | T1 flag 13: write to trigger the notification |
| 0xFF17 | READ (enc) | T3 flag 24: bonding persistence (reconnect, no re-pair) |
| 0xFF19 | READ, WRITE | T6 flag 35: reveals on an empty (edge-case) write |
| 0xFF1A | READ | T0 flag 4: full device name (advertised name is Shortened) |

Advertising: flag 15 = manufacturer data in the ADV; flag 16 = manufacturer data
in the scan response; flag 17 = service data (UUID 0xFF00) in the scan response.

## Adding flags

See [../../docs/07-adding-flags.md](../../docs/07-adding-flags.md).

## Status

Implements 15 flags across T0-T3 plus a long-read (T6), all solvable with
standard tools (bluetoothctl / bleak / nRF Connect), no BLEtterCap required. The
remaining catalog flags (BLE5 advertising, privacy, PHY, L2CAP) are ported in as
the firmware grows and tested on hardware. Not compile-tested in this environment;
build it with ESP-IDF as above.
