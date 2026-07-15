# Walkthrough - Tier 2: Advertising

Plot twist: you can read data from a BLE device **without connecting to it at
all**. Devices constantly broadcast small packets called **advertisements**, and
anyone in range can listen. This is how beacons work, how your phone notices your
headphones, and how a surprising amount of data leaks into the air for free.

Recap from the [primer](BLE-Primer): advertisements are up to 31 bytes of
(length, type, value) AD structures on channels 37/38/39. Two flavours of
scanning: **passive** (just listen) and **active** (listen, then send a SCAN_REQ
to ask for a **scan response**, a second 31-byte packet).

---

## Flag 15 - hidden in plain sight (manufacturer data)

No connection needed. Just scan and read the **Manufacturer Specific Data** AD
field. The device broadcasts a company id (`0xFFFF`, the test/unassigned id)
followed by the flag bytes.

```python
from bleak import BleakScanner
devs = await BleakScanner.discover(timeout=8, return_adv=True)
for d, adv in devs.values():
    if adv.local_name == "BLEtterCTF":
        for cid, payload in adv.manufacturer_data.items():
            print(hex(cid), payload)   # 0xffff -> the flag
```

- nRF Connect: tap the device in the Scanner tab (do not connect) and expand the
  **raw** advertising data; find the manufacturer data.
- bluetoothctl: `scan on` prints `ManufacturerData` for the device.

The lesson: advertising data is public. Never put anything secret in it. (This
flag is secret *on purpose*, to teach you that people do exactly this in real
products. Please do not be those people.)

---

## Flag 16 - ask nicely (scan response)

This flag is **not** in the advertisement. It is in the **scan response**, which
you only receive if you do an **active scan**. A passive scan will never see it,
which is the entire lesson.

Most scanners do active scanning by default:

```python
# BleakScanner.discover uses active scanning by default; the flag rides in
# manufacturer_data under company id 0xFFFE (distinct from flag 15's 0xFFFF)
```

- nRF Connect: active scanning is on by default; look for a second manufacturer
  data entry (company `0xFFFE`).
- bluetoothctl: uses active scanning; the extra data appears once the device
  answers a SCAN_REQ.

The lesson: what a device reveals depends on **how you ask**. Passive listeners
and active probers see different things. Security tools rely on this distinction
constantly.

---

## The rest of Tier 2 (needs BLE 5 hardware)

The remaining Tier 2 flags use BLE 5.0 advertising, which a typical laptop/phone
radio cannot receive reliably:

- **Flag 18 - catch it in time:** the advertised value rotates; you must observe
  over time and catch a specific state.
- **Flag 19 - beyond legacy:** the flag rides in **extended advertising** (bigger
  payloads on the data channels).
- **Flag 20 - sync up:** the flag is in **periodic advertising**; you must sync to
  the schedule to receive it.

These need an **nRF52840 dongle** or the **ButteRFly sniffer**. See
[Advanced Tiers](Advanced-Tiers) and [Hardware and Kit](Hardware-and-Kit).

---

## Tier 2 done (the host-solvable part)

You can now harvest data from the air with no connection, and you understand why
active and passive scans differ. Next: convincing the device you are allowed to
read its private characteristics:
**[Tier 3 - Security](Walkthrough-Tier-3-Security)**.
