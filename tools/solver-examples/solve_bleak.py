#!/usr/bin/env python3
"""BLEtterCTF reference solver and self-test, using bleak.

Attempts every flag the current firmware implements (15 of them, tiers T0-T3
plus a long read), submits each one, and prints a pass/fail summary. Handy as a
teaching aid and as an end-to-end smoke test after flashing.

Flags covered:
  T0: 1 (read), 2 (read), 3 (descriptor), 5 (hex), 6 (base64), 7 (3 pieces),
      8 (write-to-unlock), 9 (state machine)
  T1: 10 (notify), 11 (indicate), 12 (chunked notify)
  T2: 15 (adv mfg data), 16 (scan response)
  T3: 21 (encrypted read, needs pairing - flaky in bleak on some platforms)
  T6: 36 (long read / read blob)

Usage:
  pip install bleak
  python solve_bleak.py

Note: bleak's pairing support is uneven; flag 21 may be skipped on some OSes.
Use nRF Connect or bluetoothctl for the security tier if it fails here.
"""

import asyncio
import base64

from bleak import BleakClient, BleakScanner

DEVICE_NAME = "BLEtterCTF"


def u(x: int) -> str:
    return f"0000{x:04x}-0000-1000-8000-00805f9b34fb"


SCORE   = u(0xFF01)
SUBMIT  = u(0xFF02)
F1      = u(0xFF03)
F3_CHR  = u(0xFF04)
F8      = u(0xFF05)
F10     = u(0xFF06)
F2      = u(0xFF07)
F5_HEX  = u(0xFF08)
F6_B64  = u(0xFF09)
F7A, F7B, F7C = u(0xFF0A), u(0xFF0B), u(0xFF0C)
F9      = u(0xFF0D)
F11     = u(0xFF0E)
F12     = u(0xFF0F)
F21_ENC = u(0xFF13)
F36     = u(0xFF14)

UNLOCK_KEY = b"d34dbeef"

results = {}   # flag id -> (ok: bool, detail: str)


def record(fid, ok, detail=""):
    results[fid] = (ok, detail)
    mark = "PASS" if ok else "FAIL"
    print(f"  [{mark}] flag {fid}: {detail}")


async def submit(client, flag: bytes) -> bool:
    """Write a flag to the submit char; return True if the score increased."""
    before = (await client.read_gatt_char(SCORE)).decode(errors="replace")
    await client.write_gatt_char(SUBMIT, flag, response=True)
    after = (await client.read_gatt_char(SCORE)).decode(errors="replace")
    return before != after


async def read_descriptor_0x2901(client, char_uuid):
    for service in client.services:
        for ch in service.characteristics:
            if ch.uuid.lower() == char_uuid.lower():
                for d in ch.descriptors:
                    if d.uuid.startswith("00002901"):
                        return await client.read_gatt_descriptor(d.handle)
    return None


async def grab(client, uuid, timeout=5.0, collect=False):
    """Subscribe and capture notification(s)/indication(s)."""
    chunks = []
    done = asyncio.Event()

    def cb(_h, data: bytearray):
        chunks.append(bytes(data))
        if not collect:
            done.set()

    await client.start_notify(uuid, cb)
    try:
        if collect:
            await asyncio.sleep(timeout)          # gather a burst
        else:
            await asyncio.wait_for(done.wait(), timeout=timeout)
    except asyncio.TimeoutError:
        pass
    finally:
        await client.stop_notify(uuid)
    return b"".join(chunks) if collect else (chunks[0] if chunks else None)


async def try_flag(fid, coro):
    try:
        await coro
    except Exception as e:                         # noqa: BLE001 - test harness
        record(fid, False, f"error: {e}")


async def register_bluez_agent():
    """Best-effort: register a NoInputNoOutput BlueZ agent so Just Works pairing
    (flag 21) auto-completes on Linux. Without a registered agent, BlueZ rejects
    the pairing with 'AuthenticationFailed'. No-op / harmless on macOS/Windows,
    where the OS handles pairing natively.
    """
    try:
        from dbus_fast.aio import MessageBus
        from dbus_fast.service import ServiceInterface, method
        from dbus_fast import BusType
    except Exception:
        return None  # not a dbus/BlueZ platform

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
        return bus  # keep alive for the process lifetime
    except Exception:
        return None


