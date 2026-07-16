# BLEtterCTF

A hands-on Bluetooth Low Energy Capture-The-Flag, built to actually teach BLE and
its characteristics. It is a modern successor to
[hackgnar/ble_ctf](https://github.com/hackgnar/ble_ctf): same "flash an ESP32 and
attack it" idea, but with a tiered curriculum, real feedback, and tight
integration with the [BLEtterCap](https://github.com/Giorgiofox/BLEttercap) analyzer so you learn by *seeing* the
packets, not by memorizing handles.

## Why another BLE CTF

The original ble_ctf is great but stops at basic GATT plus notify/indicate/MTU,
gives no feedback, ships googlable MD5 flags, and relies on the now-deprecated
`gatttool`. BLEtterCTF fixes all of that:

- **Tiered curriculum (T0-T6)** from `read a characteristic` up to L2CAP CoC,
  LE Secure Connections, privacy/RPA and coded PHY.
- **Learn by seeing** - solve a flag, then replay the exact ATT/SMP/L2CAP
  exchange on the BLEtterCap live graph ("packet X-ray").
- **No dead tooling** - solvable from any Mac/PC/phone for the core tiers via
  the BLEtterCap web UI, `bleak`, or the nRF Connect app.
- **Not all-or-nothing** - beginners are never blocked; advanced tiers unlock
  with a ~10 EUR nRF dongle or the ButteRFly sniffer.

## Tiered solvability

| Tier | Topic | Solvable with |
|------|-------|---------------|
| T0 | GATT base: read/write/descriptors/state | any Mac/PC/phone |
| T1 | notify / indicate / CCCD | any Mac/PC/phone |
| T2 | advertising (mfg/scan-resp), then ext/periodic | Mac/PC; ext/periodic need capable radio |
| T3 | SMP: pairing, bonding, LESC | any Mac/PC/phone |
| T4 | privacy: RPA, directed adv, address filtering | sniffer / nRF probe |
| T5 | PHY: 2M, coded / long range, conn params | nRF probe / sniffer |
| T6 | L2CAP CoC, GATT caching, fuzzing, capstone | mixed |

## Full-clear kit

1. **ESP32** target (esp32 / s3 / c3 / c6). BLE5 chips unlock T2-ext, T5.
2. **Mac/PC/phone** - solves the core (~T0-T3).
3. **ButteRFly sniffer** - observe-only flags.
4. **nRF52840 dongle (~10 EUR)** - the must-control flags (coded PHY, periodic
   adv, address).

All three attacker pieces are things BLEtterCap uses anyway.

## Layout

```
BLEtterCTF/
  firmware/
    esp32/          ESP-IDF + NimBLE target firmware (this is what you flash)
    nrf-probe/      optional nRF52840 client firmware for advanced tiers
  tools/
    solver-examples/  reference solvers (bleak)
  docs/
    01-hardware.md      boards, kit, capability matrix
    02-flashing.md      how to build and flash the ESP32 (start here)
    03-solving-guide.md how to play, tool by tool
    04-flag-catalog.md  the full challenge list (player-facing)
    05-design.md        internal design + solvability matrix
    06-nrf-probe.md     nRF probe setup
    07-adding-flags.md  how to author new flags
```

## Documentation

The detailed, didactic docs live in the **[GitHub Wiki](https://github.com/Giorgiofox/BLEtterCTF/wiki)**,
starting with a full **BLE primer**. The `docs/` and `wiki/` folders in this repo
hold the same material version-controlled. To publish the wiki, see
`tools/publish-wiki.sh` (one-time manual step: create the first wiki page in the
web UI).

## No BLEtterCap required

The entire core game (Tiers 0-3) is solvable with tools you already have:
`bluetoothctl`, Python `bleak`, or the free nRF Connect phone app. BLEtterCap adds
an optional packet X-ray; it is never a dependency.

## Quickstart

1. Flash an ESP32: see the [Flashing](https://github.com/Giorgiofox/BLEtterCTF/wiki/Flashing-the-ESP32) wiki page (or `docs/02-flashing.md`).
2. Confirm it advertises as `BLEtterCTF`.
3. Start capturing flags: [Solving Without BLEtterCap](https://github.com/Giorgiofox/BLEtterCTF/wiki/Solving-Without-BLEtterCap).

## Status

The firmware implements 15 flags across Tiers 0-3 plus a long-read (T6), all
host-solvable. The remaining catalog flags (BLE5 advertising, privacy, PHY,
L2CAP) are ported in as the firmware grows and tested on real hardware. Full list
and status in [Flag Catalog](https://github.com/Giorgiofox/BLEtterCTF/wiki/Flag-Catalog).
