# BLE Primer: everything you need before flag 1

This is the "cappello", the introduction that makes the rest of BLEtterCTF click.
Read it once now, skim it again whenever a flag confuses you. It is long, but so
is a good espresso queue, and this is more useful.

By the end you will know what BLE is, how its layers stack up, how devices find
and talk to each other, and how they (try to) keep it private. Every concept
here maps directly onto a flag.

---

## 1. What is BLE, and what it is not

**Bluetooth Low Energy (BLE)** is a short-range wireless technology designed to
run for months on a coin cell. It powers fitness bands, smart locks, beacons,
medical sensors, that temperature tag in your fridge, and roughly everything with
"smart" in the name.

BLE is **not** Bluetooth Classic. They share a brand and a radio band (2.4 GHz)
and almost nothing else. Classic Bluetooth streams your music; BLE sends tiny
bursts of data and then goes back to sleep to save power. They speak different
protocols. When people say "Bluetooth" today for gadgets, they usually mean BLE.

BLE was introduced in **Bluetooth 4.0 (2010)**. Everything in this CTF is BLE.

Key mental model: BLE is built for **small, infrequent data**. A characteristic
holding a heart rate is normal. Streaming a video over BLE is a war crime.

---

## 2. The BLE stack: layers, from radio to app

BLE is a layered protocol, like a cake where each layer only trusts the one
directly below it. From the bottom up:

```
+-------------------------------------------------------------+
|  Application  (your smart lock app, or a CTF player)        |
+-------------------------------------------------------------+
|  GAP            |  GATT                                     |   <- what you touch
|  (who am I,     |  (services / characteristics / values)   |
|   find/connect) |                                          |
+-----------------+------------------------------------------+
|  SMP (pairing / encryption)  |  ATT (read/write primitives)|
+-------------------------------------------------------------+
|  L2CAP  (channels, packet multiplexing)                     |
+-------------------------------------------------------------+
|  HCI    (host <-> controller boundary)                      |
+-------------------------------------------------------------+
|  Link Layer  (connections, advertising, timing, channels)   |
+-------------------------------------------------------------+
|  Physical Layer (PHY)  (the actual radio)                    |
+-------------------------------------------------------------+
```

Let's meet each layer. You will interact mostly with the top two (GAP and GATT),
but the flags reach all the way down.

### PHY - the radio
The Physical Layer is the radio turning bits into 2.4 GHz waves. BLE splits the
band into **40 channels** of 2 MHz each. Three of them (37, 38, 39) are reserved
for **advertising**; the other 37 carry **connection** data, hopping between
channels to dodge interference (your microwave and Wi-Fi live here too).

BLE has several PHYs:
- **1M PHY** - the original, 1 Mbit/s. The default everyone supports.
- **2M PHY** (BLE 5.0) - 2 Mbit/s, faster, slightly shorter range.
- **Coded PHY** (BLE 5.0) - trades speed for range using error-correction coding
  (**S=2** or **S=8**). This is "long range" mode. Great for a doorbell across a
  garden, terrible for throughput.

*Flags that live here:* T5 (2M PHY, coded PHY).

### Link Layer - timing and state
The Link Layer manages advertising, scanning, and connections: when to transmit,
which channel to hop to next, connection intervals, and so on. It also owns the
device **address** (more on that soon). You rarely talk to it directly; the
controller handles it.

### HCI - the host/controller border
BLE stacks are split into a **controller** (the radio + Link Layer, usually a
chip) and a **host** (L2CAP up to GATT, usually software). The **Host Controller
Interface** is the standardized border between them. On your laptop, the Bluetooth
chip is the controller and BlueZ / CoreBluetooth / Windows is the host. This
split is exactly why some things (coded PHY, setting your MAC) are possible on one
laptop and not another: it depends on what the controller exposes over HCI.

### L2CAP - channels
The Logical Link Control and Adaptation Protocol multiplexes higher-layer data
into channels. Most of the time it is invisible plumbing carrying ATT. But BLE
also offers **L2CAP Connection-Oriented Channels (CoC)**: a credit-based stream
channel for bulk-ish data that bypasses GATT entirely.

*Flags that live here:* T6 (L2CAP CoC).

### ATT - the read/write primitives
The Attribute Protocol is the simple request/response layer underneath GATT. An
**attribute** is just: a **handle** (a 16-bit address, like a house number), a
**type** (a UUID), and a **value**. ATT defines the raw operations: Read Request,
Write Request, Read Blob, Notification, Indication, and friends. GATT is a set of
conventions layered on top of ATT to give those attributes meaning.

If ATT is the alphabet, GATT is the grammar.

### GATT - services and characteristics
The **Generic Attribute Profile** is where you will spend most of the CTF. It
organizes attributes into a tidy hierarchy:

```
Service  (e.g. "Heart Rate", or our custom 0xFF00)
  |
  +-- Characteristic  (a single data point, e.g. "measurement")
  |     |
  |     +-- Value        (the actual bytes)
  |     +-- Descriptor   (metadata about the value)
  |
  +-- Characteristic ...
```

