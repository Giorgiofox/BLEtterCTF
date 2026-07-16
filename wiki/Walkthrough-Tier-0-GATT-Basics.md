# Walkthrough - Tier 0: GATT Basics

> New to BLE? This page assumes you know **nothing**. Every term is explained the
> first time it appears. Read it top to bottom; by the end you will understand
> how a BLE device stores and shares data, and you will have captured 9 flags.

## First, the big picture: who is who

Two devices are involved, and each plays two roles at once. Do not let the
vocabulary scare you:

- **Your laptop/phone** is the **Central** (it starts the connection) and the
  **Client** (it asks for data).
- **The ESP32 board** is the **Peripheral** (it waits to be connected to) and the
  **Server** (it holds the data).

So: you connect *to* the board, then you *ask it for* data. That is the entire
relationship. (More on roles in the [BLE Primer](BLE-Primer).)

## What is the board actually holding?

A BLE server is not a filesystem or a website. It is a flat little **table of
values**, organized in a strict hierarchy called **GATT** (Generic Attribute
Profile). Picture it like this:

```
Service  0xFF00                      <- a folder that groups related values
  |
  +-- Characteristic 0xFF03          <- one value...
  |     value:  "flag{...}"          <-   ...its actual bytes
  |     handle: 0x001a               <-   ...its address on the device
  |     props:  READ                 <-   ...what you're allowed to do to it
  |     descriptors: [ ... ]         <-   ...optional sticky-notes about it
  |
  +-- Characteristic 0xFF07 ...
```

Four words you now own:

- **Service** - a folder grouping related characteristics. BLEtterCTF puts
  everything in one service, UUID `0xFF00`.
- **Characteristic** - a single piece of data. It has a **value** (the bytes),
  a **handle**, a **UUID**, **properties**, and optional descriptors.
- **UUID** - the *type* / id of the thing. Short standard ones are 16-bit
  (`0xFF03`); custom ones are 128-bit. BLEtterCTF uses the short `0xFFxx` range.
- **Handle** - the *address* of the thing on this particular device (like a house
  number). Your tools mostly hide handles and let you use UUIDs; both appear here.

**Properties** are the verbs a characteristic allows: **Read** (you pull the
value), **Write** (you set it), **Notify/Indicate** (it pushes to you - Tier 1).
A characteristic without Read simply cannot be read; the property list is the
contract.

## The game loop (memorize this)

1. **Scan** for a device advertising the name `BLEtterCTF`, and **connect**.
2. **Enumerate** its services and characteristics (your tool lists them).
3. **Extract** a flag using whatever trick the challenge hides.
4. **Submit** it: **write the flag string to characteristic `0xFF02`**.
5. **Check the score**: read (or subscribe to) `0xFF01` -> `flags captured N/20`.

Pick a tool from [Solving Without BLEtterCap](Solving-Without-BLEtterCap). The
snippets below are `bleak` (Python); the same three steps work identically in the
nRF Connect phone app and in `bluetoothctl`.

A tiny helper used throughout (turns a 16-bit id into the full UUID string):

```python
def u(x): return f"0000{x:04x}-0000-1000-8000-00805f9b34fb"
SUBMIT, SCORE = u(0xFF02), u(0xFF01)

async def submit(client, flag: bytes):
    await client.write_gatt_char(SUBMIT, flag, response=True)
    print((await client.read_gatt_char(SCORE)).decode())
```

---

## Flag 1 - the gift: your first read

**The idea.** The most common thing you ever do in BLE is *read a value*. A
thermometer's temperature, a lock's battery level - all just "read this
characteristic". So the first flag is simply that, to prove your pipeline works.

**How BLE does it.** A **Read Request** is an ATT (Attribute Protocol) message:
your client sends "give me the value at handle X", the server replies with the
bytes. Your library does this when you call `read_gatt_char(uuid)` - it looks up
the handle for that UUID and sends the request.

**Do it.** Connect, read `0xFF03`, submit what you get.

```python
val = await client.read_gatt_char(u(0xFF03))
print(val)                 # b'flag{welcome_you_absolute_legend}'
await submit(client, val)  # score becomes 1/20
```

