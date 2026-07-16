# Walkthrough - Tier 3: Security

> Beginner-proof, same format. This tier is where the device starts saying "prove
> you are allowed".

## The big idea: some data is protected

Until now the device answered every question. Real devices should not do that -
your smart lock must not open its "unlock" characteristic to any stranger in
range. So BLE lets a characteristic **require an encrypted, paired connection**
before it will hand over its value. This tier is about establishing that trust.

Three words you need:

- **Pairing** - the one-time process where two devices agree on secret keys.
- **Encryption** - once keys exist, the link is scrambled so an eavesdropper
  (a sniffer) sees gibberish instead of your data.
- **Bonding** - *remembering* the keys, so the next connection skips pairing and
  goes straight to encrypted.

## How BLE pairing works (from zero)

When a client tries to read a characteristic marked "encryption required", the
server refuses with an error like **Insufficient Authentication/Encryption**.
That refusal is a signal: your operating system responds by starting **pairing**.

Pairing has several **association models**, chosen automatically from what each
side can do (does it have a screen? a keypad?):

- **Just Works** - no user interaction. Encrypts the link but gives **no
  protection against a man-in-the-middle**. Fine for a lightbulb.
- **Passkey Entry** - one side shows a 6-digit code, the other types it. Blocks
  man-in-the-middle attacks.
- **Numeric Comparison** - both sides show a number, the user confirms they match.
- **Out Of Band (OOB)** - keys shared over another channel (NFC, a printed code).

Also worth knowing: **Legacy pairing** (Bluetooth 4.0-4.1) uses a weak key
exchange a sniffer can often crack; **LE Secure Connections (LESC)** (4.2+) uses
elliptic-curve crypto that a sniffer cannot crack even if it captures everything.
Always prefer LESC.

---

## Flag 21 - members only: encryption required

**The idea.** Characteristic `0xFF13` is marked "read requires encryption". Read
it cold and you are refused. Pair first, then read.

**How BLE does it.** BLEtterCTF uses **Just Works** pairing (no PIN, no
comparison). When you attempt the read, the server demands encryption, your OS
pairs Just Works, the link becomes encrypted, and the read then succeeds.

**Do it - and the one Linux gotcha that will bite you.** On Linux/BlueZ, pairing
fails with `org.bluez.Error.AuthenticationFailed` **unless a pairing agent is
registered**. Even Just Works needs *something* to authorize it. The two easy
paths:

- **nRF Connect (phone) - simplest:** connect, tap read on `0xFF13`, accept the
  pairing pop-up, read the flag, write it to `0xFF02`. The phone *is* the agent.
- **`bluetoothctl`:** in one live session, register an agent, then pair:
  ```
  agent NoInputNoOutput
  default-agent
  pair 14:C1:9F:E5:A6:5A
  ```
  then connect and read `0xFF13`.

The reference solver (`tools/solver-examples/solve_bleak.py`) registers a
`NoInputNoOutput` D-Bus agent automatically, so on Linux the read just works.

**What you learned.** Encryption is a feature you *request by requiring it*: a
characteristic that is not marked encrypted is readable by anyone. And on Linux,
"pairing failed" almost always means "no agent registered", not "bad hardware" -
a debugging lesson that costs people hours.

---

## Flag 24 - come back later: bonding persistence

**The idea.** Pairing every single time you connect would be miserable. **Bonding**
stores the keys after the first pairing, so future connections encrypt instantly,
no user interaction. Your phone does this with your headphones - you pair once,
then it "just connects" forever.

**How BLE does it.** `0xFF17` also requires encryption. But because the previous
pairing (flag 21) **bonded** - both sides saved the keys - a later read over the
already-encrypted link succeeds *without pairing again*. If you disconnect and
reconnect, the bond is reused: the link re-encrypts silently and the read works.

**Do it.** After flag 21 has paired/bonded, `0xFF17` reads over the same encrypted
link:

```python
flag = await client.read_gatt_char(u(0xFF17))   # works: link is encrypted/bonded
await submit(client, flag)
```

To *see* persistence directly: disconnect, reconnect, and read `0xFF17` again
**without** calling pair() - it still works, because the bond survived the
disconnect.

**What you learned.** Pair once, reconnect forever. Bonding is why real BLE
devices feel seamless after setup - and why clearing a bond (`remove` in
`bluetoothctl`, or erasing the device's storage) is the fix when pairing gets
"stuck".

---

## The rest of Tier 3 (concepts + where they go next)

These are designed and coming; they each need a specific pairing setup:

- **Flag 22 - prove it is you:** **Passkey Entry**. The device shows a 6-digit
  code; you type it into your client. Adds man-in-the-middle protection.
- **Flag 23 - confirm the number:** **LESC Numeric Comparison**. Both sides show a
  number; you confirm they match. No typing, still MITM-protected.
- **Flag 25 - out of band:** the pairing key arrives over a side channel.
- **Flag 26 - why LESC matters:** with the **ButteRFly sniffer**, capture a
  *legacy*-paired exchange (crackable, so effectively plaintext to you) next to a
  *LESC* one (uncrackable). Seeing the difference is the single most convincing
  argument for LESC you will ever get.

See [Advanced Tiers](Advanced-Tiers) for the sniffer-based ones.

---

## If pairing gets stuck

Pairing is the flakiest part of BLE across operating systems. If it wedges:

```
# bluetoothctl
remove 14:C1:9F:E5:A6:5A        # forget the bond on the host, then retry
```

And on the device: erasing its flash (`esptool.py -p PORT erase-flash`) wipes its
stored bonds (and resets the score). Fresh start, no grudges.

---

## Tier 3 done

You can pair, encrypt, bond, and read protected data - and you understand why some
characteristics refuse to talk, and why "pairing failed" on Linux usually means
"no agent". Beyond here lies privacy, PHYs, and L2CAP, which need a sniffer or an
nRF dongle: **[Advanced Tiers](Advanced-Tiers)**.