- A **Service** groups related characteristics.
- A **Characteristic** is one value plus its properties (can I read it? write it?
  will it notify me?).
- A **Descriptor** is metadata attached to a characteristic. The most famous are
  the **User Description (0x2901)**, a human label, and the **CCCD (0x2902)**, the
  switch you flip to turn notifications on.

Each of these has a **handle** (its address on the device) and a **UUID** (its
type). UUIDs come in two sizes:
- **16-bit** - reserved by the Bluetooth SIG for standard things (0x180D = Heart
  Rate Service, 0x2A37 = Heart Rate Measurement). Shorthand for a full 128-bit
  UUID with a standard base.
- **128-bit** - anyone's custom UUID. BLEtterCTF uses the 16-bit **0xFF00** range
  because it is short and readable.

**Characteristic properties** are the verbs a characteristic allows:
- **Read** - client pulls the value.
- **Write** - client sets the value (with acknowledgement).
- **Write Without Response** - fire and forget, no ACK.
- **Notify** - server pushes the value, no ACK (fast, lossy-tolerant).
- **Indicate** - server pushes the value and waits for an ACK (slower, reliable).

*Flags that live here:* T0 (read/write/descriptors), T1 (notify/indicate).

### GAP - identity and connections
The **Generic Access Profile** governs how devices present themselves and
connect: advertising, scanning, device name, appearance, and the connection
lifecycle. GAP defines **roles**:
- **Broadcaster** - only advertises, never connects (a beacon).
- **Observer** - only scans, never connects (a scanner).
- **Peripheral** - advertises and accepts one connection (the ESP32 here).
- **Central** - scans and initiates connections (your laptop/phone).

*Flags that live here:* T2 (advertising), and the whole find-and-connect dance.

### SMP - security
The **Security Manager Protocol** handles **pairing** (agreeing on keys),
**bonding** (remembering them), and **encryption**. This is where "prove you are
allowed to read this" happens.

*Flags that live here:* T3 (pairing, bonding, LESC), T4 (privacy).

---

## 3. Client and server vs central and peripheral

A classic source of confusion, so let's kill it now. There are **two independent
role pairs**:

- **Central / Peripheral** (GAP): who initiated the connection. The central
  scans and connects; the peripheral advertises and accepts.
- **Client / Server** (GATT): who owns the data. The **server** holds the
  services and characteristics; the **client** reads and writes them.

Usually the peripheral is the server (the sensor holds the data) and the central
is the client (your phone reads it). That is the BLEtterCTF setup: the **ESP32 is
the peripheral and GATT server**, your **laptop/phone is the central and GATT
client**. But the two pairings are orthogonal, and advanced setups mix them.

---

## 4. Addresses: how a device is named

Every BLE device has a 48-bit **address** (like a MAC). There are several kinds,
and this matters for privacy flags:

- **Public** - a globally unique, permanent address (bought from the IEEE).
- **Random static** - random, but stable until reboot. Cheap and common.
- **Resolvable Private Address (RPA)** - changes every ~15 minutes to stop
  trackers following you. Only a device holding your **Identity Resolving Key
  (IRK)** can tell that two different RPAs are actually the same device. This is
  BLE's privacy feature, and it is why you cannot just track a modern phone by its
  advertising address.

*Flags that live here:* T4 (RPA, identity, directed advertising).

---

## 5. Advertising: how devices are found

Before any connection exists, a peripheral **advertises**: it periodically shouts
small packets on channels 37, 38, 39. Each advertising packet is tiny -
**31 bytes** of payload in legacy advertising - carrying **AD structures**, each a
(length, type, value) triple. Common AD types:
- **Flags** - basic capability bits.
- **Complete Local Name** - the device name (`BLEtterCTF`).
- **Manufacturer Specific Data** - a company id plus arbitrary bytes. Beacons
  abuse this constantly.
- **Service Data** - data tagged to a specific service UUID.

**Scanning** comes in two flavours:
- **Passive scan** - just listen to advertisements.
- **Active scan** - after hearing an advertisement, send a **SCAN_REQ** asking
  "got anything else?". The device may reply with a **SCAN_RESP**, a second
  31-byte packet. This is how a device advertises more than 31 bytes without a
  connection. A passive scanner never sees the scan response.

*Flag hint that pays off later:* if a flag lives in the scan response, a passive
scan will not find it. You must scan actively.

BLE 5.0 added bigger and fancier advertising:
- **Extended Advertising** - advertise on the data channels with much larger
  payloads (up to ~255 bytes, chained further), on the 1M/2M/coded PHYs.
- **Periodic Advertising** - a precisely timed broadcast that a scanner can
  **sync** to, receiving data on a schedule without connecting. Great for
  broadcasting to many listeners at once.

*Flags that live here:* T2 (manufacturer data, scan response, extended, periodic).

---

## 6. Connections: what changes once you connect

