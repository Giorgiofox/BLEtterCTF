# Getting Started

You need three things: a board, a way to flash it, and a way to talk BLE. None of
it is expensive, and the core game needs no special hardware at all.

## 1. What to buy (or dig out of a drawer)

**Required: one ESP32 board.** Any of these works:
- ESP32 (classic) - covers the core tiers (BLE 4.2).
- ESP32-C3 / S3 - adds BLE 5.0 features.
- ESP32-C6 - the full set (BLE 5.3).

A USB devkit (DevKitC, XIAO, etc.) plus a data-capable USB cable is ideal. See
[Hardware and Kit](Hardware-and-Kit) for details and the optional add-ons.

**Required: a BLE client you already own.** Any one of:
- A **laptop** with Bluetooth (Linux `bluetoothctl`, or Python `bleak` on any OS).
- A **phone** with the free **nRF Connect** app (iOS/Android).

**Optional (advanced tiers only):**
- A **ButteRFly** BLE sniffer for observe-only flags.
- An **nRF52840 USB dongle** (~10 EUR) for flags that need to control the radio.

## 2. Flash the board

Follow **[Flashing the ESP32](Flashing-the-ESP32)**. Ten minutes from blank board
to a device advertising as `BLEtterCTF`.

## 3. Confirm it is alive

Scan for BLE devices from your phone or laptop. You should see a device named
**`BLEtterCTF`**. If you do, congratulations, the hard part is over.

If you do not, check the serial monitor shows `BLEtterCTF up: N flags implemented`
and see the [FAQ](FAQ).

## 4. Learn the game loop

Every flag ends the same way:

1. Extract the flag string using whatever BLE trick the challenge hides.
2. **Submit** it: write the string to characteristic **0xFF02**.
3. **Check your score**: read (or subscribe to) **0xFF01**. It reports
   `flags captured N/M`.

## 5. Pick your toolkit and start

Read **[Solving Without BLEtterCap](Solving-Without-BLEtterCap)** to set up your
tool of choice, then dive into **[Tier 0 - GATT Basics](Walkthrough-Tier-0-GATT-Basics)**.

That is it. The board is waiting, and it is a little smug about flag 1. Go prove
it wrong.
