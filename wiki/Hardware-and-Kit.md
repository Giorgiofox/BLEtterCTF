# Hardware and Kit

The short version: you need one ESP32 and a device you already own. Everything
else is optional and cheap. Here is the long version.

## The target: any ESP32

BLEtterCTF runs on a single ESP32. You do **not** need a C6.

| Chip | BLE version | Covers | Notes |
|------|-------------|--------|-------|
| ESP32 (classic) | 4.2 | T0, T1, T3, most T6 | no BLE5: no ext/periodic adv, no coded/2M PHY |
| ESP32-C3 | 5.0 | + T2 extended, T5 (2M/coded) | cheap, single core |
| ESP32-S3 | 5.0 | + T2 extended, T5 | USB-native, common devkits |
| ESP32-C6 | 5.3 | everything | newest radio; also 802.15.4 |

Any devkit works (DevKitC, XIAO, generic boards, or a bare module with a
USB-serial adapter). A USB-C devkit with a native USB-serial bridge is easiest.
Bring a **data** USB cable, not a charge-only one.

## The attacker side: what solves the flags

The **client**, not the target, sets the ceiling on what you can solve (see the
[primer](BLE-Primer), section 2, on the host/controller split).

| Piece | Cost | Unlocks | Needed for |
|-------|------|---------|------------|
| Mac / PC / phone (built-in BLE) | you have it | T0-T3 core | the whole core game |
| ButteRFly sniffer | (owned) | observe-only flags | T2 observe, T4 RPA, T5 conn-param, T3 #26 |
| nRF52840 USB dongle | ~10 EUR | radio-control flags | T2 ext/periodic, T4 directed/identity, T5 PHY, T6 L2CAP |

None of these require BLEtterCap. The sniffer feeds Wireshark; the nRF dongle runs
from a script or the BLEtterCTF probe firmware.

## The nRF52840 probe

For the flags that need to actively control the radio, flash an nRF52840 dongle
with the BLEtterCTF probe firmware (`firmware/nrf-probe/`, planned on Zephyr). It
gives you: periodic-advertising sync, coded/2M PHY control, a settable address,
and an L2CAP client. It doubles as a general BLEtterCap probe, so it is not a
single-use purchase.

## A note on ports and safety

An ESP32 devkit and a sniffer dongle are different USB devices that both appear as
serial ports. Always confirm which port is which before flashing (see
[Flashing the ESP32](Flashing-the-ESP32)). BLEtterCap must never auto-open the CTF
target's serial port; flashing is a deliberate manual step.

## Recommended starter kit

1. One **ESP32-C6** devkit (futureproof, does everything) - or a classic ESP32 if
   you only want the core game.
2. Your **phone** with nRF Connect (free).

That is enough to flash the board and clear Tiers 0-3. Add the sniffer and nRF
dongle later, only if you want the BLE 5 tiers.
