# Solver examples

Reference solvers for BLEtterCTF. These are teaching aids for the core tiers, not
a full auto-solver (BLEtterCTF is meant to be played with the hint ladder).

## solve_bleak.py

Cross-platform Python solver using [bleak](https://github.com/hbldh/bleak).
Covers the scaffold flags (T0-T2): direct read, descriptor, write-to-unlock,
notify, and advertising manufacturer data.

```sh
pip install bleak
python solve_bleak.py            # scans for "BLEtterCTF" and walks the flags
```

Note: `bleak` has limited pairing support, so the T3 security flags are better
solved with the nRF Connect phone app or BlueZ `bluetoothctl`.
