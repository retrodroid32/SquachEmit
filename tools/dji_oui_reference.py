#!/usr/bin/env python3
"""Offline DJI MAC OUI registry helper. Vendor allocation does NOT prove a drone."""
import argparse
import json
import re
from pathlib import Path

REFERENCE = Path(__file__).with_name("dji_oui_reference.json")

def normalize_mac(value):
    value=value.strip()
    patterns=[
        r"(?:[0-9a-fA-F]{2}:){5}[0-9a-fA-F]{2}",
        r"(?:[0-9a-fA-F]{2}-){5}[0-9a-fA-F]{2}",
        r"[0-9a-fA-F]{12}",
        r"(?:[0-9a-fA-F]{4}\.){2}[0-9a-fA-F]{4}",
    ]
    if not any(re.fullmatch(pattern,value) for pattern in patterns):
        raise ValueError("Invalid 48-bit MAC address")
    raw=re.sub(r"[:-]|\.", "", value).upper()
    return ":".join(raw[i:i+2] for i in range(0,12,2))

def classify(value, reference=None):
    data=reference if reference is not None else json.loads(REFERENCE.read_text())
    mac=normalize_mac(value)
    first=int(mac[:2],16)
    local=bool(first&2)
    multicast=bool(first&1)
    found=next((entry for entry in data["prefixes"] if
                mac.startswith(entry["prefix"]+":")),None)
    trusted=found is not None and not local and not multicast
    return dict(mac=mac,
                classification="DJI_REGISTERED_OUI" if trusted else "UNKNOWN_VENDOR",
                registry_vendor=data["vendor"] if trusted else None,
                registration_date=found["registered"] if trusted else None,
                locally_administered=local, multicast=multicast,
                is_drone=None, is_remote_id=None, model=None)

if __name__=="__main__":
    parser=argparse.ArgumentParser(description="Passive vendor-OUI lookup, not a drone classifier")
    parser.add_argument("mac", help="Observed MAC address")
    args=parser.parse_args()
    try:
        print(json.dumps(classify(args.mac),indent=2))
    except ValueError as exc:
        parser.error(str(exc))
