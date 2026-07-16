#!/usr/bin/env python3
"""BLEtterCTF reference solver and self-test (all 20 implemented flags), via bleak.

Walks every implemented flag, submits each, and verifies captures against the
live scoreboard NOTIFICATIONS (0xFF01) - not reads, because BlueZ caches GATT
reads and a re-read can show a stale score. Prints a pass/fail summary.

No BLEtterCap required. On Linux it also registers a NoInputNoOutput BlueZ
pairing agent so the security flags (21, 24) pair automatically; without an agent
BlueZ rejects Just Works pairing with 'AuthenticationFailed'.

Usage:
    pip install bleak            # plus dbus-fast on Linux (bleak pulls it in)
    python solve_bleak.py
"""

import asyncio
import base64
import re

from bleak import BleakClient, BleakScanner

DEVICE_NAME_PREFIX = "BLEtterCTF"
TOTAL = 20


def u(x: int) -> str:
    return f"0000{x:04x}-0000-1000-8000-00805f9b34fb"


SCORE, SUBMIT = u(0xFF01), u(0xFF02)
F1, F3_CHR, F8, F10 = u(0xFF03), u(0xFF04), u(0xFF05), u(0xFF06)
F2, F5_HEX, F6_B64 = u(0xFF07), u(0xFF08), u(0xFF09)
F7A, F7B, F7C = u(0xFF0A), u(0xFF0B), u(0xFF0C)
F9_STATE, F11, F12 = u(0xFF0D), u(0xFF0E), u(0xFF0F)
F21_ENC, F36_LONG = u(0xFF13), u(0xFF14)
F13_TRIG, F24_BOND, F35_FUZZ, F4_NAME = u(0xFF15), u(0xFF17), u(0xFF19), u(0xFF1A)
SVC_UUID = u(0xFF00)

_score = {"n": 0}
_passed = {"n": 0}


def _on_score(_handle, data: bytearray):
    m = re.search(rb"(\d+)/", bytes(data))
    if m:
        _score["n"] = int(m.group(1))


async def submit(client, flag: bytes, fid: int, shown: str):
    before = _score["n"]
    await client.write_gatt_char(SUBMIT, flag, response=True)
    for _ in range(30):
        if _score["n"] > before:
            break
        await asyncio.sleep(0.1)
    ok = _score["n"] > before
    _passed["n"] += 1 if ok else 0
    print(f"  [{'PASS' if ok else 'FAIL'}] flag {fid}: {shown}  ({_score['n']}/{TOTAL})")


# --- Linux: register a Just Works pairing agent (no-op elsewhere) ------------
async def register_bluez_agent():
    try:
        from dbus_fast.aio import MessageBus
        from dbus_fast.service import ServiceInterface, method
        from dbus_fast import BusType
    except Exception:
        return None

    class _Agent(ServiceInterface):
        def __init__(self):
            super().__init__("org.bluez.Agent1")
        @method()
        def Release(self): pass
        @method()
        def RequestConfirmation(self, device: 'o', passkey: 'u'): pass
        @method()
        def RequestAuthorization(self, device: 'o'): pass
        @method()
        def AuthorizeService(self, device: 'o', uuid: 's'): pass
        @method()
        def RequestPasskey(self, device: 'o') -> 'u': return 0
        @method()
        def RequestPinCode(self, device: 'o') -> 's': return "0000"
        @method()
        def Cancel(self): pass

    try:
        bus = await MessageBus(bus_type=BusType.SYSTEM).connect()
        bus.export("/blectf/agent", _Agent())
        intro = await bus.introspect("org.bluez", "/org/bluez")
        am = bus.get_proxy_object("org.bluez", "/org/bluez", intro) \
                .get_interface("org.bluez.AgentManager1")
        await am.call_register_agent("/blectf/agent", "NoInputNoOutput")
        await am.call_request_default_agent("/blectf/agent")
        print("  (BlueZ Just Works agent registered)")
        return bus
    except Exception:
        return None


# --- notification helpers ---------------------------------------------------
async def read(client, uuid):
    return await asyncio.wait_for(client.read_gatt_char(uuid), timeout=12)


async def read_desc_2901(client, char_uuid):
    for service in client.services:
        for ch in service.characteristics:
            if ch.uuid.lower() == char_uuid.lower():
                for d in ch.descriptors:
                    if d.uuid.startswith("00002901"):
                        return await client.read_gatt_descriptor(d.handle)
    return None