**What you learned.** Reading a characteristic = one request, one reply. If your
score went to 1, connect/read/submit all work. Everything else is variations.

---

## Flag 2 - hello, world: there is more than one

**The idea.** Real devices have *many* characteristics. You never assume there is
just one - you **enumerate** them all and look at each.

**How BLE does it.** Right after connecting, the client does **service
discovery**: it asks the server for its full list of services, characteristics,
handles, and properties. Your tool caches this list (`client.services`).

**Do it.** This flag lives in a *different* characteristic, `0xFF07`. Reading it
is identical to flag 1 - the lesson is only "check every characteristic, not just
the obvious one".

```python
val = await client.read_gatt_char(u(0xFF07))
await submit(client, val)
```

In nRF Connect you literally see the whole list in the service tree; in
`bluetoothctl` it is `list-attributes`.

**What you learned.** Always enumerate. The interesting data is rarely in the
first characteristic you look at.

---

## Flag 3 - read the fine print: descriptors

**The idea.** A characteristic's *value* is not the only place data hides.
Characteristics can carry **descriptors** - little metadata attachments. The most
common is the **User Description** (`0x2901`), a human-readable label like
"Temperature in Celsius". People sometimes stash real data there.

**How BLE does it.** A descriptor is just another attribute (handle + UUID +
value) that belongs *under* a characteristic. `0x2901` is the standard UUID for
"Characteristic User Description". You read it exactly like a value, but you have
to look one level deeper than the characteristic itself.

**Do it.** Read the `0x2901` descriptor of characteristic `0xFF04` (not its
value - its descriptor).

```python
async def read_user_description(client, char_uuid):
    for service in client.services:
        for ch in service.characteristics:
            if ch.uuid.lower() == char_uuid.lower():
                for d in ch.descriptors:
                    if d.uuid.startswith("00002901"):
                        return await client.read_gatt_descriptor(d.handle)

val = await read_user_description(client, u(0xFF04))
await submit(client, val)
```

- nRF Connect: expand `0xFF04`, tap the descriptor row.
- `bluetoothctl`: `list-attributes` shows the descriptor path; select and `read`.

**What you learned.** When a value looks empty or boring, check its descriptors.
Metadata is data.

---

## Flag 4 - the name is longer than it looks

**The idea.** Every BLE device has a **name**, and it appears in two very
different places: the tiny **advertisement** everyone can see while scanning, and
a **characteristic** you read after connecting. Those two can *disagree*, and the
advertised one is often deliberately abbreviated.

**How BLE does it.** The advertisement (the packet a device broadcasts before any
connection) can carry either a **Complete Local Name** or a **Shortened Local
Name** - because the advertisement is only 31 bytes and long names do not fit.
The device's *full* name lives elsewhere. In standard BLE that is the GAP
"Device Name" characteristic (`0x2A00`); BLEtterCTF serves the full name from
`0xFF1A` instead, because Linux/BlueZ hides the standard GAP service from apps
(a real-world quirk worth knowing - see the note below).

**Do it.** The scanner shows the device as `BLEtterCTF` (the Shortened name).
Read `0xFF1A` for the full name; the flag is inside it.

```python
full = (await client.read_gatt_char(u(0xFF1A))).decode()
print(full)                          # "BLEtterCTF flag{names_can_be_longer}"
import re
await submit(client, re.search(r"flag\{[^}]*\}", full).group(0).encode())
```

> **Real-world note.** On Linux, BlueZ (which `bleak` and `bluetoothctl` use) does
> *not* expose the standard GAP service (`0x1800`/`0x2A00`) to applications - it
> reads the name itself. That is why BLEtterCTF puts the full name in a normal
> characteristic. On the nRF Connect phone app you *can* browse the GAP service
> directly. Different stacks reveal different things - a recurring theme.

**What you learned.** The advertised name is a preview, not the whole truth, and
what a tool lets you see depends on the tool.

---

## Flag 5 - hex marks the spot: encoding, not encryption

**The idea.** Bytes are often shown as text so humans can read them. The most
common way is **hex** (base16): each byte becomes two characters `0-9a-f`. It is
a costume, not a lock - anyone can take it off.

