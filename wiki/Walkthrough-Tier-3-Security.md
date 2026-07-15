# Walkthrough - Tier 3: Security

Until now the device answered every question you asked. Tier 3 is where it starts
saying "prove you are allowed." This is **pairing**, **bonding**, and
**encryption**, the machinery that keeps your smart lock from opening for
strangers (in theory).

Recap from the [primer](BLE-Primer): a characteristic can **require encryption**.
Read it without pairing and the server returns an authentication/encryption error,
which prompts your OS to start **pairing**. Pairing agrees on keys; **bonding**
stores them so future connections skip straight to encrypted.

---

## Flag 21 - members only (encrypted read on 0xFF13)

`0xFF13` is marked read-encrypted. Try to read it cold and you get an "Insufficient
Authentication" (or Encryption) error. Pair first, then read.

This device uses **Just Works** pairing (no PIN, no numeric compare), so pairing
is a single step:

- **nRF Connect (easiest):** connect, tap the read icon on `0xFF13`. The phone
  pops a pairing dialog; accept it. The read then succeeds. (You can also use the
  overflow menu -> **Bond**.)
- **bluetoothctl:**
  ```
  pairable on
  pair AA:BB:CC:DD:EE:FF
  connect AA:BB:CC:DD:EE:FF
  menu gatt
  select-attribute <0xFF13 path>
  read
  ```
- **bleak:** pairing support is uneven across platforms; prefer nRF Connect or
  bluetoothctl for this one. On some backends `await c.pair()` works.

The lesson: encryption is a *feature you request by requiring it*. A characteristic
that is not marked encrypted is readable by anyone in range, paired or not. Design
accordingly.

---

## The rest of Tier 3 (concepts + where they go)

The scaffold implements the Just Works flag; these are the rest of the tier,
finalized as the firmware grows:

- **Flag 22 - prove it is you:** **Passkey Entry**. The device displays a 6-digit
  number (on its serial log) and you type it into your client. Adds MITM
  protection. Requires switching the device IO capability to DISPLAY_ONLY.
- **Flag 23 - confirm the number:** **LESC Numeric Comparison**. Both sides show a
  number; you confirm they match. No typing, still MITM-protected.
- **Flag 24 - come back later:** **bonding persistence**. Pair once, disconnect,
  reconnect, and read an encrypted characteristic **without pairing again**. The
  lesson: bonding stores keys so the second connection is instant and encrypted.
- **Flag 26 - why LESC matters:** with the **ButteRFly sniffer**, capture a
  **legacy**-paired exchange (the key is derivable, so the traffic is effectively
  plaintext to you) next to a **LESC** exchange (ECDH, uncrackable from the
  capture). Seeing the difference is the flag. This is the single most convincing
  argument for LESC you will ever get.

See [Advanced Tiers](Advanced-Tiers) for the sniffer-based ones.

---

## A note on pairing pain

Pairing is the flakiest part of BLE across operating systems. If a pairing gets
stuck, the fix is almost always: **remove the existing bond** and retry.

```sh
# bluetoothctl
remove AA:BB:CC:DD:EE:FF
```

On the device side, `esptool.py -p PORT erase_flash` wipes stored bonds (and your
score). Fresh start, no grudges.

---

## Tier 3 done

You can now pair, bond, and read protected data, and you understand why some
characteristics refuse to talk. Beyond here lies privacy, PHY tricks, and L2CAP,
which need a sniffer or an nRF dongle: **[Advanced Tiers](Advanced-Tiers)**.
