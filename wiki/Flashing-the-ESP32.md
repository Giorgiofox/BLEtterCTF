# Flashing the ESP32

Goal: turn a blank ESP32 into a board advertising as `BLEtterCTF`. Two paths:

- **Path A** - build from source with ESP-IDF (works for any chip, recommended).
- **Path B** - flash a prebuilt binary with `esptool` (no toolchain).

> **Safety first.** Flashing writes to whatever serial port you name. A BLE
> sniffer dongle and other USB gadgets also show up as serial ports. Confirm you
> picked your ESP32 devkit before you flash (see below). If you run BLEtterCap,
> note it must never auto-open the CTF target's serial port; flashing is a manual
> step you run yourself.

## Prerequisites

- An ESP32 devkit and a **data** USB cable (charge-only cables are a classic
  time sink).
- On Linux, permission to use the serial port:
  ```sh
  sudo usermod -aG dialout "$USER"   # then log out and back in
  ```

## Identify the serial port

Plug in **only** the ESP32, then:

```sh
# Linux
ls -l /dev/serial/by-id/          # most reliable, shows device names
ls /dev/ttyUSB* /dev/ttyACM*      # CP210x/CH34x -> ttyUSB*, native USB -> ttyACM*

# macOS
ls /dev/cu.usbserial-* /dev/cu.usbmodem*
```

Note the exact path (e.g. `/dev/ttyUSB0`). Replace `PORT` with it below. If
several appear, unplug the ESP32, list again, plug it back in, and take the new
one.

---

## Path A: build from source (ESP-IDF)

### 1. Install ESP-IDF v5.x

```sh
mkdir -p ~/esp && cd ~/esp
git clone -b v5.2.1 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf && ./install.sh
# run this at the start of every terminal session:
. ~/esp/esp-idf/export.sh
```

### 2. Build

```sh
cd BLEtterCTF/firmware/esp32
idf.py set-target esp32c6      # or esp32 / esp32c3 / esp32s3 to match your board
idf.py build
```

`set-target` regenerates the config from `sdkconfig.defaults`. On the classic
`esp32` the BLE 5 features stay off automatically; the core tiers still build.

### 3. Flash and watch

```sh
idf.py -p PORT flash monitor
```

Expected:
```
I (xxx) blectf: BLEtterCTF up: 15 flags implemented
```

Leave `monitor` running to watch captures. Exit with `Ctrl-]`. If flashing will
not start, hold the board's **BOOT** button while it connects, or slow down with
`idf.py -p PORT -b 115200 flash`.

---

## Path B: prebuilt binary (esptool only)

For when you want to play, not install a toolchain.

```sh
pip install esptool
```

Download the release bundle for your chip (bootloader, partition table, app) from
the repo Releases, then:

```sh
esptool.py -p PORT -b 460800 --chip esp32c6 write_flash \
    0x0     bootloader.bin \
    0x8000  partition-table.bin \
    0x10000 bletterctf.bin
```

Bootloader offset is `0x0` for esp32c3/c6/s3, or `0x1000` for the classic
`esp32`. Match `--chip` to your board. Verify with:

```sh
esptool.py -p PORT read_mac
```

Then scan for `BLEtterCTF`.

---

## Erase (reset the score and bonds)

```sh
esptool.py -p PORT erase_flash
```

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| `Permission denied` on PORT | add user to `dialout`, re-login |
| `Failed to connect` | hold BOOT during connect; try another cable; lower `-b` |
| Garbled monitor output | baud mismatch; use `idf.py monitor` (auto 115200) |
| No `BLEtterCTF` in scans | check the monitor shows "BLEtterCTF up"; toggle BLE on the client |
| Two serial ports appear | you may have flashed the wrong device; re-identify the port |

Next: **[Solving Without BLEtterCap](Solving-Without-BLEtterCap)**.
