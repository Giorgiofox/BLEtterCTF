# nRF52840 probe firmware (placeholder)

Optional client firmware for an nRF52840 USB dongle, used to solve the advanced
tiers that a commodity laptop/phone radio cannot (coded PHY, periodic advertising
sync, 2M PHY control, settable address, L2CAP CoC).

Not implemented yet. Design and rationale in
[../../docs/06-nrf-probe.md](../../docs/06-nrf-probe.md).

Planned: Zephyr / nRF Connect SDK, USB DFU flashing, a small serial protocol so
BLEtterCap can drive the probe.

For observe-only advanced flags, use the ButteRFly sniffer instead; the probe is
only needed for flags that require actively controlling the radio.
