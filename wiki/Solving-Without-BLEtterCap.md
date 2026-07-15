# Solving Without BLEtterCap

BLEtterCap gives you a lovely X-ray of the packets, but it is entirely optional.
This page sets up the three standard toolkits that solve the core game on their
own. Pick one (or learn all three, future you will thank present you).

| Toolkit | Platform | Best for | Weak at |
|---------|----------|----------|---------|
| **nRF Connect** (app) | iOS / Android | pairing flags (T3), beginners | scripting |
| **bleak** (Python) | Win / macOS / Linux | scripting, T0-T2 | pairing (T3) |
| **bluetoothctl** (BlueZ) | Linux | everything, advanced tiers | ergonomics |

> `gatttool` is deprecated in modern BlueZ. Do not use it. Use the tools below.

---

## Toolkit 1: nRF Connect (phone app)

The friendliest way to start, and the best tool for the security tier because the
phone handles pairing prompts natively.

1. Install **nRF Connect for Mobile** (Nordic Semiconductor), free.
2. **Scanner** tab -> find `BLEtterCTF` -> **Connect**.
3. Expand the `0xFF00` service to see the characteristics.
4. Each characteristic has icons: down-arrow = **read**, up-arrow = **write**,
   triple-arrow = **subscribe** (notify/indicate).
5. To submit a flag: tap the write icon on **0xFF02**, choose **Text** format,
   type the flag, send.
6. To check the score: read **0xFF01**, or subscribe to it.

Reading and writing text vs bytes: nRF Connect lets you pick the format. Flags are
text; some challenge values are raw bytes (hex) you must decode yourself.

---

## Toolkit 2: bleak (Python, cross-platform)

Scriptable and works everywhere. Install:

```sh
pip install bleak
```

Minimal building blocks:

```python
import asyncio
from bleak import BleakClient, BleakScanner

def u(x): return f"0000{x:04x}-0000-1000-8000-00805f9b34fb"
SCORE, SUBMIT = u(0xFF01), u(0xFF02)

async def main():
    dev = await BleakScanner.find_device_by_name("BLEtterCTF", timeout=10)
    async with BleakClient(dev) as c:
        val = await c.read_gatt_char(u(0xFF03))          # read a characteristic
        print("read:", val)
        await c.write_gatt_char(SUBMIT, val, response=True)  # submit a flag
        print("score:", (await c.read_gatt_char(SCORE)).decode())

asyncio.run(main())
```

Subscribing to notifications:

```python
def cb(handle, data): print("notify:", data)
await c.start_notify(u(0xFF06), cb)
await asyncio.sleep(3)
await c.stop_notify(u(0xFF06))
```

A complete reference solver for the scaffold flags lives in the repo at
`tools/solver-examples/solve_bleak.py`.

**Caveat:** `bleak` has limited/uneven pairing support across platforms. For the
Tier 3 security flags, use nRF Connect or `bluetoothctl` instead.

---

## Toolkit 3: bluetoothctl (Linux / BlueZ)

The most capable, especially for the advanced tiers (pairing, L2CAP, address
control). Start the interactive shell:

```sh
bluetoothctl
```

Core commands:

```
scan on                       # discover devices (Ctrl-C or 'scan off' to stop)
# note the address of BLEtterCTF, e.g. AA:BB:CC:DD:EE:FF
connect AA:BB:CC:DD:EE:FF
menu gatt
list-attributes              # enumerate services/characteristics/descriptors
select-attribute <uuid|path> # pick a characteristic
read                         # read the selected characteristic
write "0x64 0x33 ..."        # write bytes to the selected characteristic
notify on                    # subscribe to notifications/indications
back
```

Pairing (Tier 3):

```
pairable on
pair AA:BB:CC:DD:EE:FF        # Just Works needs no input for flag 21
trust AA:BB:CC:DD:EE:FF       # bond persists across reconnects
```

Writing text as bytes: BlueZ `write` wants space-separated hex. To turn a flag
string into hex quickly:

```sh
echo -n "flag{...}" | xxd -p -c1 | sed 's/^/0x/' | tr '\n' ' '
```

For L2CAP CoC (Tier 6) you drop below `bluetoothctl` to raw L2CAP sockets; see
[Advanced Tiers](Advanced-Tiers).

---

## Which tool per tier

| Tier | Easiest tool |
|------|--------------|
| T0 GATT base | any of the three |
| T1 notify / indicate | any of the three |
| T2 advertising (legacy) | any scanner; parse mfg / scan-response data |
| T2 extended / periodic | nRF probe or ButteRFly sniffer |
| T3 pairing / bonding / LESC | nRF Connect or bluetoothctl |
| T4 privacy / RPA | ButteRFly sniffer |
| T5 PHY (2M / coded) | nRF probe (+ sniffer to confirm) |
| T6 L2CAP CoC | macOS CoreBluetooth, Linux L2CAP socket, or nRF probe |

Ready? Start capturing: **[Tier 0 - GATT Basics](Walkthrough-Tier-0-GATT-Basics)**.
