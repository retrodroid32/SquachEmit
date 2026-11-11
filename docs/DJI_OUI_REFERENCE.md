# DJI MAC address vendor prefixes (passive reference)

**Verified 2026-11-10** against MACLookup and Udger. Registered organization:
**SZ DJI TECHNOLOGY CO.,LTD** (Udger ID **10425**, code **sz_dji_technology_co-ltd**).

These allocations are **not drone or Remote ID signatures**. An OUI match
only indicates that a globally administered address belongs to the specified
vendor registry block. It cannot identify a specific DJI aircraft model,
establish that a drone is present, or distinguish proprietary DroneID from
ASTM F3411. Randomized/local MACs and radio-module suppliers also limit
the coverage of vendor lookups.

| IEEE MA-L prefix | Registration date |
|---|---|
| 04:A8:5A | 2025-01-09 |
| 0C:9A:E6 | 2025-08-14 |
| 34:D2:62 | 2019-08-13 |
| 48:1C:B9 | 2022-05-07 |
| 4C:43:F6 | 2025-12-01 |
| 58:B8:58 | 2024-07-26 |
| 60:60:1F | 2013-03-11 |
| 88:29:85 | 2025-10-29 |
| 8C:58:23 | 2025-05-27 |
| E4:7A:2C | 2023-10-19 |

Each 24-bit MA-L block nominally contains 2^24 possible MAC addresses.
Registration dates describe vendor allocation history, **not** the dates
those addresses entered any particular aircraft.

## Offline passive lookup

From the repository root:

    python tools/dji_oui_reference.py 34:D2:62:11:22:33

The classification will be DJI_REGISTERED_OUI, but the fields is_drone,
is_remote_id and model are deliberately null. Unknown OUIs do not imply
that a valid drone transmission is absent.

For example, the successful Air 2S synthetic Wi-Fi test used a locally
administered MAC of 02:B0:CB:D8:04:26. The reference tool correctly
reports UNKNOWN_VENDOR for it, because this test transmitter is **not**
authentic DJI hardware.

## Application to SquachWatch

A future passive detector can display a DJI OUI match as **manufacturer
context only**, not as a detection or proof of a DJI model. Require valid
Remote ID message evidence for an ASTM detection; treat proprietary
DJI DroneID as a separately validated protocol. This reference does
not modify SquachWatch's detection logic.

## SquachEmit guardrails

No DJI-allocated OUIs are used by the emitter. Continue to derive unique
locally administered test MACs and clearly fabricated SQE-DJI identifiers.
Do not convert registry assignments into on-air spoofed identities.

## Sources

- MACLookup: https://maclookup.app/vendors/sz-dji-technology-co-ltd
- Udger: https://udger.com/resources/mac-address-vendor-detail?name=sz_dji_technology_co-ltd

Both independent lists matched the 10 entries when checked. Verify any
later additions against current registry data before operational use.
