#!/usr/bin/env python3
"""Independent OFFLINE-ONLY BLE/802.11 format checks, not a receiver.

Reference concepts: ESP32Marauder RemoteIdDecoder and IBeacon host tests.
No SquachWatch classification is inferred from these fixture results.
"""
from __future__ import annotations
import struct


def remote_message(message: bytes) -> dict | None:
    if len(message) != 25:
        return None
    kind = message[0] >> 4
    if kind == 0:
        return {"kind": "basic", "uas_id": message[2:22].split(b"\0", 1)[0].decode("ascii", "replace")}
    if kind == 1:
        lat, lon = struct.unpack_from("<ii", message, 5)
        if abs(lat) > 900_000_000 or abs(lon) > 1_800_000_000:
            return None
        return {"kind": "location", "lat_e7": lat, "lon_e7": lon,
                "altitude_geo_m": struct.unpack_from("<H", message, 15)[0] / 2 - 1000}
    if kind == 4:
        lat, lon = struct.unpack_from("<ii", message, 2)
        return {"kind": "system", "operator_lat_e7": lat, "operator_lon_e7": lon}
    return None


def ble_remote_adv(adv: bytes) -> dict | None:
    if (len(adv) != 31 or adv[0] != 30 or adv[1] != 0x16 or
            adv[2:4] != bytes.fromhex("faff") or adv[4] != 0x0d):
        return None
    return remote_message(adv[6:31])


def beacon_remote_id(frame: bytes) -> dict | None:
    # Passive Wi-Fi beacon IE parser; no RF injection.
    if len(frame) < 36 or frame[0] != 0x80:
        return None
    p = 36
    while p + 2 <= len(frame):
        eid, length = frame[p], frame[p + 1]
        p += 2
        if p + length > len(frame):
            return None
        val = frame[p:p + length]
        p += length
        if eid != 221 or len(val) < 8:
            continue
        if val[:5] not in (bytes.fromhex("fa0bbc0d01"), bytes.fromhex("903ae60d01")):
            continue
        pack = val[5:]
        if len(pack) < 3 or pack[0] >> 4 != 15 or pack[1] != 25:
            return None
        count = pack[2]
        if count < 1 or len(pack) != 3 + count * 25:
            return None
        return remote_message(pack[3:28])
    return None


def wifi_nan_remote_id(frame: bytes) -> bool:
    return (len(frame) >= 36 and frame[0] == 0xd0 and
            frame[4:10] == bytes.fromhex("516f9a010000") and
            bytes.fromhex("506f9a138869199d9209") in frame[24:])


def parse_ibeacon_mfg(data: bytes) -> dict | None:
    if len(data) != 25 or data[:4] != bytes.fromhex("4c000215"):
        return None
    return {"uuid_hex": data[4:20].hex(),
            "major": int.from_bytes(data[20:22], "big"),
            "minor": int.from_bytes(data[22:24], "big"),
            "measured_power": struct.unpack("b", data[24:25])[0]}
