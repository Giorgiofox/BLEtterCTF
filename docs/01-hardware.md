# Hardware

## Target: any ESP32

BLEtterCTF runs on a single ESP32 board. You do **not** need a C6 - pick by how
far up the tiers you want to go.

| Chip | BLE version | Covers | Notes |
|------|-------------|--------|-------|
| ESP32 (classic) | 4.2 | T0, T1, T3, most T6 | no BLE5: no ext/periodic adv, no coded/2M PHY control |
| ESP32-C3 | 5.0 | + T2-ext, T5 (2M/coded) | cheap, single core |
| ESP32-S3 | 5.0 | + T2-ext, T5 | USB-native, common devkits |
| ESP32-C6 | 5.3 | everything | newest radio; also 802.15.4 |

Any devkit works (DevKitC, XIAO, dev boards, bare modules with a USB-serial
adapter). A USB-C devkit with a native USB-serial bridge is the easiest.

## Attacker side (what solves the flags)

The **client**, not the target, sets the ceiling on what you can solve. Commodity
host radios cannot do coded PHY, periodic advertising sync, PHY control, or set
their MAC. So solvability is tiered:

| Piece | Cost | Unlocks |
|-------|------|---------|
| Mac / PC / phone (built-in BLE) | you have it | T0-T3 core |
| ButteRFly sniffer | owned | observe-only flags (T2-ext observe, T4 RPA, T5 conn-param, LESC-vs-legacy) |
| nRF52840 USB dongle | ~10 EUR | must-control flags (coded PHY read, 2M-only, directed/identity adv, some L2CAP/OOB) |

The nRF dongle is flashed with the BLEtterCTF probe firmware
(`firmware/nrf-probe/`, see `docs/06-nrf-probe.md`) and doubles as a general
BLEtterCap probe.

## Note on port identification

An ESP32 devkit and the ButteRFly sniffer are different USB devices. Always
confirm which serial port belongs to which before flashing (see
`docs/02-flashing.md`). BLEtterCap must never auto-open the CTF target's serial
port; that is a manual, deliberate flashing step only.