When a central connects to a peripheral, they agree on **connection parameters**:
- **Connection interval** - how often they wake up to talk (7.5 ms to 4 s).
- **Slave latency** - how many intervals the peripheral may skip if it has
  nothing to say (saves power).
- **Supervision timeout** - how long before they declare the link dead.

They also negotiate the **MTU** (Maximum Transmission Unit), the largest ATT
payload per packet. The default is a stingy **23 bytes** (so 20 bytes of usable
value after overhead). Want to move more than 20 bytes in one go? Either raise the
MTU, or use:
- **Read Blob** - read a long value in chunks (the client keeps asking for the
  next offset).
- **Prepare / Execute Write (long write)** - queue chunks, then commit them.

*Flag hint:* if a value is longer than the MTU and you only read the first 20-ish
bytes, you truncated the flag. Read the whole thing.

An important gotcha for this CTF: **MTU, connection interval, and PHY are usually
chosen by the operating system, not exposed to your app.** That is why BLEtterCTF
never asks you to "set the MTU to X" (the original ble_ctf did, and everyone
hated it). Instead, the advanced flags ask you to **observe** these values with a
sniffer.

---

## 7. Security: pairing, bonding, encryption, privacy

BLE security is a menu, and different flags order different dishes.

**Pairing** is the process of two devices agreeing on encryption keys. There are
several **association models**, chosen by each side's input/output capabilities:
- **Just Works** - no user interaction. Encrypts the link but offers **no
  protection against a man-in-the-middle (MITM)**. Fine for a lightbulb.
- **Passkey Entry** - one side shows a 6-digit number, the other types it. MITM
  protection.
- **Numeric Comparison** (LESC only) - both sides show a number, the user
  confirms they match. MITM protection, no typing.
- **Out Of Band (OOB)** - keys exchanged over another channel (NFC, a printed
  code). MITM protection if that channel is secure.

**Legacy pairing vs LE Secure Connections (LESC):**
- **Legacy** (BLE 4.0-4.1) uses a weaker key exchange. A sniffer that catches the
  pairing can often derive the key and decrypt everything. Yikes.
- **LESC** (BLE 4.2+) uses **ECDH** (elliptic-curve Diffie-Hellman), so a sniffer
  cannot derive the key even if it catches the whole exchange. Always prefer LESC.

*A flag literally shows you this:* sniff a legacy-paired characteristic (plaintext
to the sniffer) next to a LESC-protected one (gibberish). That is the whole point
of encryption in one screenshot.

**Bonding** is pairing plus "remember me": the keys are stored so the next
connection skips pairing and just encrypts. Bond once, reconnect forever (until
someone clears the bond).

**Encryption** protects a link once keys exist. A characteristic can require it:
try to read an encrypted-only characteristic without pairing and the server
refuses (an "Insufficient Authentication/Encryption" error), which triggers your
OS to start pairing.

---

## 8. BLE versions at a glance

Each Bluetooth Core version added features. You do not need to memorize this, but
it explains why an old laptop cannot do half of Tier 5.

| Version | Year | Headline additions |
|---------|------|---------------------|
| 4.0 | 2010 | BLE is born: GATT, ATT, GAP, advertising, 1M PHY |
| 4.1 | 2013 | Better coexistence, dual-role devices |
| 4.2 | 2014 | **LE Secure Connections (LESC)**, privacy (RPA), larger MTU, data length extension |
| 5.0 | 2016 | **2M PHY**, **Coded PHY (long range)**, **extended + periodic advertising** |
| 5.1 | 2019 | Direction finding (AoA/AoD), GATT caching improvements |
| 5.2 | 2020 | **LE Audio**, Isochronous channels, enhanced power control |
| 5.3 | 2021 | Connection subrating, better periodic advertising, channel classification |
| 5.4 | 2023 | PAwR (periodic advertising with responses), encrypted advertising data |

BLEtterCTF target chips:
- **ESP32 (classic)** speaks BLE 4.2 - core tiers only.
- **ESP32-C3 / S3** speak BLE 5.0 - unlock extended advertising and PHY flags.
- **ESP32-C6** speaks BLE 5.3 - the full menu.

---

## 9. How this maps onto BLEtterCTF

Here is the whole primer folded back into the game:

| You learned about | Tier | You will |
|-------------------|------|----------|
| GATT read/write, descriptors, handles, UUIDs | T0 | enumerate and read characteristics |
| Notify / Indicate / CCCD | T1 | subscribe and receive pushes |
| Advertising, scan response, ext/periodic | T2 | scan (passively and actively) |
| Pairing, bonding, LESC | T3 | pair to unlock encrypted characteristics |
| Addresses, RPA, IRK | T4 | deal with privacy and directed advertising |
| PHYs, connection parameters | T5 | observe or use 2M / coded PHY |
| L2CAP CoC, caching, long reads/writes | T6 | open channels and move bigger data |

You now know more about BLE than most people shipping BLE products. Slightly
terrifying, isn't it? Go capture some flags:
**[Getting Started](Getting-Started)**.