**How BLE does it.** BLE does nothing special here; the value is just bytes. The
*firmware* chose to store the flag hex-encoded. Reading gives you something like
`666c61677b...`; you decode it back to text.

**Do it.**

```python
raw = (await client.read_gatt_char(u(0xFF08))).decode()   # "666c61677b..."
flag = bytes.fromhex(raw)                                   # b"flag{...}"
await submit(client, flag)
```

**What you learned.** Encoding != encryption. If it decodes without a key, it was
never secret. (Real encryption comes in Tier 3.)

---

## Flag 6 - sixty-four shades: base64

**The idea.** Another costume: **base64**, which packs bytes into the characters
`A-Z a-z 0-9 + /` and pads with `=`. You will recognize it by that trailing `=`.

**Do it.**

```python
import base64
raw = (await client.read_gatt_char(u(0xFF09))).decode()
await submit(client, base64.b64decode(raw))
```

**What you learned.** Learn to *recognize* encodings on sight: hex is `0-9a-f`
only; base64 ends in `=` and mixes upper/lower case. Recognizing them is half the
job.

---

## Flag 7 - collect all three: reassembly

**The idea.** Sometimes one logical value is split across several
characteristics (or several packets). You must gather the pieces and put them
back **in order**.

**How BLE does it.** Nothing forces this; the firmware simply split the flag into
thirds across `0xFF0A`, `0xFF0B`, `0xFF0C`. This mirrors real life, where a big
value gets chunked because each BLE packet is small (you will meet the packet
size limit properly in flag 12 and flag 36).

**Do it.**

```python
a = await client.read_gatt_char(u(0xFF0A))
b = await client.read_gatt_char(u(0xFF0B))
c = await client.read_gatt_char(u(0xFF0C))
await submit(client, a + b + c)      # order matters
```

**What you learned.** Data can be scattered; order matters when you rejoin it.

---

## Flag 8 - knock knock: writing changes state

**The idea.** Reading is passive. **Writing** lets you *change* the device -
send a command, set a config, unlock something. Most real interactions are
"write a command, then read the response".

**How BLE does it.** A **Write Request** carries your bytes to a characteristic's
handle; the server processes them and (for a Write *Request*) acknowledges. Here,
writing the right value flips an internal switch so a later read returns the flag.

**Do it.** Read `0xFF05` first and it tells you what to do. Write the key
`d34dbeef`, then read again.

```python
await client.write_gatt_char(u(0xFF05), b"d34dbeef", response=True)
flag = await client.read_gatt_char(u(0xFF05))    # now returns the flag
await submit(client, flag)
```

> `response=True` uses a **Write Request** (the server ACKs). There is also
> Write *Without Response* (fire-and-forget) - faster, no confirmation. The
> property list says which a characteristic supports.

**What you learned.** Writing drives state. "Write a command, read the result" is
the backbone of talking to real devices.

---

## Flag 9 - the magic words, in order: a state machine

**The idea.** Devices often enforce a **sequence**: unlock, then configure, then
act. Do the steps out of order and you get nothing. Order is a security boundary,
and a very common place bugs (and flags) hide.

**How BLE does it.** Still just writes - but the firmware tracks *how far along*
you are. Write `1`, then `2`, then `3` to `0xFF0D`. Any wrong step resets the
counter to the start, so you cannot skip ahead.

**Do it.**

```python
for step in (b"1", b"2", b"3"):
    await client.write_gatt_char(u(0xFF0D), step, response=True)
flag = await client.read_gatt_char(u(0xFF0D))
await submit(client, flag)
```

**What you learned.** State machines remember. Respecting (or probing) the
required order is a core skill for both building and attacking devices.

---

## Tier 0 done

You can now connect, enumerate, read values and descriptors, decode, write, and
drive a sequence - the daily bread of BLE. You also met the words that matter:
service, characteristic, handle, UUID, property, descriptor.

Next, the board stops waiting for you to ask and starts calling **you**:
**[Tier 1 - Notifications and Indications](Walkthrough-Tier-1-Notifications-and-Indications)**.
