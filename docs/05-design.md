# Design

## Goals

1. Teach BLE and its characteristics by doing, not by memorizing handles.
2. Never block a beginner: the core is solvable with hardware everyone has.
3. Cover modern BLE (5.x) for those who want it, with cheap add-on hardware.
4. Integrate with BLEtterCap so players learn by *seeing* the packets.

## Key decision: the client sets the ceiling

The target is a single ESP32 and it is capable enough for everything on the
peripheral side. The real constraint is the **client** doing the solving:
commodity host radios (laptop/phone built-in BLE) cannot control PHY, sync to
periodic advertising, use coded PHY, or set their address. So solvability is
tiered rather than all-or-nothing.

| | Host (Mac/PC/phone) | + ButteRFly sniffer | + nRF52840 probe |
|--|--|--|--|
| GATT read/write/notify/indicate | yes | | |
| pairing / bonding / LESC | yes (OS agent) | | |
| legacy advertising RX | yes | | |
| extended / periodic advertising | no / unstable | observe | RX + sync |
| coded / 2M PHY | no | observe | control |
| RPA / privacy | limited | observe | |
| settable client address | rarely | | yes |
| L2CAP CoC | macOS/Linux only | | yes |

## Design rules

- **Observe, do not force.** MTU, connection interval, and PHY are usually not
  settable from a commodity host. Challenges about them are `observe via
  sniffer`, never `set it to X` (this is what made classic ble_ctf #16 painful).
- **Flags are per-client salted** in the production build so they cannot be
  googled or copied between players. The scaffold uses plain strings for clarity.
- **Hint ladder, not auto-solve.** Progressive nudges; the flag is never given.
- **One primitive per flag.** Each challenge isolates a single concept.

## Firmware architecture

ESP-IDF + NimBLE (not Bluedroid): portable across esp32/s3/c3/c6, full BLE5
peripheral support (extended advertising, coded PHY, LESC, L2CAP CoC), smaller
footprint.

```
main.c       nimble init, GAP, advertising, SMP config, app_main
gatt_svc.c   the 0xFF00 GATT service + per-characteristic access callbacks
flags.c      declarative flag registry + scoreboard + submit + adv payload
flags.h      blectf_flag_t table type and tiers
blectf.h     shared interface between the modules
```

Adding a flag is a table row plus the mechanic that reveals it - see
`07-adding-flags.md`.

## BLEtterCap integration (the differentiator)

- **Packet X-ray**: after a capture, replay the exact ATT/SMP/L2CAP exchange on
  the live traffic graph.
- **Sniffer-required flags**: values that exist only over the air, forcing use of
  the analyzer.
- **Attack + defense**: show sniffed plaintext (legacy pairing) next to
  LESC-protected traffic to teach *why* encryption matters.

## Non-goals

- No dependency on deprecated `gatttool`.
- No moving the target to nRF: the ESP32 covers the peripheral side; nRF is only
  the optional client probe.
