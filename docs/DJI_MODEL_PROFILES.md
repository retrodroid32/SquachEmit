# SquachEmit — DJI model synthetic Remote ID test coverage

**Original DJI model expansion:** 135 cases (earlier baseline: 95),
including 40 new synthetic on-air model-label cases across 20 named DJI
aircraft (20 BLE4 and 20 Wi-Fi beacon).

## Important: these are NOT authentic DJI model RF captures

The model name is a selection label. Each paired case emits fake `SQE-DJI-*`
Basic IDs and static test aircraft coordinates with standard-shaped
OpenDroneID messages. They do **not** identify themselves as DJI hardware,
clone a genuine serial, emulate OcuSync, or transmit the distinct DJI
proprietary DroneID protocol. A valid receiver may say **DRONE**, but
correctness for a specific aircraft model remains unverified.

Current Wi-Fi tests use ordinary **2.4 GHz SoftAP beacons on channel 6**.
Their vendor information element now carries a **five-message synthetic
OpenDroneID pack** (Basic ID, Location, System, Self-ID, and Operator ID)
in a 135-byte vendor IE. The message pack format follows the
[OpenDroneID Core C Wi-Fi beacon reference](https://github.com/opendroneid/opendroneid-core-c/blob/master/libopendroneid/wifi.c).
All identities and coordinates are fabricated. The emitter has no verified
GNSS or trusted UTC time, and these tests are **not operationally compliant
ASTM F3411-22a aircraft broadcasts**. The Wi-Fi message pack's counter
is fixed during each bounded test; cadence is controlled by the SoftAP's
ordinary beacon interval.

The existing BLE4 tests rotate Basic ID/Location/System/Self-ID test
messages. This board's BLE4 test is **not** Bluetooth 5 coded/extended
advertising and none of the cases reproduce proprietary DJI DroneID.

## Model-to-case mapping

| DJI model label | Synthetic BLE4 | Synthetic Wi-Fi Beacon |
|---|---|---|
| DJI Air 2S | `RID-DJI-AIR2S` | `RID-WIFI-DJI-AIR2S` |
| DJI Air 3 | `RID-DJI-AIR3` | `RID-WIFI-DJI-AIR3` |
| DJI Avata | `RID-DJI-AVATA` | `RID-WIFI-DJI-AVATA` |
| DJI FPV | `RID-DJI-FPV` | `RID-WIFI-DJI-FPV` |
| DJI Matrice 300 RTK | `RID-DJI-MATRICE300RTK` | `RID-WIFI-DJI-MATRICE300RTK` |
| DJI Matrice 30 | `RID-DJI-MATRICE30` | `RID-WIFI-DJI-MATRICE30` |
| DJI Matrice 350 RTK | `RID-DJI-MATRICE350RTK` | `RID-WIFI-DJI-MATRICE350RTK` |
| DJI Mavic 2 Enterprise Advanced | `RID-DJI-MAVIC2EA` | `RID-WIFI-DJI-MAVIC2EA` |
| DJI Mavic 2 Pro | `RID-DJI-MAVIC2PRO` | `RID-WIFI-DJI-MAVIC2PRO` |
| DJI Mavic 2 Zoom | `RID-DJI-MAVIC2ZOOM` | `RID-WIFI-DJI-MAVIC2ZOOM` |
| DJI Mavic 3 | `RID-DJI-MAVIC3` | `RID-WIFI-DJI-MAVIC3` |
| DJI Mavic 3 Classic | `RID-DJI-MAVIC3CLASSIC` | `RID-WIFI-DJI-MAVIC3CLASSIC` |
| DJI Mavic 3 Enterprise | `RID-DJI-MAVIC3E` | `RID-WIFI-DJI-MAVIC3E` |
| DJI Mavic 3 Pro | `RID-DJI-MAVIC3PRO` | `RID-WIFI-DJI-MAVIC3PRO` |
| DJI Mavic 3 Thermal | `RID-DJI-MAVIC3T` | `RID-WIFI-DJI-MAVIC3T` |
| DJI Mini 3 | `RID-DJI-MINI3` | `RID-WIFI-DJI-MINI3` |
| DJI Mini 3 Pro | `RID-DJI-MINI3PRO` | `RID-WIFI-DJI-MINI3PRO` |
| DJI Mini 4 Pro | `RID-DJI-MINI4PRO` | `RID-WIFI-DJI-MINI4PRO` |
| DJI Phantom 4 Pro V2.0 | `RID-DJI-P4PROV2` | `RID-WIFI-DJI-P4PROV2` |
| DJI Phantom 4 RTK | `RID-DJI-P4RTK` | `RID-WIFI-DJI-P4RTK` |

The five preexisting generic DJI family cases remain for backwards
compatibility. They also use synthetic message contents and are not model
fingerprints.

## Not modeled as native built-in DJI broadcast RID

An original Phantom 4 is **not** a Phantom 4 Pro V2.0, and a Mavic Pro is
**not** a Mavic 2 Pro. The Mavic 2 Enterprise Zoom/Dual are distinct from
the supported consumer Mavic 2 Pro/Zoom. For legacy aircraft that require a
retrofit broadcast module, test the **module's** observed Remote ID profile
separately rather than attributing it to aircraft-native DJI firmware.

DJI's support and firmware requirements are published at:
https://support.dji.com/help/content?customId=en-us03400007747&lang=en&re=US&spaceId=34

## Next: evidence-based realistic profiles

Collect authorized **passive** captures of the operator's own aircraft,
along with exact firmware version and transport, then compare structure,
update cadence, message fields and available receiver indications. Scrub
identifiers and precise locations before storing public fixtures. Build
additional offline golden vectors from OpenDroneID Core C and compare
independent decoder output with SquachWatch.

Do not claim actual DJI model transmissions from generic ASTM messages,
or RF features unsupported by this ESP32, until separately hardware tested.
This document records the current merged test catalog; further firmware or release changes require separate approval.

## Vendor MACs are not model identification

See [DJI OUI Reference](DJI_OUI_REFERENCE.md) for 10 source-verified
MA-L manufacturer blocks. A matching MAC is only a vendor-registry clue;
model selection in SquachEmit remains a synthetic test label. The emitter
continues using locally administered MACs. One Air 2S Wi-Fi test was
reported independently received by SquachWatch, but other models and
complete ASTM conformance still require validation.

## Separate manufacturer-prefix Wi-Fi LAB

Merged main includes ten DJI manufacturer-OUI test beacons using
DJI registered MA-L prefixes. These are deliberately **not Remote ID**;
they test only SquachWatch's manufacturer-prefix detection heuristic.
The established forty model-labeled Remote ID cases are unchanged.
See [DJI OUI Reference](DJI_OUI_REFERENCE.md) for prefixes and caveats.

## Extended UAS vendor allocations — separate 172-case LAB

A separate unmerged CYD 3.2 LAB adds 27 precisely verified prefixes for
Autel, Parrot, Skydio, DJI Baiwang, Quantum-Systems, Insitu, Yuneec,
FIMI, Hubsan, XAG, and avionics/payload suppliers. These emit only ordinary
Wi-Fi beacons, never verified aircraft Remote ID. See
[Extended UAV Vendor OUI LAB](UAV_VENDOR_OUI_LAB.md) for exact 24/28/36-bit
allocations and rejected misattributions.

## Full DJI OUI receiver-decoder fixture matrix (offline)

The accompanying `tests/dji_oui_registry.json` records all ten known
SZ DJI TECHNOLOGY CO.,LTD MA-L /24 manufacturer prefixes with source
registration dates. `tools/generate_vectors.py` produces ten additional
**offline-only** 802.11 beacon frames from this snapshot. Those frames contain
the registered prefixes in the transmitter and BSSID bytes and allow the
SquachWatch receiver decoder to be checked using
`python tools/verify_native.py ../SquachWatch-CYD`.

This feature **does not introduce additional transmitted test scenarios**.
Mainline already includes ten separate `OUI-DJI-...` *on-air test labels*
among its 172 catalog entries. Their ordinary Wi-Fi test beacons use
board/case-unique **locally administered** MAC identities and synthetic
`SQE-LAB-DJI-...` SSIDs, **not** the registered DJI MAC bytes from the
offline fixtures. Those two sets deliberately use related test IDs, but
exercise different mechanisms.

A matched vendor OUI supports only **manufacturer attribution**. It does not
establish a flying aircraft, genuine DJI Remote ID, an exact model, DJI's
proprietary protocol, or authenticity. No drone-specific radio hardware,
RF profile, scenario count, installer behavior or release is altered by these
offline additions.
