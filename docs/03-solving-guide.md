# Solving guide

## The loop

1. Find the target: scan for a device named `BLEtterCTF`.
2. Connect and enumerate its GATT services and characteristics (0xFF00 range).
3. Extract a flag using whatever mechanic the challenge exposes.
4. Submit it: write the flag string to the submit characteristic **0xFF02**.
5. Check your score: read (or subscribe to) **0xFF01** -> `flags captured N/M`.

Flags are submitted as raw strings. Reading `0xFF01` after each submit confirms
whether it counted.

## Tools

Pick whichever fits your platform. The core tiers work with all of them.

### BLEtterCap web UI (recommended)
Guided play with the packet X-ray view: solve a flag, then see the ATT/SMP/L2CAP
exchange on the live graph. No CLI needed.

### nRF Connect (phone app)
Best for pairing/bonding flags (T3): the phone handles Passkey / Numeric
Comparison prompts natively. Enumerate, read, write, and subscribe from the UI.

### bleak (Python, cross-platform)
Scriptable. See `tools/solver-examples/solve_bleak.py`. Good for T0-T2; note
`bleak` has limited pairing support (use nRF Connect or BlueZ for T3).

### BlueZ (Linux)
Most flexible for the advanced tiers: `bluetoothctl` for pairing/bonding, L2CAP
sockets for T6 CoC, `btmgmt` for some address control.

> `gatttool` is deprecated in modern BlueZ. Prefer `bluetoothctl`, `bleak`, or
> the BLEtterCap UI.

## Which tool per tier

| Tier | Easiest tool |
|------|--------------|
| T0 GATT base | bleak / BLEtterCap / nRF Connect |
| T1 notify/indicate | bleak / BLEtterCap / nRF Connect |
| T2 advertising (legacy) | any scanner; parse mfg/scan-resp data |
| T2 ext/periodic adv | nRF probe or ButteRFly sniffer |
| T3 pairing/bonding/LESC | nRF Connect (phone) or BlueZ |
| T4 privacy / RPA | ButteRFly sniffer |
| T5 PHY (2M/coded) | nRF probe (+ sniffer to confirm) |
| T6 L2CAP CoC | macOS CoreBluetooth, Linux L2CAP socket, or nRF probe |

## Hints, not answers

BLEtterCTF ships a hint ladder instead of an auto-solver: each challenge exposes
progressive nudges (what to look at, then how) but never the flag itself. The
firmware also stores a one-line hint per flag (see the `hint` field in
`firmware/esp32/main/flags.c`) surfaced by the BLEtterCap UI.

## Worked example: T0 gift flag

```sh
# with bleak (see the full script in tools/solver-examples/)
# 1. read 0xFF03  -> "flag{welcome_bletterctf}"
# 2. write that string to 0xFF02
# 3. read 0xFF01  -> "flags captured 1/5"
```
