#!/usr/bin/env python3
"""Create OFFLINE ONLY 802.11 fixtures; none are transmitted by SquachEmit.

Used to exercise SquachWatch's OUI and management-frame parsing in native
unit tests. These synthetic frames are not PCAP captures of real products.
"""
import json
import struct
from rid_vectors import messages
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "tests" / "offline_vectors.json"

def mac(s: str) -> bytes:
    b = bytes.fromhex(s.replace(":", ""))
    if len(b) != 6:
        raise ValueError(s)
    return b

def beacon(source: str, ssid: str, protected: bool = False,
           extra_ies: bytes = b"") -> bytes:
    addr = mac(source)
    head = b"\x80\x00\x00\x00" + b"\xff"*6 + addr + addr + b"\x00\x00"
    fixed = b"\x00"*8 + b"\x64\x00" + (b"\x11\x00" if protected else b"\x01\x00")
    data = ssid.encode("ascii")
    if len(data) > 32: raise ValueError(ssid)
    return head + fixed + bytes([0, len(data)]) + data + extra_ies

def probe(source: str) -> bytes:
    addr = mac(source)
    return b"\x40\x00\x00\x00" + b"\xff"*6 + addr + b"\xff"*6 + b"\x00\x00\x00\x00"

def deauth(source: str) -> bytes:
    # Single synthetic disconnection frame in a file, not a flood or RF code.
    addr = mac(source)
    return b"\xc0\x00\x00\x00" + b"\xff"*6 + addr + addr + b"\x00\x00" + b"\x03\x00"

vectors = []
def add(id, klass, frame, expected, notes):
    vectors.append({"id": id, "class": klass, "expected": expected,
                    "frame_hex": frame.hex(), "notes": notes,
                    "rf_transmitted": False})

add("OUI-FLOCK-OWN", "WIFI_OUI", beacon("b4:1e:52:00:00:01", "SQE-LAB"),
    "FLOCK", "Flock-owned MA-L, high OUI evidence; *offline-only*")
add("OUI-ALPR", "WIFI_OUI", beacon("00:04:7d:00:00:01", "SQE-LAB"),
    "ALPR", "Motorola Solutions MA-L; offline-only")
add("OUI-CAMERA", "WIFI_OUI", beacon("00:40:8c:00:00:01", "SQE-LAB"),
    "CAMERA", "Axis MA-L; offline-only")
add("OUI-RING", "WIFI_OUI", beacon("ac:9f:c3:00:00:01", "SQE-LAB"),
    "RING", "Ring MA-L; offline-only")
add("OUI-DRONE", "WIFI_OUI", beacon("04:a8:5a:00:00:01", "SQE-LAB"),
    "DRONE", "DJI MA-L; offline-only")

# All ten registered SZ DJI TECHNOLOGY MA-L prefixes. OUI is *manufacturer*
# evidence, not confirmed aircraft/RID. These beacons exist only in the file;
# on-air scenarios retain locally administered 02:/C2: test identities.
with (ROOT / "tests" / "dji_oui_registry.json").open(encoding="utf-8") as f:
    dji_oui_registry = json.load(f)
for record in dji_oui_registry["allocations"]:
    prefix = record["prefix"].lower()
    add("OUI-DJI-" + prefix.replace(":", ""), "WIFI_OUI",
        beacon(prefix + ":00:00:01", "SQE-LAB"), "DRONE",
        "DJI registered MA-L " + record["registered"] +
        "; vendor hint only, not aircraft/Remote ID; offline-only")
add("OUI-AUTEL-28", "WIFI_OUI", beacon("ec:5b:cd:e0:00:01", "SQE-LAB"),
    "DRONE", "Autel 28-bit registration fourth high nibble E")
add("OUI-AUTEL-NEIGHBOR", "WIFI_OUI", beacon("ec:5b:cd:d0:00:01", "SQE-LAB"),
    "NONE", "Same 24-bit prefix; different fourth high nibble D must NOT match")
add("OUI-SONOS-NEG", "WIFI_OUI", beacon("00:0e:58:00:00:01", "SQE-LAB"),
    "NONE", "Regression guard: removed incorrect ALPR Sonos prefix")
add("FLOCK-WILDCARD", "WIFI_PROBE", probe("00:03:7f:00:00:01"),
    "FLOCK", "Qualcomm source + wildcard probe => FLOCK MED")
add("FLOCK-WILDCARD-NEG", "WIFI_PROBE", probe("02:aa:bb:00:00:01"),
    "NONE", "Wildcard probe without a Flock-associated prefix")
add("DEAUTH-ONE", "WIFI_DEAUTH", deauth("02:aa:bb:00:00:01"),
    "NONE", "One deauth frame is normal; MUST NOT trigger flood threshold")
# The native detector must see BOTH over a suitable time window. Neither
# frame by itself is sufficient to assert EVILTWIN.
for i, protected, source in [(1, False, "02:aa:bb:00:00:01"),
                             (2, True,  "02:cc:dd:00:00:02")]:
    add(f"EVILTWIN-PAIR-{i}", "WIFI_EVILTWIN_SEQUENCE",
        beacon(source, "SQE-TWIN-LAB", protected),
        "SEQUENCE_ONLY", "Two BSSIDs with same SSID, conflicting protection; assert only after both")

# Pwnagotchi vendor IE with a synthetic lab name; includes a real string
# signature ('pwnd_tot') read by SquachWatch's passive frame scanner.
pwn = b'{"name":"sqe-lab","pwnd_tot":2,"version":"test"}'
add("PWNAGOTCHI-BEACON", "WIFI_PWNAGOTCHI",
    beacon("02:bb:aa:00:00:01", "SQE-LAB", extra_ies=bytes([221, len(pwn)])+pwn),
    "HACKER", "Synthetic vendor IE with pwnd_tot; offline-only")

# Wi-Fi ASTM F3411: vendor IE with a valid OpenDroneID message pack.
# The complete fake packet stays in this JSON file; not radiated by the S3.
pack = bytes([0xF2, 25, 1]) + messages()[0]
for id, prefix in [("RID-WIFI-BEACON", bytes.fromhex("fa0bbc0d01")),
                   ("RID-WIFI-PARROT", bytes.fromhex("903ae60d01"))]:
    body = prefix + pack
    add(id, "WIFI_REMOTEID", beacon("02:aa:bb:00:00:01", "SQE-LAB",
                                      extra_ies=bytes([221, len(body)]) + body),
        "DRONE", "Synthetic ASTM Remote ID Wi-Fi beacon vendor IE; offline-only")
bad = bytes.fromhex("fa0bbc0d01") + bytes([0xF2, 24, 1]) + messages()[0]
add("RID-WIFI-BAD-PACK", "WIFI_REMOTEID",
    beacon("02:aa:bb:00:00:01", "SQE-LAB", extra_ies=bytes([221, len(bad)]) + bad),
    "NONE", "Invalid message size, must not count as Remote ID")
# NAN public-action frame: destination and 6-byte service hash from the fork.
addr = mac("02:aa:bb:00:00:01")
nan = (b"\xd0\x00\x00\x00" + bytes.fromhex("516f9a010000") + addr +
       addr + b"\x00\x00" + bytes.fromhex("0409506f9a13") +
       bytes.fromhex("8869199d9209"))
add("RID-WIFI-NAN", "WIFI_REMOTEID", nan,
    "DRONE", "OpenDroneID NAN public-action service hash; offline-only")

DEST.write_text(json.dumps(vectors, indent=2) + "\n", encoding="utf-8")
print(f"Created {len(vectors)} offline-only fixtures: {DEST}")
