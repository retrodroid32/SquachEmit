# DJI MAC vendor prefixes and limited on-air OUI test LAB

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

## SquachEmit 10-case OUI-only transmit LAB

A separate **draft LAB branch** adds ten opt-in Wi-Fi test beacons that
use one of these MA-L prefixes in the transmitted BSSID. **This is
intentional MAC-vendor emulation for receiver testing**, not a genuine
DJI radio or an ASTM Remote ID message.

The full BSSID uses the chosen registered 3-byte prefix plus a generated
3-byte suffix. It is not derived from an actual DJI serial or airframe.
The suffix is unique per scenario on a given test board, but we cannot
guarantee that it is unallocated within DJI's global block.

Each test uses an ordinary 2.4 GHz WPA2 SoftAP on channel 6 with an
unambiguous SQE-LAB-DJI-... SSID. **No OpenDroneID vendor IE**, DJI
proprietary DroneID or OcuSync is sent. Run duration is bounded to 3–30
seconds; nothing transmits until RUN is pressed.

| Case ID | Wi-Fi MAC prefix | Beacon SSID |
|---|---|---|
| OUI-DJI-04A85A | 04:A8:5A | SQE-LAB-DJI-04A85A |
| OUI-DJI-0C9AE6 | 0C:9A:E6 | SQE-LAB-DJI-0C9AE6 |
| OUI-DJI-34D262 | 34:D2:62 | SQE-LAB-DJI-34D262 |
| OUI-DJI-481CB9 | 48:1C:B9 | SQE-LAB-DJI-481CB9 |
| OUI-DJI-4C43F6 | 4C:43:F6 | SQE-LAB-DJI-4C43F6 |
| OUI-DJI-58B858 | 58:B8:58 | SQE-LAB-DJI-58B858 |
| OUI-DJI-60601F | 60:60:1F | SQE-LAB-DJI-60601F |
| OUI-DJI-882985 | 88:29:85 | SQE-LAB-DJI-882985 |
| OUI-DJI-8C5823 | 8C:58:23 | SQE-LAB-DJI-8C5823 |
| OUI-DJI-E47A2C | E4:7A:2C | SQE-LAB-DJI-E47A2C |

Because SquachWatch's present OUI rules classify manufacturer-prefix
matches as DRONE, the expected legacy alert for these cases is DRONE.
**The correct interpretation is vendor-OUI heuristic detection only.**
This cannot be scored as successful ASTM Remote ID decoding.

See [DJI OUI transmitter LAB](DJI_OUI_LAB_TRANSMIT.md) for the
flash/serial/receiver hardware verification matrix. Test only in an
authorized controlled environment, since other Wi-Fi scanners could
misinterpret the imitated vendor prefix. All non-OUI SquachEmit test
cases retain their locally administered MACs.

## Sources

- MACLookup: https://maclookup.app/vendors/sz-dji-technology-co-ltd
- Udger: https://udger.com/resources/mac-address-vendor-detail?name=sz_dji_technology_co-ltd

Both independent lists matched the 10 entries when checked. Verify any
later additions against current registry data before operational use.
