# Advanced Tiers (T4-T6)

Welcome to the deep end. These tiers cover privacy, PHYs, and L2CAP, the parts of
BLE 5 that a plain laptop or phone radio simply cannot do. Not because you are not
skilled enough, but because the **controller** in a typical laptop does not expose
these capabilities over HCI (see the [primer](BLE-Primer), section 2).

So here the **client** hardware is the gate, not the target. Two cheap add-ons
open it:

- **ButteRFly sniffer** - for **observe-only** flags: you watch what is on the
  air but do not need to actively control your radio.
- **nRF52840 USB dongle (~10 EUR)** - flashed as the BLEtterCTF probe, for flags
  that need you to **control** the radio (coded PHY, periodic sync, set your
  address, L2CAP). See [Hardware and Kit](Hardware-and-Kit).

Neither requires BLEtterCap. The sniffer works with Wireshark; the nRF dongle
works from a script.

---

## T4 - Privacy

The lesson of this tier: modern BLE tries hard to stop you from tracking devices,
and understanding how is half of understanding BLE security.

- **Flag 27 - same device, new address (RPA):** the target periodically changes
  its **Resolvable Private Address**. With a sniffer you watch the address rotate;
  the flag is proving you can tell two different RPAs are the same device using the
  **IRK**. This is exactly the anti-tracking mechanism in your phone.
- **Flag 28 - for your eyes only (directed advertising):** the device sends
  **directed advertisements** aimed at one specific address. Only a client using
  that address receives them, which is why you need the nRF probe to set your
  address.
- **Flag 29 - show your identity (address filtering):** a redesign of the classic
  ble_ctf "spoof your MAC" flag. The device only reveals the flag to a client
  presenting a specific identity. Set your probe's address to match.

Tools: ButteRFly (flag 27), nRF probe (flags 28, 29).

---

## T5 - PHY and BLE 5.x

The lesson: BLE 5 is not one radio, it is several, and range/speed is a choice.

- **Flag 30 - double speed (2M PHY):** a characteristic is only served when the
  connection is running on the **2M PHY**. Your probe requests the PHY update.
- **Flag 31 - long range (coded PHY):** the flag is only transmitted on the
  **LE Coded PHY (S=8)**. Commodity radios cannot receive coded PHY at all; the
  nRF probe (or a capable sniffer) can.
- **Flag 32 - read the timing (connection parameters):** you do **not** set
  anything here. You **observe** the negotiated connection interval / latency with
  the sniffer and report it. This is deliberate: hosts rarely let apps set these,
  so BLEtterCTF makes them an observation, not a fight.

Tools: nRF probe (flags 30, 31), ButteRFly (flags 31 observe, 32).

---

## T6 - L2CAP, caching, and the capstone

The lesson: there is more to BLE than GATT.

- **Flag 33 - open a channel (L2CAP CoC):** connect an **L2CAP
  Connection-Oriented Channel** to the device's PSM and exchange data over a
  credit-based stream, bypassing GATT entirely.
  - macOS: CoreBluetooth `CBL2CAPChannel`.
  - Linux: an `AF_BLUETOOTH` `SOCK_SEQPACKET` / `BTPROTO_L2CAP` socket to the PSM.
  - nRF probe: the probe's L2CAP client.
- **Flag 34 - the database moved (GATT caching):** the services change between
  connections. A client that blindly trusts a cached database misses the new flag;
  you must re-discover (or honour **Service Changed** / the database hash). Host
  solvable, but sneaky.
- **Flag 35 - break it gently (safe fuzzing):** a characteristic reveals the flag
  only on a malformed or edge-length write (e.g. an empty write, or one longer
  than expected). Teaches input-validation testing without breaking anything.
- **Flag 36 - too big for one bite (long read):** the value is longer than one
  MTU, so a naive read truncates it. Do a full read (**Read Blob**) to get it all.
  *(This one is host-solvable and already implemented; see the catalog.)*
- **Flag 37 - the capstone:** chain several skills - sniff something, pair, and
  reassemble a notification - into one final flag. The graduation exam.
- **Flag 38 - look outside (OSINT):** the answer is not on the device. Recon
  skills, not radio skills.

Tools: mixed. See the [Flag Catalog](Flag-Catalog) for the per-flag matrix.

---

## Do I need all this hardware?

No. The **core game (T0-T3) is complete without any of it.** The advanced tiers
are there for when you want the full BLE 5 picture, and the two add-ons together
cost less than a nice dinner. Buy them when you are ready, not before.

Back to the map: **[Home](Home)** or the full **[Flag Catalog](Flag-Catalog)**.
