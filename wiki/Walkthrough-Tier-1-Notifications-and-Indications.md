# Walkthrough - Tier 1: Notifications and Indications

> Assumes you finished [Tier 0](Walkthrough-Tier-0-GATT-Basics). Same
> beginner-proof style: idea, how BLE does it, do it, takeaway.

## The big idea: let the device call you

In Tier 0 *you* did all the asking: read, write, read. That works, but imagine a
heart-rate monitor. Do you want to *ask* "what is the rate now?" a hundred times a
second? No. You want the device to **tell you the moment it changes**. That is
what this tier is about: the server pushing data to the client without being
polled.

This matters hugely for battery life and responsiveness, and it is how buttons,
sensors, and beacons actually behave in the real world.

## How BLE pushes data: the CCCD switch

A characteristic can have the **Notify** or **Indicate** property. But the server
does not push to everyone all the time - each client must first **subscribe**.
Subscribing means writing to a special descriptor attached to the characteristic:
the **CCCD** (Client Characteristic Configuration Descriptor, UUID `0x2902`).

- Write `0x0001` to the CCCD -> "start sending me **notifications**".
- Write `0x0002` to the CCCD -> "start sending me **indications**".
- Write `0x0000` -> "stop".

Your library hides this: calling `start_notify(uuid, callback)` writes the CCCD
for you and registers your callback. In nRF Connect it is the triple-arrow icon;
in `bluetoothctl` it is `notify on`.

**Notify vs Indicate** - two flavors of push:

- **Notify**: server sends, does *not* wait for acknowledgement. Fast, and it
  does not care whether you were listening. Use for a firehose of sensor samples.
- **Indicate**: server sends and *waits for the client to ACK* each message.
  Slower, but guaranteed delivery. Use for data you cannot afford to lose. Your
  stack sends the ACK automatically - you just subscribe.

---

## Flag 10 - ring ring: your first notification

**The idea.** Subscribe to a characteristic and the device pushes you a value.
You are not reading - you are *listening*.

**How BLE does it.** `start_notify` writes `0x0001` to the CCCD of `0xFF06`. The
firmware sees the subscription and immediately fires one notification carrying the
flag. Your callback receives it.

**Do it.**

```python
import asyncio
got = asyncio.Event(); box = {}
def on_notify(_handle, data):        # called when a notification arrives
    box["flag"] = bytes(data); got.set()

await client.start_notify(u(0xFF06), on_notify)
await asyncio.wait_for(got.wait(), timeout=5)   # wait for the push
await client.stop_notify(u(0xFF06))
await submit(client, box["flag"])
```

If nothing arrives, you probably subscribed to the wrong characteristic, or your
callback is not wired. Notifications are fire-and-forget: miss the moment and you
may need to re-subscribe.

**What you learned.** Subscribe = flip the CCCD switch. Then be ready with a
callback, because the data arrives on the device's schedule, not yours.

---

## Flag 11 - read receipts on: indications

**The idea.** Same as flag 10, but this characteristic uses **Indicate** - the
reliable, acknowledged flavor.

**How BLE does it.** `0xFF0E` has the Indicate property. When you subscribe, your
stack writes `0x0002` to the CCCD, and for every message it sends the server an
ACK under the hood. From your code it looks identical - `start_notify` handles
both notify and indicate.

**Do it.** Exactly the same code as flag 10, pointed at `0xFF0E`:

```python
got = asyncio.Event(); box = {}
def cb(_h, data): box["flag"] = bytes(data); got.set()
await client.start_notify(u(0xFF0E), cb)
await asyncio.wait_for(got.wait(), timeout=5)
await client.stop_notify(u(0xFF0E))
await submit(client, box["flag"])
```

**What you learned.** Indicate trades speed for a delivery guarantee. Choose
indicate for a config change you must not lose, notify for a stream where the odd
dropped sample does not matter.

---

## Flag 12 - the flag that would not fit: reassembly over the air

**The idea.** A single notification is small. Anything bigger than one packet gets
sent as **several** notifications, and reassembling them is your job.

**How BLE does it.** Remember the **MTU** (Maximum Transmission Unit) - the
biggest ATT payload per packet, only **23 bytes by default** (about 20 usable).
The firmware splits this flag into three chunks and sends three notifications on
`0xFF0F`. You collect them all and join them in arrival order.

**Do it.** Collect for a moment instead of stopping at the first message.

```python
parts = []
def cb(_h, data): parts.append(bytes(data))
await client.start_notify(u(0xFF0F), cb)
await asyncio.sleep(2)               # gather the burst
await client.stop_notify(u(0xFF0F))
await submit(client, b"".join(parts))
```

**What you learned.** BLE packets are tiny; large values are chunked and you
reassemble them. This is extremely common with real notify streams (and it is why
BLE 5 added bigger payloads - a later tier).

---

## Flag 13 - knock first, then listen: notify meets write

**The idea.** Real devices rarely spam notifications for no reason. Usually you
**arm** or **request** something with a write, and *then* the device notifies. A
"start measurement" command, for example, followed by results streaming back.

**How BLE does it.** `0xFF15` has *both* Write and Notify. First you subscribe
(so you are listening), then you **write** to it (the trigger). The firmware, on
seeing your write, fires the notification carrying the flag. Order matters: if you
are not subscribed when the notification goes out, you miss it - so subscribe
first, then trigger.

**Do it.**

```python
box = {}; got = asyncio.Event()
def cb(_h, data): box["flag"] = bytes(data); got.set()

await client.start_notify(u(0xFF15), cb)          # 1) listen
await client.write_gatt_char(u(0xFF15), b"go", response=True)  # 2) trigger
await asyncio.wait_for(got.wait(), timeout=5)     # 3) receive
await client.stop_notify(u(0xFF15))
await submit(client, box["flag"])
```

**What you learned.** The classic real pattern: **subscribe, then command, then
receive results**. Getting the order wrong (triggering before subscribing) is a
common reason "the notification never came".

---

## Tier 1 done

You now handle both directions of GATT: **pull** (read/write from Tier 0) and
**push** (notify/indicate), including chunked streams and write-triggered pushes.
You understand the CCCD switch and why notify and indicate exist.

Next you learn to read data from a device **without connecting to it at all**:
**[Tier 2 - Advertising](Walkthrough-Tier-2-Advertising)**.
