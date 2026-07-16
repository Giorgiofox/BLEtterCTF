# Full Walkthrough and Test Report

This page is the real thing: BLEtterCTF flashed onto an actual ESP32-C6 and every
flag exercised on hardware, **without BLEtterCap**, using only standard Linux BLE
tooling (`bleak` over a BlueZ adapter). Each flag lists the method, the exact
operation, and the observed result, verified against the on-device scoreboard.

**Result: 20/20 implemented flags captured in a single automated run** (device
scoreboard climbed `0 -> 20/20` in real time), from a freshly erased board, using
only `bleak` and a BlueZ pairing agent. No BLEtterCap involved. (The full catalog
is 38 flags; the remaining 18 are advanced tiers - see the [Flag Catalog](Flag-Catalog).)

If you just want to learn the mechanics, read the per-tier walkthroughs first.
This page proves they actually work on hardware.

## Test environment

| Piece | Detail |
|-------|--------|
| Target | ESP32-C6, firmware `bletterctf` (ESP-IDF v5.2.1 + NimBLE), BLE MAC `14:C1:9F:E5:A6:5A`, advertises `BLEtterCTF` |
| Central (solver) | Linux host, BlueZ adapter `hci0`, Python `bleak` |
| Pairing (flag 21) | same `hci0` adapter + a `NoInputNoOutput` BlueZ agent (registered via D-Bus by the solver) |
| BLEtterCap | **not used** - every flag solved with standard tools |
| Scoreboard | characteristic `0xFF01` (read + notify), `flags captured N/20` |

Firmware built and flashed with:

```sh
idf.py set-target esp32c6
idf.py build
idf.py -p /dev/ttyACM2 flash        # serial-bound to the target board only
# -> "BLEtterCTF up: 20 flags implemented"
```

## Verification method (important)

Flags are submitted by writing the flag string to `0xFF02`. To confirm a capture,
**subscribe to the scoreboard notifications on `0xFF01`** rather than reading it:
BlueZ caches GATT reads, so a plain re-read of the score can return a stale value
even after a successful submit. The firmware pushes an updated score notification
on every new capture, which is the reliable signal. The reference tester
(`tools/solver-examples/solve_bleak.py`) uses this approach.

## Results

Scan (active) found the target and, before any connection, already leaked three
flags in the advertising data (advertising is public - that is the lesson):

```
target: 14:C1:9F:E5:A6:5A  name=BLEtterCTF   (advertised name is Shortened)
  ADV  mfg 0xFFFF          -> "advf1337"   (flag 15)
  RSP  mfg 0xFFFE          -> "scan1337"   (flag 16, active scan only)
  RSP  service data 0xFF00 -> "svc1337"    (flag 17, active scan only)
```

Then, connected over `hci0` with `bleak`, walking one flag at a time. Every line
below is the real observed capture and the resulting device score:

| # | Tier | Method | Result | Score |
|---|------|--------|--------|:-----:|
| 1 | T0 | read `0xFF03` | `flag{welcome_you_absolute_legend}` | 1/20 |
| 2 | T0 | read `0xFF07` | `flag{plain_and_simple}` | 2/20 |
| 3 | T0 | read the `0x2901` descriptor of `0xFF04` | `flag{descriptors_are_not_decor}` | 3/20 |
| 4 | T0 | read the full device name at `0xFF1A` | `flag{names_can_be_longer}` | 4/20 |
| 5 | T0 | read `0xFF08`, hex-decode | `flag{hex_is_just_base16}` | 5/20 |
| 6 | T0 | read `0xFF09`, base64-decode | `flag{base64_is_not_encryption}` | 6/20 |
| 7 | T0 | read `0xFF0A`+`0xFF0B`+`0xFF0C`, concatenate | `flag{teamwork_dream_work}` | 7/20 |
| 8 | T0 | write `d34dbeef` to `0xFF05`, read it back | `flag{write_then_read_classic}` | 8/20 |
| 9 | T0 | write `1`,`2`,`3` in order to `0xFF0D`, read | `flag{state_machines_hold_grudges}` | 9/20 |
| 10 | T1 | subscribe `0xFF06` (notify) | `flag{notifications_never_sleep}` | 10/20 |
| 11 | T1 | subscribe `0xFF0E` (indicate) | `flag{indicate_wants_an_ack}` | 11/20 |
| 12 | T1 | subscribe `0xFF0F`, reassemble chunks | `flag{reassembly_required}` | 12/20 |
| 13 | T1 | subscribe `0xFF15`, then write to trigger the notify | `flag{write_then_it_talks}` | 13/20 |
| 15 | T2 | manufacturer data in the advertisement | `advf1337` | 14/20 |
| 16 | T2 | active scan, scan-response manufacturer data | `scan1337` | 15/20 |
| 17 | T2 | active scan, scan-response service data (0xFF00) | `svc1337` | 16/20 |
| 21 | T3 | Just Works pairing, then read encrypted `0xFF13` | `flag{encryption_is_a_feature}` | 17/20 |
| 24 | T3 | read encrypted `0xFF17` over the bonded link | `flag{bonds_outlive_connections}` | 18/20 |
| 35 | T6 | empty (zero-length) write to `0xFF19`, then read | `flag{empty_writes_are_edge_cases}` | 19/20 |
| 36 | T6 | full read (Read Blob) of `0xFF14` | `flag{read_blobs_are_a_thing_yeah}` | 20/20 |

The scoreboard climbed `1 -> 20` in real time over the `hci0` connection,
confirming the whole implemented CTF is solvable with nothing but a laptop and
`bleak` (plus a BlueZ agent for the pairing flags).

## Flag 21 - the security tier

`0xFF13` is marked read-encrypted, so it cannot be read until the link is paired
and encrypted. Two facts were verified:

1. **The gate works.** Reading `0xFF13` on an unpaired link is rejected by the
   peripheral (insufficient authentication/encryption). The flag is genuinely
   protected, not just hidden.
2. **After pairing, the flag reads out.** Observed value:
   `flag{encryption_is_a_feature}`, taking the scoreboard to `15/15`.

**The Linux gotcha that matters:** on BlueZ, `client.pair()` (or an encrypted
read) fails with `org.bluez.Error.AuthenticationFailed` **unless a pairing agent
is registered**. Just Works pairing still needs *something* to authorize it. Once
the solver registers a `NoInputNoOutput` agent over D-Bus, the same `hci0` adapter
pairs and reads `0xFF13` cleanly. This is not a firmware issue and not a dongle
issue - it is BlueZ requiring an agent.

`tools/solver-examples/solve_bleak.py` now registers this agent automatically
(best-effort, Linux only; macOS/Windows pair natively), so a single run captures
all 15 including the security flag.

Equivalent manual routes:

- **Phone + nRF Connect:** connect, tap read on `0xFF13`, accept the pairing
  prompt, read the flag, write it to `0xFF02`.
- **`bluetoothctl`:** register an agent (`agent NoInputNoOutput` / `default-agent`)
  in a live session, then `pair` and read `0xFF13`.

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
