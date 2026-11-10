# SquachEmit — DJI model Remote ID coverage (draft LAB)

**v1.0.0 publicly released baseline:** 95 cases. **This draft LAB: 135 cases**,
including 40 new synthetic on-air model-label cases across 20 named DJI
aircraft (20 BLE4 and 20 Wi-Fi beacon).

## Important: these are NOT authentic DJI model RF captures

The model name is a selection label. Each paired case emits fake `SQE-DJI-*`
Basic IDs and static test aircraft coordinates with standard-shaped
OpenDroneID messages. They do **not** identify themselves as DJI hardware,
clone a genuine serial, emulate OcuSync, or transmit the distinct DJI
proprietary DroneID protocol. A valid receiver may say **DRONE**, but
correctness for a specific aircraft model remains unverified.

Current Wi-Fi tests use ordinary **2.4 GHz SoftAP beacons on channel 6**,
with an OpenDroneID vendor information element that carries **one
synthetic Basic ID**. This is a *receiver smoke test*, not a complete
operational ASTM F3411-22a message pack: full broadcast compliance requires
the required Location, System and timing behavior. The existing BLE4 cases
rotate Basic ID/Location/System/Self-ID test messages. This board's
experimental BLE4 test is **not** Bluetooth 5 coded/extended advertising.

## Model-to-case mapping

| DJI model label | Synthetic BLE4 | Synthetic Wi-Fi Beacon |
|---|---|---|
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |
| undefined | `RID-DJI-undefined` | `RID-WIFI-DJI-undefined` |

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
No firmware release, merge or tag is authorized by this document.
