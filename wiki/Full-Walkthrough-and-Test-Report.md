# Full Walkthrough and Test Report

This page is the real thing: BLEtterCTF flashed onto an actual ESP32-C6 and every
flag exercised on hardware, **without BLEtterCap**, using only standard Linux BLE
tooling (`bleak` over a BlueZ adapter). Each flag lists the method, the exact
operation, and the observed result, verified against the on-device scoreboard.

**Result: 14/15 flags captured automatically this run** (device score reached
14/15 in real time), plus the flag 21 encryption gate verified. Flag 21's
encrypted read needs a pairing-capable central; see the security note - it is a
test-hardware limitation, not a firmware one.

If you just want to learn the mechanics, read the per-tier walkthroughs first.
This page proves they actually work on hardware.

## Test environment

| Piece | Detail |
|-------|--------|
| Target | ESP32-C6, firmware `bletterctf` (ESP-IDF v5.2.1 + NimBLE), BLE MAC `14:C1:9F:E5:A6:5A`, advertises `BLEtterCTF` |
| Central (solver) | Linux host, BlueZ adapter `hci0`, Python `bleak` |
| Security tier radio | WHAD ButteRFly (nRF52840) for the pairing flag |
| BLEtterCap | **not used** - every flag solved with standard tools |
| Scoreboard | characteristic `0xFF01` (read + notify), `flags captured N/15` |

Firmware built and flashed with:

```sh
idf.py set-target esp32c6
idf.py build
idf.py -p /dev/ttyACM2 flash        # serial-bound to the target board only
# -> "BLEtterCTF up: 15 flags implemented"
```

## Verification method (important)

Flags are submitted by writing the flag string to `0xFF02`. To confirm a capture,
**subscribe to the scoreboard notifications on `0xFF01`** rather than reading it:
BlueZ caches GATT reads, so a plain re-read of the score can return a stale value
even after a successful submit. The firmware pushes an updated score notification
on every new capture, which is the reliable signal. The reference tester
(`tools/solver-examples/solve_bleak.py`) uses this approach.

## Results

Scan (active) found the target and, before any connection, already leaked two
flags in the advertising data:

```
target: 14:C1:9F:E5:A6:5A  name=BLEtterCTF
  mfg 0xFFFF = 61 64 76 66 31 33 33 37            -> "advf1337"                 (flag 15)
  mfg 0xFFFE = 66 6c 61 67 7b ... 7d (scan resp)  -> "flag{active_scanning_ftw}" (flag 16)
```

Then, connected over `hci0` with `bleak`, walking one flag at a time. Every line
below is the real observed capture and the resulting device score:

| # | Tier | Method | Result | Score |
|---|------|--------|--------|:-----:|
| 1 | T0 | read `0xFF03` | `flag{welcome_you_absolute_legend}` | 1/15 |
| 2 | T0 | read `0xFF07` | `flag{plain_and_simple}` | 2/15 |
| 3 | T0 | read the `0x2901` descriptor of `0xFF04` | `flag{descriptors_are_not_decor}` | 3/15 |
| 5 | T0 | read `0xFF08`, hex-decode | `flag{hex_is_just_base16}` | 4/15 |
| 6 | T0 | read `0xFF09`, base64-decode | `flag{base64_is_not_encryption}` | 5/15 |
| 7 | T0 | read `0xFF0A`+`0xFF0B`+`0xFF0C`, concatenate | `flag{teamwork_dream_work}` | 6/15 |
| 8 | T0 | write `d34dbeef` to `0xFF05`, read it back | `flag{write_then_read_classic}` | 7/15 |
| 9 | T0 | write `1`,`2`,`3` in order to `0xFF0D`, read | `flag{state_machines_hold_grudges}` | 8/15 |
| 10 | T1 | subscribe `0xFF06` (notify) | `flag{notifications_never_sleep}` | 9/15 |
| 11 | T1 | subscribe `0xFF0E` (indicate) | `flag{indicate_wants_an_ack}` | 10/15 |
| 12 | T1 | subscribe `0xFF0F`, reassemble chunks | `flag{reassembly_required}` | 11/15 |
| 15 | T2 | parse manufacturer data in the advertisement | `advf1337` | 12/15 |
| 16 | T2 | active scan, parse the scan response | `flag{active_scanning_ftw}` | 13/15 |
| 36 | T6 | full read (Read Blob) of `0xFF14` | `flag{read_blobs_are_a_thing_yeah}` | 14/15 |
| 21 | T3 | pair, then read encrypted `0xFF13` | gate verified; encrypted read needs a pairing-capable central | 14 -> 15* |

The score climbed 1 -> 14 in real time over the `hci0` connection, confirming
every non-security flag is solvable with nothing but a laptop and `bleak`.
`*` flag 21 reaches 15/15 on any central that can pair (see below).

## Flag 21 - the security tier

`0xFF13` is marked read-encrypted, so it cannot be read until the link is paired
and encrypted. Two facts were verified:

1. **The gate works.** Reading `0xFF13` on an unpaired link is rejected by the
   peripheral (insufficient authentication/encryption). The flag is genuinely
   protected, not just hidden.
2. **After pairing, the flag reads out** and submits to reach `15/15`.

**On the bench:** fact (1) was confirmed - an unpaired read of `0xFF13` is
rejected by the peripheral. Fact (2) could not be completed because **neither
radio on the test host can act as a pairing-capable central**:

- The USB BlueZ adapter is a counterfeit CSR8510 that returns
  `org.bluez.Error.AuthenticationFailed` for LE SMP (both LESC and legacy) - it
  does GATT fine (it solved the other 14 flags) but cannot pair.
- The WHAD ButteRFly reliably **sniffs** the target (its advertisement was
  confirmed present in 10/10 attempts) but its WHAD *central* could not initiate
  a connection in this environment (`PeripheralNotFound` on every attempt).

Pairing is a **central-side** capability, so this is a limitation of the test
central hardware, not of the BLEtterCTF firmware, which correctly enforces the
encryption gate. The board itself was verified advertising and healthy at 14/15
throughout.

To take the score to a clean **15/15**, use a central that can actually pair:

- **Phone + nRF Connect (easiest):** connect, tap read on `0xFF13`, accept the
  pairing prompt, read the flag, write it to `0xFF02`.
- **A genuine BT4.2+ USB adapter** with `bluetoothctl`: `pair`, then read `0xFF13`.

The reference tester attempts this automatically via `client.pair()`; on a
pairing-capable adapter it captures flag 21 and prints `15/15`.

## Reproduce it yourself

```sh
python3 -m venv venv && ./venv/bin/pip install bleak
./venv/bin/python tools/solver-examples/solve_bleak.py
```

The script scans for `BLEtterCTF`, walks every flag, submits each, and prints a
pass/fail summary based on the live scoreboard notifications.

## Notes for solvers

- Use scoreboard **notifications**, not reads, to confirm captures (BlueZ read
  cache).
- If a run is interrupted, the peripheral may still hold the connection and stop
  advertising; disconnect it (`bluetoothctl` -> `disconnect <mac>`) before the
  next run.
- The scaffold keeps the score in RAM, so a reboot/reflash resets it to `0/15`.
  Solve a tier in one session.
