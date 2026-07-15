# BLEtterCTF Wiki

Welcome to BLEtterCTF, a Bluetooth Low Energy Capture-The-Flag built for one
reason: to make you genuinely understand BLE, not just collect flags. You flash a
tiny ESP32, it starts whispering over the air, and you learn to listen, talk,
and eventually sweet-talk it into giving up its secrets.

No prior BLE knowledge required. If the phrase "GATT characteristic" currently
sounds like a fancy cheese, you are in exactly the right place. Start with the
primer.

> You do **not** need BLEtterCap to play. Every core flag is solvable with tools
> you already have: a laptop with `bluetoothctl` or Python `bleak`, or a phone
> running the free nRF Connect app. BLEtterCap adds an X-ray view of the packets,
> which is lovely, but it is optional.

## Start here

1. **[BLE Primer](BLE-Primer)** - what BLE is and how it is built. Read this first.
2. **[Getting Started](Getting-Started)** - what you need and how to begin.
3. **[Flashing the ESP32](Flashing-the-ESP32)** - blank board to `BLEtterCTF` in ten minutes.
4. **[Solving Without BLEtterCap](Solving-Without-BLEtterCap)** - the three standard toolkits.

## Walkthroughs

- **[Tier 0 - GATT Basics](Walkthrough-Tier-0-GATT-Basics)**
- **[Tier 1 - Notifications and Indications](Walkthrough-Tier-1-Notifications-and-Indications)**
- **[Tier 2 - Advertising](Walkthrough-Tier-2-Advertising)**
- **[Tier 3 - Security](Walkthrough-Tier-3-Security)**
- **[Advanced Tiers (T4-T6)](Advanced-Tiers)** - privacy, PHY, L2CAP; needs a sniffer or nRF dongle.

## Reference

- **[Flag Catalog](Flag-Catalog)** - the full challenge list.
- **[Hardware and Kit](Hardware-and-Kit)** - boards and the optional add-ons.
- **[Adding Flags](Adding-Flags)** - author your own challenges.
- **[FAQ](FAQ)** - stuck? start here.

## The rules of the game

- Find the target (it advertises as `BLEtterCTF`).
- Extract a flag using whatever BLE trick the challenge hides behind.
- Submit it by writing the flag string to characteristic **0xFF02**.
- Check your score by reading (or subscribing to) **0xFF01**.

That is the whole loop. Everything else is learning how BLE actually works, one
flag at a time. Let's go.
