# nRF52840 probe (optional)

The advanced tiers (coded PHY, periodic advertising, 2M PHY, directed/identity
advertising, some L2CAP) need a client radio that commodity laptops and phones do
not expose. A ~10 EUR **nRF52840 USB dongle** flashed with the BLEtterCTF probe
firmware fills that gap. It doubles as a general BLEtterCap probe.

> This directory is a placeholder. The probe firmware is planned on Zephyr /
> nRF Connect SDK; contents will land in `firmware/nrf-probe/`.

## What the probe provides

| Capability | Used by |
|------------|---------|
| Scan + sync to periodic advertising | T2 #20 |
| Receive on LE Coded PHY (long range) | T5 #31 |
| Force 2M PHY on a connection | T5 #30 |
| Set the device identity / address | T4 #28, #29 |
| L2CAP CoC client | T6 #33 |

## Hardware

- nRF52840 USB dongle (e.g. the common "Nordic dongle" form factor), or any
  nRF52840 devkit.

## Planned toolchain

- Zephyr / nRF Connect SDK.
- Flash over USB DFU (`nrfutil`) or a debug probe (`west flash`).
- A small serial/host protocol so BLEtterCap can drive the probe (scan on coded
  PHY, set address, open an L2CAP channel, etc.).

## Alternative

For observe-only advanced flags (T2 ext/periodic *observe*, T4 RPA, T5
conn-param, T3 #26 LESC-vs-legacy) the **ButteRFly sniffer** you already have is
enough; the probe is only needed for the flags that require actively *controlling*
the radio.
