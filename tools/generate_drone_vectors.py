#!/usr/bin/env python3
"""Deterministic OFFLINE Remote ID fixtures for BLE4, BLE5, Wi-Fi Beacon/NAN.

Frames here are test data, NOT radio transmitters. In particular, a BLE5
extended payload cannot establish a physical BLE Coded PHY transmission.
Only BLE4 and ordinary SoftAP beacon vendor-IE are in on-air firmware.
"""
from __future__ import annotations
import argparse
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def basic(serial="SQE-OFFLINE-LAB"):
    m = bytearray(25)
    m[0:2] = b"\x02\x12"
    m[2:22] = serial.encode("ascii")[:20].ljust(20, b"\0")
    return bytes(m)


def location(timestamp=0xFFFF):
    m = bytearray(25)
    m[0] = 0x12
    struct.pack_into("<i", m, 5, 407128000)
    struct.pack_into("<i", m, 9, -740060000)
    struct.pack_into("<H", m, 15, 2240)
    struct.pack_into("<H", m, 21, timestamp)
    return bytes(m)


def system():
    m = bytearray(25)
    m[0] = 0x42
    struct.pack_into("<i", m, 2, 407580000)
    struct.pack_into("<i", m, 6, -739855000)
    return bytes(m)


def self_id(name="LAB SYNTHETIC DRONE"):
    m = bytearray(25)
    m[0] = 0x32
    m[2:25] = name.encode("ascii")[:23].ljust(23, b"\0")
    return bytes(m)


def pack(messages):
    if not 1 <= len(messages) <= 9 or any(len(m) != 25 for m in messages):
        raise ValueError("pack requires 1..9 messages of 25 bytes")
    return bytes([0xF2, 25, len(messages)]) + b"".join(messages)


def ble4(m, app=0x0D):
    # 31-byte legacy advertisement: single AD service-data structure.
    return bytes([30, 0x16, 0xFA, 0xFF, app, 1]) + m


def ble5(messages):
    # Advertising-data serialization for an *offline* extended-PHY fixture.
    data = bytes.fromhex("faff0d01") + pack(messages)
    return bytes([len(data) + 1, 0x16]) + data


def beacon(messages, parrot=False):
    frame = bytearray(36)
    frame[0] = 0x80
    oui = bytes.fromhex("903ae60d01" if parrot else "fa0bbc0d01")
    payload = oui + pack(messages)
    return bytes(frame) + bytes([0xDD, len(payload)]) + payload


def nan(valid=True):
    frame = bytearray(42)
    frame[0] = 0xD0
    frame[4:10] = bytes.fromhex("516f9a010000")
    frame[24:30] = bytes.fromhex("0409506f9a13")
    frame[36:42] = bytes.fromhex("8869199d9209" if valid else "8869199d9208")
    return bytes(frame)


def elapsed_since_hour_seconds(now_seconds, encoded_timestamp):
    if encoded_timestamp == 0xFFFF:
        return None
    if not 0 <= encoded_timestamp <= 36000:
        return None
    then_seconds = encoded_timestamp / 10
    return (now_seconds - then_seconds) % 3600


def build_vectors():
    m = [basic(), location(), system(), self_id()]
    result = []
    def add(name, transport, payload, expected, explanation, **kwargs):
        result.append(dict(id=name, transport=transport, frame_hex=payload.hex(),
                           expected=expected, notes=explanation,
                           rf_transmitted=False, **kwargs))
    add("RID-BLE4-BASIC", "BLE4_LEGACY", ble4(m[0]), "DRONE",
        "Valid test Basic ID over ordinary legacy service data")
    add("RID-BLE5-EXT-PACK", "BLE5_EXTENDED", ble5(m), "DRONE",
        "Extended advertising-data geometry; coded PHY NOT transmitted", phy="LE_CODED_TEST_ONLY")
    add("RID-BLE5-NEG-TRUNC", "BLE5_EXTENDED", ble5(m)[:-9], "NONE",
        "Corrupted/truncated message pack")
    add("RID-WIFI-BEACON-LAB", "WIFI_BEACON", beacon([m[0]]), "DRONE",
        "Generic ASTM vendor IE, passive fixture")
    add("RID-WIFI-PARROT-LAB", "WIFI_BEACON", beacon([m[0]], parrot=True), "DRONE",
        "Parrot-style vendor OUI, synthetic record only")
    add("RID-WIFI-NEG-PACK", "WIFI_BEACON", beacon([m[0]])[:-4], "NONE",
        "Declared length exceeds actual frame")
    add("RID-WIFI-NAN-LAB", "WIFI_NAN", nan(), "DRONE",
        "NAN service-hash presence fixture, not an on-air NAN frame")
    add("RID-WIFI-NAN-NEG", "WIFI_NAN", nan(False), "NONE",
        "Wrong NAN service hash must not match")
    add("RID-BLE-NEG-APP", "BLE4_LEGACY", ble4(m[0], 0x0E), "NONE",
        "Wrong OpenDroneID application code")
    add("RID-BLE-NEG-SHORT", "BLE4_LEGACY", ble4(m[0])[:-1], "NONE",
        "Truncated 25-byte Basic ID")
    bad = bytearray(m[0]); bad[1] = 0xF2
    add("RID-IDTYPE-INVALID", "BLE4_LEGACY", ble4(bytes(bad)), "DRONE",
        "Invalid Basic ID type F; transport remains recognizable", invalid_idtype=True)
    add("RID-NO-OPERATOR", "BLE5_EXTENDED", ble5(m[:2]), "DRONE",
        "Valid Basic+Location; missing operator System, not a negative detection", missing_operator=True)
    old = location(timestamp=1)
    add("RID-STALE-LOCATION", "BLE5_EXTENDED", ble5([m[0], old, m[2]]), "DRONE",
        "Old time-of-hour at reference time 3500s; transport still valid",
        location_timestamp_tenths=1, reference_seconds_after_hour=3500,
        age_seconds=elapsed_since_hour_seconds(3500, 1))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "tests" / "drone_vectors.json")
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    result = build_vectors()
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"Generated {len(result)} offline-only Remote ID vector cases: {args.output}")


if __name__ == "__main__":
    main()
