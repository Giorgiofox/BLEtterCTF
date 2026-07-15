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

## GATT map (scaffold)

| UUID | Properties | Purpose |
|------|-----------|---------|
| 0xFF00 | service | BLEtterCTF |
| 0xFF01 | READ, NOTIFY | scoreboard: `flags captured N/M` |
| 0xFF02 | WRITE | submit a flag string here |
| 0xFF03 | READ | T0: value is flag 1 |
| 0xFF04 | READ (+0x2901 desc) | T0: flag 3 in the user description |
| 0xFF05 | READ, WRITE | T0: write `d34dbeef`, then read flag 8 |
| 0xFF06 | NOTIFY | T1: subscribe to receive flag 10 |

Flag 15 rides in the advertising manufacturer-specific data.

## Adding flags

See [../../docs/07-adding-flags.md](../../docs/07-adding-flags.md).

## Note

This is a scaffold: one working example per mechanic class. It is written to be
read and extended, not to be a finished 38-flag build. Not compile-tested in this
environment; build it with ESP-IDF as above.