async def grab(client, uuid, timeout=6.0, collect=False):
    chunks, done = [], asyncio.Event()

    def cb(_h, data):
        chunks.append(bytes(data))
        if not collect:
            done.set()

    await client.start_notify(uuid, cb)
    try:
        if collect:
            await asyncio.sleep(timeout)
        else:
            await asyncio.wait_for(done.wait(), timeout=timeout)
    except asyncio.TimeoutError:
        pass
    finally:
        await client.stop_notify(uuid)
    return b"".join(chunks) if collect else (chunks[0] if chunks else None)


async def trigger_notify(client, uuid, trigger=b"go", timeout=5.0):
    box, done = {}, asyncio.Event()

    def cb(_h, data):
        box["v"] = bytes(data)
        done.set()

    await client.start_notify(uuid, cb)
    await client.write_gatt_char(uuid, trigger, response=True)
    try:
        await asyncio.wait_for(done.wait(), timeout=timeout)
    except asyncio.TimeoutError:
        pass
    finally:
        await client.stop_notify(uuid)
    return box.get("v")


async def main():
    await register_bluez_agent()
    print(f"scanning (active) for {DEVICE_NAME_PREFIX} ...")

    device = adv15 = adv16 = svc17 = None
    for d, adv in (await BleakScanner.discover(timeout=10.0, return_adv=True)).values():
        if (adv.local_name or "").startswith(DEVICE_NAME_PREFIX):
            device = d
            for cid, payload in (adv.manufacturer_data or {}).items():
                if cid == 0xFFFF:
                    adv15 = payload           # flag 15 (advertisement)
                elif cid == 0xFFFE:
                    adv16 = payload           # flag 16 (scan response)
            for su, sd in (adv.service_data or {}).items():
                if su.lower() == SVC_UUID:
                    svc17 = sd                # flag 17 (service data)
            break

    if device is None:
        print("device not found; is it powered and advertising?")
        return
    print(f"found {device.address}\n")

    async with BleakClient(device, timeout=25) as c:
        await c.start_notify(SCORE, _on_score)

        # T0 - GATT basics
        v = await read(c, F1);  await submit(c, v, 1, v.decode())
        v = await read(c, F2);  await submit(c, v, 2, v.decode())
        v = await read_desc_2901(c, F3_CHR); await submit(c, v, 3, v.decode())
        dn = (await read(c, F4_NAME)).decode(errors="replace")
        m = re.search(r"flag\{[^}]*\}", dn)
        await submit(c, (m.group(0) if m else dn).encode(), 4, dn)
        v = bytes.fromhex((await read(c, F5_HEX)).decode()); await submit(c, v, 5, v.decode())
        v = base64.b64decode((await read(c, F6_B64)).decode()); await submit(c, v, 6, v.decode())
        v = (await read(c, F7A)) + (await read(c, F7B)) + (await read(c, F7C))
        await submit(c, v, 7, v.decode())
        await c.write_gatt_char(F8, b"d34dbeef", response=True)
        v = await read(c, F8); await submit(c, v, 8, v.decode())
        for step in (b"1", b"2", b"3"):
            await c.write_gatt_char(F9_STATE, step, response=True)
        v = await read(c, F9_STATE); await submit(c, v, 9, v.decode())

        # T1 - notifications / indications
        v = await grab(c, F10, 6);  await submit(c, v, 10, v.decode() if v else "none")
        v = await grab(c, F11, 6);  await submit(c, v, 11, v.decode() if v else "none")
        v = await grab(c, F12, 3, collect=True); await submit(c, v, 12, v.decode() if v else "none")
        v = await trigger_notify(c, F13_TRIG);   await submit(c, v, 13, v.decode() if v else "none")

        # T2 - advertising (captured during the scan above)
        await submit(c, adv15, 15, adv15.decode() if adv15 else "none")
        await submit(c, adv16, 16, adv16.decode() if adv16 else "none")
        await submit(c, svc17, 17, svc17.decode() if svc17 else "none")

        # T3 - security (agent enables Just Works pairing; bonds the link)
        v = bytes(await read(c, F21_ENC)); await submit(c, v, 21, v.decode())
        v = bytes(await read(c, F24_BOND)); await submit(c, v, 24, v.decode())

        # T6 - fuzzing + long read
        await c.write_gatt_char(F35_FUZZ, b"", response=True)  # empty write = edge case
        v = await read(c, F35_FUZZ); await submit(c, v, 35, v.decode())
        v = await read(c, F36_LONG); await submit(c, v, 36, v.decode())

        print(f"\nsummary: {_passed['n']}/{TOTAL} captured; device score {_score['n']}/{TOTAL}")


if __name__ == "__main__":
    asyncio.run(main())
