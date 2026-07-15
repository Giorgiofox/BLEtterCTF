# Walkthrough - Tier 1: Notifications and Indications

So far you did all the talking: you read, you wrote, the device answered. Tier 1
flips it. Now the device calls **you**. This is how heart-rate monitors, buttons,
and sensors push data the instant it changes, instead of you polling like an
anxious parent.

The mechanism is **subscription**. A characteristic with the Notify or Indicate
property has a hidden switch called the **CCCD (Client Characteristic
Configuration Descriptor, 0x2902)**. You flip it on, and the server starts
pushing. Your tool does this for you when you "subscribe" / "enable
notifications".

- **Notify** - server pushes, no acknowledgement. Fast, and it does not care if
  you were listening.
- **Indicate** - server pushes and waits for your ACK. Slower, reliable. The
  stack ACKs automatically; you just subscribe.

---

## Flag 10 - ring ring (notify on 0xFF06)

Subscribe to `0xFF06`. The moment you do, the device sends one notification
carrying the flag. Your job is simply to be listening.

```python
got = asyncio.Event(); box = {}
def cb(_h, data): box["f"] = data; got.set()
await c.start_notify(u(0xFF06), cb)
await asyncio.wait_for(got.wait(), timeout=5)
await c.write_gatt_char(u(0xFF02), box["f"], response=True)
```

- nRF Connect: tap the triple-arrow (subscribe) icon on `0xFF06`; the value
  appears.
- bluetoothctl: `select-attribute <0xFF06>` then `notify on`.

If you subscribe and nothing arrives, you probably subscribed to the wrong
characteristic, or your callback is not wired. Notifications are fire-and-forget:
miss the moment and you may need to re-subscribe.

---

## Flag 11 - read receipts on (indicate on 0xFF0E)

Same as flag 10, but `0xFF0E` uses **Indicate**. From your side the code is
identical - `start_notify` handles both - but under the hood your stack sends an
ACK for each message. Subscribe, receive, submit.

The lesson: indicate trades speed for a guarantee of delivery. Use it for data
you cannot afford to lose (a configuration change), notify for a firehose (sensor
samples).

---

## Flag 12 - the flag that would not fit (multi-notify on 0xFF0F)

Subscribe to `0xFF0F` and you receive **several** notifications, each carrying a
**piece** of the flag. Concatenate them in arrival order to rebuild it.

```python
parts = []
done = asyncio.Event()
def cb(_h, data):
    parts.append(data)
    # heuristic: stop when a short/last chunk arrives, or after a timeout
await c.start_notify(u(0xFF0F), cb)
await asyncio.sleep(2)                 # collect the burst
flag = b"".join(parts)
```

The lesson: BLE packets are small (remember the ~20-byte MTU from the
[primer](BLE-Primer)). Anything bigger than one packet gets **chunked**, and
reassembly is your problem. This is extremely common with real notify streams.

---

## Tier 1 done

You now handle both directions of GATT: pull (read/write) and push
(notify/indicate), including chunked streams. Next you learn to find and read a
device **without even connecting** to it:
**[Tier 2 - Advertising](Walkthrough-Tier-2-Advertising)**.