async def main():
    _agent_bus = await register_bluez_agent()   # enables flag 21 pairing on Linux
    print(f"scanning (active) for {DEVICE_NAME} ...")
    device = None
    adv_flag15 = adv_flag16 = None

    found = await BleakScanner.discover(timeout=8.0, return_adv=True)
    for d, adv in found.values():
        if adv.local_name == DEVICE_NAME:
            device = d
            for cid, payload in (adv.manufacturer_data or {}).items():
                if cid == 0xFFFF:
                    adv_flag15 = payload         # ADV
                elif cid == 0xFFFE:
                    adv_flag16 = payload         # scan response
            break

    if device is None:
        print("device not found; is it powered and advertising?")
        return
    print(f"found {device.address}")
    print(f"adv flag15 seen: {bool(adv_flag15)}, scan-rsp flag16 seen: {bool(adv_flag16)}\n")

    # Advertising flags 15/16 were captured during the scan above; they are
    # submitted after connecting, since submitting is a GATT write to 0xFF02.
    async with BleakClient(device) as client:
        print("connected; walking flags:\n")

        # T0
        await try_flag(1,  _simple(client, 1, F1))
        await try_flag(2,  _simple(client, 2, F2))
        await try_flag(3,  _descriptor(client))
        await try_flag(5,  _hex(client))
        await try_flag(6,  _b64(client))
        await try_flag(7,  _pieces(client))
        await try_flag(8,  _unlock(client))
        await try_flag(9,  _state(client))

        # T1
        await try_flag(10, _notify(client, 10, F10))
        await try_flag(11, _notify(client, 11, F11))
        await try_flag(12, _multi(client))

        # T2 (values captured during the scan above)
        await try_flag(15, _adv(client, 15, adv_flag15))
        await try_flag(16, _adv(client, 16, adv_flag16))

        # T3
        await try_flag(21, _encrypted(client))

        # T6
        await try_flag(36, _simple(client, 36, F36))

        final = (await client.read_gatt_char(SCORE)).decode(errors="replace")
        print(f"\nfinal score: {final}")

    ok = sum(1 for v in results.values() if v[0])
    print(f"summary: {ok}/{len(results)} flags captured by this run")


# --- per-flag routines ------------------------------------------------------

async def _simple(client, fid, uuid):
    val = await client.read_gatt_char(uuid)
    record(fid, await submit(client, val), val.decode(errors="replace"))

async def _descriptor(client):
    val = await read_descriptor_0x2901(client, F3_CHR)
    if val is None:
        record(3, False, "no 0x2901 descriptor found")
        return
    record(3, await submit(client, val), val.decode(errors="replace"))

async def _hex(client):
    raw = (await client.read_gatt_char(F5_HEX)).decode()
    flag = bytes.fromhex(raw)
    record(5, await submit(client, flag), flag.decode(errors="replace"))

async def _b64(client):
    raw = (await client.read_gatt_char(F6_B64)).decode()
    flag = base64.b64decode(raw)
    record(6, await submit(client, flag), flag.decode(errors="replace"))

async def _pieces(client):
    a = await client.read_gatt_char(F7A)
    b = await client.read_gatt_char(F7B)
    c = await client.read_gatt_char(F7C)
    flag = a + b + c
    record(7, await submit(client, flag), flag.decode(errors="replace"))

async def _unlock(client):
    await client.write_gatt_char(F8, UNLOCK_KEY, response=True)
    flag = await client.read_gatt_char(F8)
    record(8, await submit(client, flag), flag.decode(errors="replace"))

async def _state(client):
    for step in (b"1", b"2", b"3"):
        await client.write_gatt_char(F9, step, response=True)
    flag = await client.read_gatt_char(F9)
    record(9, await submit(client, flag), flag.decode(errors="replace"))

async def _notify(client, fid, uuid):
    flag = await grab(client, uuid, timeout=6.0)
    if flag is None:
        record(fid, False, "no notification/indication received")
        return
    record(fid, await submit(client, flag), flag.decode(errors="replace"))

async def _multi(client):
    flag = await grab(client, F12, timeout=3.0, collect=True)
    if not flag:
        record(12, False, "no chunks received")
        return
    record(12, await submit(client, flag), flag.decode(errors="replace"))

async def _adv(client, fid, payload):
    if not payload:
        record(fid, False, "not seen in advertising/scan-response")
        return
    record(fid, await submit(client, payload), payload.decode(errors="replace"))

async def _encrypted(client):
    try:
        await asyncio.wait_for(client.pair(), timeout=25)
    except Exception:                              # noqa: BLE001
        pass  # some platforms pair implicitly on the encrypted read
    flag = await asyncio.wait_for(client.read_gatt_char(F21_ENC), timeout=15)
    record(21, await submit(client, flag), flag.decode(errors="replace"))


if __name__ == "__main__":
    asyncio.run(main())
