# Walkthrough - Tier 2: Advertising

> Beginner-proof, same format. You do not need to have finished the earlier tiers
> to understand this one, but it helps.

## The big idea: reading a device without connecting

Everything so far needed a **connection**. But BLE devices spend most of their
life *not connected* - they just periodically shout tiny packets into the air so
that phones and laptops can discover them. Those packets are called
**advertisements**, and anyone in range can listen. No connection, no permission.

This is how a beacon in a shop tells your phone "I am here", how your headphones
get noticed, and - importantly for security - how a surprising amount of data
leaks into the air for free. This tier is about harvesting that data.

## How advertising works (from zero)

- A **Peripheral** broadcasts advertisement packets on 3 special radio channels
  (37, 38, 39) every so often.
- Each packet is **at most 31 bytes** of payload. Tiny. This size limit shapes
  everything below.
- The payload is a list of **AD structures**, each a `(length, type, value)`
  triple. Common AD types: **Flags**, **Complete/Shortened Local Name**,
  **Manufacturer Specific Data**, **Service Data**.

**Scanning** comes in two kinds, and the difference is a whole flag:

- **Passive scan**: you just listen to the advertisements.
- **Active scan**: after hearing an advertisement, your radio sends a **SCAN_REQ**
  ("got anything else?"), and the device may reply with a **SCAN_RESP** - a
  *second* 31-byte packet. A passive scanner never sees the scan response.

So a device has up to **two** 31-byte packets to give you: the advertisement
(always) and the scan response (only if you ask, i.e. active scan). Most tools
(`bleak`, nRF Connect, `bluetoothctl`) do active scanning by default.

BLEtterCTF hides one flag in the advertisement and two more in the scan response,
precisely to teach this split.

## Capturing advertising data with your tool

In `bleak` you read advertising data during the scan itself, before connecting:

```python
from bleak import BleakScanner
devices = await BleakScanner.discover(timeout=10, return_adv=True)
for d, adv in devices.values():
    if (adv.local_name or "").startswith("BLEtterCTF"):
        print("mfg  :", adv.manufacturer_data)   # {company_id: bytes}
        print("svc  :", adv.service_data)         # {uuid: bytes}
```

- nRF Connect: tap the device in the Scanner tab (do **not** connect) and expand
  the **raw** advertising data.
- `bluetoothctl`: `scan on` prints `ManufacturerData` / `ServiceData` lines.

---

## Flag 15 - hidden in plain sight: manufacturer data

**The idea.** The **Manufacturer Specific Data** AD field is a free-for-all: a
2-byte company id followed by whatever bytes the vendor wants. Beacons abuse it
constantly to broadcast state. It is completely public - no connection needed.

**How BLE does it.** In the advertisement, BLEtterCTF includes manufacturer data
with company id `0xFFFF` (the "test/unassigned" id) and the flag bytes after it.
Any scanner sees it.

**Do it.** Grab it from the scan; company `0xFFFF`:

```python
for d, adv in (await BleakScanner.discover(timeout=8, return_adv=True)).values():
    if (adv.local_name or "").startswith("BLEtterCTF"):
        flag = adv.manufacturer_data.get(0xFFFF)   # b"advf1337"
```

Then connect once and `submit(client, flag)` (submitting is a GATT write, so it
needs a connection - but *finding* the flag did not).

**What you learned.** Advertising data is public. Never put anything secret in it.
(This flag is secret on purpose, to show you that real products do exactly this.)
Note it is a short token, not a long `flag{...}` - because 31 bytes is tiny, which
is the whole reason the fields below are short too.

---

## Flag 16 - ask nicely: the scan response

**The idea.** Some data is only handed out to a scanner that actively asks. If you
only passively listen, you never see it. The difference between passive and active
scanning is a real reconnaissance skill.

**How BLE does it.** This flag's manufacturer data (company `0xFFFE`) is in the
**scan response**, not the advertisement. Your radio only receives it after
sending a SCAN_REQ, i.e. during an **active** scan.

**Do it.** `bleak`'s `discover` active-scans by default, so it is already in the
same `manufacturer_data`, under company `0xFFFE`:

```python
flag16 = adv.manufacturer_data.get(0xFFFE)   # b"scan1337"
```

To *prove* the point, a purely passive scanner would return `None` here. On nRF
Connect it appears once the device answers the scan request.

**What you learned.** What a device reveals depends on **how you ask**. Passive
and active scans see different things - security tools rely on this constantly.

---

## Flag 17 - typed for a service: service data

**The idea.** Besides "manufacturer data" (vendor free-for-all), advertising has
**Service Data**: bytes tagged to a specific **service UUID**. It says "this data
belongs to service X". Fitness and sensor beacons use it a lot.

**How BLE does it.** The scan response also carries a **Service Data** AD field
tagged with UUID `0xFF00` (BLEtterCTF's service), followed by the flag bytes. It
is a different AD *type* from manufacturer data - same idea (public bytes in the
advertising), different labeling convention.

**Do it.** In `bleak`, service data comes in `adv.service_data`, keyed by the full
UUID string:

```python
flag17 = adv.service_data.get(u(0xFF00))     # b"svc1337"
```

**What you learned.** Advertising can carry several *typed* fields at once
(manufacturer data, service data, name, flags). Parse them all - each is a place
data can hide.

---

## The rest of Tier 2 (needs BLE 5 hardware)

The remaining Tier 2 flags use advertising features from **Bluetooth 5.0**, which
a typical laptop/phone radio cannot receive reliably:

- **Flag 18 - catch it in time:** the advertised value rotates over time; you must
  observe and catch a specific state (host-solvable, just needs patience).
- **Flag 19 - beyond legacy:** the flag rides in **extended advertising** (BLE 5
  packets with much bigger payloads than 31 bytes - remember how cramped this tier
  felt? this is the fix).
- **Flag 20 - sync up:** the flag is in **periodic advertising**; you must
  synchronize to a schedule to receive it.

These need an **nRF52840 dongle** or the **ButteRFly sniffer** - see
[Advanced Tiers](Advanced-Tiers) and [Hardware and Kit](Hardware-and-Kit).

---

## Tier 2 done (the host-solvable part)

You can now harvest data straight from the air with no connection, you understand
the 31-byte limit and why it forces short payloads, and you know why active and
passive scans differ. Next: convincing the device you are allowed to read its
*protected* data - **[Tier 3 - Security](Walkthrough-Tier-3-Security)**.
