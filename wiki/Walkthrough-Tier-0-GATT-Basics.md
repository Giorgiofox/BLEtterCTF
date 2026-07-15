# Walkthrough - Tier 0: GATT Basics

Tier 0 teaches the GATT alphabet: connect, enumerate, read, decode, write, and
follow a little state machine. No special hardware, no pairing, no drama. If BLE
were a video game, this is the tutorial level where the enemies barely move.

These pages teach the **method**, not the literal answer. You still get to press
the button yourself. (That is the whole point. Also it feels great.)

Every flag ends with: write the string you found to **0xFF02**, then read
**0xFF01** to confirm your score went up.

Setup reminder: pick a toolkit from
[Solving Without BLEtterCap](Solving-Without-BLEtterCap). Examples below use
`bleak`-style pseudocode; the same steps work in nRF Connect and bluetoothctl.

---

## Flag 1 - the gift (read 0xFF03)

The concept: a **characteristic read**. Connect, read `0xFF03`, get a flag. That
is it. BLE really is just "read the value at this address" a lot of the time.

```python
val = await c.read_gatt_char(u(0xFF03))
await c.write_gatt_char(u(0xFF02), val, response=True)
```

If you submitted the gift flag and your score is now 1, the pipeline works.
Everything else is variations on this.

---

## Flag 2 - hello, world (read 0xFF07)

Same mechanic, different characteristic. The lesson: **enumerate**. Real devices
have many characteristics; you list them all and check each. In nRF Connect you
see them in the service tree; in bleak, iterate `c.services`; in bluetoothctl,
`list-attributes`.

---

## Flag 3 - read the fine print (descriptor of 0xFF04)

The concept: **descriptors**. A characteristic's value is not the only place data
hides. The **User Description (0x2901)** descriptor is a human label, and this one
holds a flag. Read the *descriptor*, not the characteristic value.

- nRF Connect: expand `0xFF04`, tap the descriptor.
- bleak: find the char, read `descriptor.handle` via `read_gatt_descriptor`.
- bluetoothctl: `list-attributes` shows the descriptor path; select and `read`.

Moral: when a value looks empty or boring, check its descriptors.

---

## Flag 5 - hex marks the spot (read 0xFF08)

The value looks like `666c61677b...`. That is **hex** (base16), the most common
way bytes get shown as text. Decode it:

```python
raw = await c.read_gatt_char(u(0xFF08))
flag = bytes.fromhex(raw.decode())   # if it arrives as an ASCII hex string
```

Encoding is not encryption. It is just a costume. Take the costume off.

---

## Flag 6 - sixty-four shades (read 0xFF09)

This value ends in `=` or `==` and uses A-Z a-z 0-9 + /. Classic **base64**.

```python
import base64
flag = base64.b64decode(await c.read_gatt_char(u(0xFF09)))
```

Still not hacking. Still counts. We do not judge here.

---

## Flag 7 - collect all three (0xFF0A, 0xFF0B, 0xFF0C)

The flag is split into three characteristics, one third each. Read all three and
concatenate them **in order**. The lesson: sometimes one value is not the whole
story, and order matters.

```python
a = await c.read_gatt_char(u(0xFF0A))
b = await c.read_gatt_char(u(0xFF0B))
d = await c.read_gatt_char(u(0xFF0C))
flag = a + b + d
```

---

## Flag 8 - knock knock (write then read 0xFF05)

The concept: **write to change state, then read the result**. Read `0xFF05` first
and it tells you what to do. Write the key `d34dbeef` to it, then read again and
the flag appears.

```python
await c.write_gatt_char(u(0xFF05), b"d34dbeef", response=True)
flag = await c.read_gatt_char(u(0xFF05))
```

This is how a lot of real devices work: write a command, read the response.

---

## Flag 9 - the magic words, in order (0xFF0D)

A tiny **state machine**. You must write `1`, then `2`, then `3` to `0xFF0D`, in
that exact order. Any wrong step resets it to the start, so no frudging with `3`
first. Then read `0xFF0D` for the flag.

```python
for step in (b"1", b"2", b"3"):
    await c.write_gatt_char(u(0xFF0D), step, response=True)
flag = await c.read_gatt_char(u(0xFF0D))
```

The lesson: devices often enforce a sequence (unlock, then configure, then act).
Order is a security boundary, and a common place bugs hide.

---

## Tier 0 done

You can now connect, enumerate, read, decode, write, and drive a sequence. That
is the daily bread of BLE work. Onward to devices that call *you*:
**[Tier 1 - Notifications and Indications](Walkthrough-Tier-1-Notifications-and-Indications)**.
