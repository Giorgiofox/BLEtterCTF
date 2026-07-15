# FAQ

## The board is flashed but I do not see `BLEtterCTF` when scanning
- Check the serial monitor shows `BLEtterCTF up: N flags implemented`. If not, the
  firmware is not running; reflash (see [Flashing the ESP32](Flashing-the-ESP32)).
- Toggle Bluetooth off/on on your client. BLE scanners get stale.
- Move closer. 2.4 GHz is easily blocked by walls, hands, and bad luck.

## I connect but see no `0xFF00` service / no characteristics
- Some clients cache the GATT database. Disconnect, forget/remove the device, and
  reconnect to force a fresh discovery.
- Make sure you are looking at the `0xFF00` **service** and expanding it.

## A read gives me gibberish, not a flag
- It may be encoded. Hex looks like `666c61...`; base64 ends in `=`. See the
  [Tier 0 walkthrough](Walkthrough-Tier-0-GATT-Basics).
- It may be a raw-bytes value your tool is showing as hex. Switch the display
  format to text, or decode it yourself.

## My flag submission does not raise the score
- You must write to **0xFF02** (submit), not to the characteristic you read from.
- Submit the flag **exactly** as found, no extra spaces or newlines. In
  bluetoothctl, watch for a trailing `0x00`.
- Read **0xFF01** after submitting to confirm.

## A value looks cut off / too short
- You probably read only the first ~20 bytes (one MTU). Do a full read (Read
  Blob). See flag 36 in [Advanced Tiers](Advanced-Tiers).

## Pairing keeps failing (Tier 3)
- Remove the old bond and retry: `remove AA:BB:CC:DD:EE:FF` in bluetoothctl.
- Wipe the board's stored bonds with `esptool.py -p PORT erase_flash` (this also
  resets your score).
- nRF Connect on a phone is the most reliable pairing client; try it if a laptop
  struggles.

## `bleak` will not pair
- Known limitation; pairing support is uneven across platforms. Use nRF Connect or
  bluetoothctl for the Tier 3 flags. `bleak` is great for T0-T2.

## Do I really not need BLEtterCap?
- Correct. The entire core game (Tiers 0-3) is solvable with a phone or a laptop
  and free tools. BLEtterCap adds a packet X-ray that makes learning faster, but
  it is never required.

## Which tiers need extra hardware?
- T0-T3: nothing beyond a phone/laptop.
- T4-T6: a ButteRFly sniffer (observe-only flags) and/or an nRF52840 dongle
  (radio-control flags). See [Hardware and Kit](Hardware-and-Kit).

## The score resets when I reconnect
- The scaffold stores captures in RAM. A reboot or reflash clears them. Persisting
  to NVS is a planned improvement; for now, finish a tier in one session.

## Can I add my own flags?
- Yes, and please do. See [Adding Flags](Adding-Flags).

## Is any of this legal / safe?
- Yes. You are attacking a device you own, that was built to be attacked, in your
  own space. That is textbook ethical practice. The skills transfer to real
  security work; the targets you point them at afterwards are your responsibility.
