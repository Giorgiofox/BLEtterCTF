#!/usr/bin/env python3
"""Reference BLEtterCTF solver for the scaffold flags (T0-T2), using bleak.

Walks the mechanics one at a time and submits each flag it finds:
  - flag 1  : direct read of 0xFF03
  - flag 3  : read the 0x2901 user description of 0xFF04
  - flag 8  : write the unlock key to 0xFF05, then read it back
  - flag 10 : subscribe to 0xFF06 (notification)
  - flag 15 : parse the manufacturer-specific advertising data

This is a teaching aid. The real game is meant to be played with the hint
ladder, not auto-solved.
"""

import asyncio

from bleak import BleakClient, BleakScanner

DEVICE_NAME = "BLEtterCTF"

# 16-bit UUIDs expand to the full base UUID
def u(x: int) -> str:
    return f"0000{x:04x}-0000-1000-8000-00805f9b34fb"

SCORE   = u(0xFF01)
SUBMIT  = u(0xFF02)
F_READ  = u(0xFF03)
F_DESC  = u(0xFF04)
F_WRITE = u(0xFF05)
F_NOTIFY = u(0xFF06)
UNLOCK_KEY = b"d34dbeef"


async def submit(client: BleakClient, flag: bytes) -> None:
    await client.write_gatt_char(SUBMIT, flag, response=True)
    score = await client.read_gatt_char(SCORE)
    print(f"  submitted {flag!r} -> {score.decode(errors='replace')}")


async def read_user_description(client: BleakClient, char_uuid: str):
    """Return the 0x2901 descriptor value of a characteristic, if present."""
    for service in client.services:
        for ch in service.characteristics:
            if ch.uuid.lower() == char_uuid.lower():
                for d in ch.descriptors:
                    if d.uuid.startswith("00002901"):
                        return await client.read_gatt_descriptor(d.handle)
    return None


async def main() -> None:
    print(f"scanning for {DEVICE_NAME} ...")
    device = None
    adv_flag = None

    devices = await BleakScanner.discover(timeout=8.0, return_adv=True)
    for d, adv in devices.values():
        if adv.local_name == DEVICE_NAME:
            device = d
            # flag 15: manufacturer data, company id 0xFFFF, payload = flag
            for cid, payload in (adv.manufacturer_data or {}).items():
                if cid == 0xFFFF:
                    adv_flag = payload
            break

    if device is None:
        print("device not found; is it powered and advertising?")
        return

    print(f"found {device.address}")

    async with BleakClient(device) as client:
        # flag 1: direct read
        f1 = await client.read_gatt_char(F_READ)
        print(f"flag 1 (read 0xFF03): {f1!r}")
        await submit(client, f1)

        # flag 3: descriptor
        f3 = await read_user_description(client, F_DESC)
        if f3:
            print(f"flag 3 (0x2901 descriptor): {f3!r}")
            await submit(client, f3)

        # flag 8: write to unlock, then read
        await client.write_gatt_char(F_WRITE, UNLOCK_KEY, response=True)
        f8 = await client.read_gatt_char(F_WRITE)
        print(f"flag 8 (write-to-unlock 0xFF05): {f8!r}")
        await submit(client, f8)

        # flag 10: subscribe for a notification
        got = asyncio.Event()
        holder = {}

        def on_notify(_handle, data: bytearray):
            holder["flag"] = bytes(data)
            got.set()

        await client.start_notify(F_NOTIFY, on_notify)
        try:
            await asyncio.wait_for(got.wait(), timeout=5.0)
            f10 = holder["flag"]
            print(f"flag 10 (notify 0xFF06): {f10!r}")
            await submit(client, f10)
        except asyncio.TimeoutError:
            print("flag 10: no notification received")
        finally:
            await client.stop_notify(F_NOTIFY)

        # flag 15: from advertising manufacturer data captured during the scan
        if adv_flag:
            print(f"flag 15 (adv mfg data): {adv_flag!r}")
            await submit(client, adv_flag)

        final = await client.read_gatt_char(SCORE)
        print(f"\nfinal score: {final.decode(errors='replace')}")


if __name__ == "__main__":
    asyncio.run(main())
