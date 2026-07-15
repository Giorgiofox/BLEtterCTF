# Flashing the ESP32

This guide takes you from a blank ESP32 to a board advertising as `BLEtterCTF`.

There are two paths:

- **A. Build from source with ESP-IDF** (recommended, works for any chip).
- **B. Flash a prebuilt binary with esptool** (no toolchain needed).

> Safety note: identify the correct serial port before you flash. A BLE sniffer
> dongle (e.g. ButteRFly) and other USB devices show up as serial ports too.
> Flashing writes to whatever port you name, so confirm it is your ESP32 devkit
> first (see "Identify the serial port" below). BLEtterCap itself must never open
> the CTF target's serial port automatically; flashing is a manual step you run
> yourself.

## Prerequisites

- An ESP32 devkit and a USB data cable (not charge-only).
- Linux/macOS/Windows. On Linux your user must be in the `dialout` group to
  access the serial port:

  ```sh
  sudo usermod -aG dialout "$USER"   # then log out and back in
  ```

## Identify the serial port

Plug in the ESP32 (only the ESP32, nothing else new) and check what appeared:

```sh
# Linux
ls -l /dev/serial/by-id/          # most reliable: shows device names
ls /dev/ttyUSB* /dev/ttyACM*      # CP210x/CH34x -> ttyUSB*, native USB -> ttyACM*

# macOS
ls /dev/cu.usbserial-* /dev/cu.usbmodem*
```

Note the exact path (e.g. `/dev/ttyUSB0`, `/dev/cu.usbserial-0001`). Everywhere
below, replace `PORT` with it. If several ports exist, unplug the ESP32, list
again, plug it back in, and take the one that appeared.

---

## Path A: build from source (ESP-IDF)

### 1. Install ESP-IDF (v5.x)

```sh
mkdir -p ~/esp && cd ~/esp
git clone -b v5.2.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh
# start of every session:
. ~/esp/esp-idf/export.sh
```

### 2. Build

```sh
cd BLEtterCTF/firmware/esp32

idf.py set-target esp32c6     # or esp32 / esp32c3 / esp32s3 to match your board
idf.py build
```

`set-target` regenerates `sdkconfig` from `sdkconfig.defaults`. On the classic
`esp32` the BLE5 features stay off automatically; the core tiers still build.

### 3. Flash and watch the logs

```sh
idf.py -p PORT flash monitor
```

Expected serial output:

```
I (xxx) blectf: BLEtterCTF up: 5 flags implemented
```

Leave `monitor` running to watch flag captures. Exit with `Ctrl-]`.

If flashing fails to start, hold the board's **BOOT** button while it connects
(some boards need it), or lower the speed: `idf.py -p PORT -b 115200 flash`.

---

## Path B: prebuilt binary (esptool only)

Use this when you just want to run the CTF without installing the toolchain.

### 1. Install esptool

```sh
pip install esptool
```

### 2. Flash the release images

Download the release bundle for your chip (bootloader, partition table, app)
from the repo Releases, then:

```sh
esptool.py -p PORT -b 460800 --chip esp32c6 write_flash \
    0x0     bootloader.bin \
    0x8000  partition-table.bin \
    0x10000 bletterctf.bin
```

Offsets: `bootloader.bin` at `0x0` for esp32c3/c6/s3, or `0x1000` for the classic
`esp32`. Match `--chip` to your board.

### 3. Verify

```sh
esptool.py -p PORT read_mac        # confirms the board responds
```

Then scan for a device named `BLEtterCTF` from your phone or laptop.

---

## Erase / reset

To wipe the board (clears bonds and NVS score) before re-flashing:

```sh
esptool.py -p PORT erase_flash
```

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Permission denied` on PORT | add user to `dialout`, re-login |
| `Failed to connect` | hold BOOT during connect; try another cable; lower `-b` |
| Wrong/garbled monitor output | baud mismatch, use `idf.py monitor` (auto 115200) |
| No `BLEtterCTF` in scans | check monitor shows "BLEtterCTF up"; toggle BLE on the client |
| Two serial ports appear | you flashed the wrong device; re-run "Identify the serial port" |
