# Remote ID reference review — 2026-11-10

Target: synthetic SquachEmit signal tests for SquachWatch's detector.
The purpose is to measure parser accuracy without cloning a third-party
aircraft's real identity.

## References and suitability

| Source | Role | Recommended use |
|---|---|---|
| [ASTM F3411-22a](https://www.astm.org/f3411-22a.html) | Protocol baseline | Test transport envelope, Basic ID, Location, System, Self-ID, update rates |
| [OpenDroneID Core C](https://github.com/opendroneid/opendroneid-core-c) (Apache-2.0) | Standard encoder/decoder | Build independent **offline** golden message vectors; evaluate carefully before linking into GPL firmware |
| [ArduRemoteID](https://github.com/ArduPilot/ArduRemoteID) (GPL-2.0) | Hardware transmitter reference | Study C3/S3 Bluetooth Legacy, BT5 long-range, Wi-Fi beacon and NAN code paths; do not assume all modes work on classic CYD |
| [OpenDroneID transmitter-linux](https://github.com/opendroneid/transmitter-linux) (Apache-2.0) | Transmitter example | Check Wi-Fi message-pack construction and update cadence offline |
| [cyber-defence-campus/droneRemoteIDSpoofer](https://github.com/cyber-defence-campus/droneRemoteIDSpoofer) (MIT) | Security research emitter | Explore parser false positives and crafted negatives only in authorized settings; do not port arbitrary aircraft impersonation |
| [alphafox02/droneid-go](https://github.com/alphafox02/droneid-go) (binary, non-open-source license) | Independent receiver | Decode Wi-Fi/BLE via suitable Linux/radio receiver and PCAPs; do not vendor proprietary binary into firmware |
| [RUB-SysSec/DroneSecurity](https://github.com/RUB-SysSec/DroneSecurity) (AGPL-3.0) | DJI proprietary DroneID decoder | **Receive/analyze** DJI proprietary broadcasts separately; not interchangeable with ASTM Remote ID |
| [OP-drone](https://github.com/OP-drone) | Aggregator/forks | Track upstream OpenDroneID transmitter/receiver implementations rather than fork drift |
| [remote-id GitHub topic](https://github.com/topics/remote-id) | Discovery | Find message and receiver test fixtures, confirm licensing first |
| [CIAJeepDoors](https://github.com/MAVProxyUser/CIAJeepDoors) | DJI broadcasts suppression proof-of-concept | Historical context only; **no suppression, disabling or cloned identity features** |

## ASTM transport matrix

ASTM F3411-22a describes BLE 4 legacy, BLE 5 long range/extended,
Wi-Fi Neighbor Awareness Network (NAN) and Wi-Fi beacons. OpenDroneID
Core C discusses regional requirements, message packs and transmission
cadence. **Transport support and regulatory compliance are not the same.**

| Mode | Classic CYD 3.2 SquachEmit currently emits | Next action |
|---|---|---|
| BLE4 Legacy service-data | Yes; synthetic message rotation | Validate message bytes against OpenDroneID golden vectors |
| Wi-Fi 2.4 GHz beacon vendor IE | Yes; one synthetic Basic ID per test | Add full multi-message pack and cadence after offline conformance tests |
| BLE5 long-range / coded | No | Mark offline/other hardware; avoid fake pass claims |
| Wi-Fi NAN | No | Offline frame fixtures or a verified hardware implementation |
| 5 GHz beacon | No on classic CYD | Verify capability of different hardware before offering on-air mode |
| Proprietary DJI DroneID / OcuSync | No | Passive capture/receiver comparison only; not a model-specific fake |

## Acceptance criteria

- No transmitter activity at boot; explicit user action starts bounded tests
- Synthetic `SQE-` identifiers, no copied genuine aircraft serials/locations
- Negative vectors with wrong application ID, truncated payload, invalid ID type
- Wi-Fi beacon field layout matches golden vectors; full packs eventually
  include required messages, not Basic ID alone
- 2.8/3.2 SquachWatch receiver detection logged separately from emitter CI
- Model names in UI labeled as a simulation, never a real-aircraft fingerprint
- Every case appears exactly once in alphabetical Drones / Remote ID library
- No merging until user approves and hardware results are reviewed

Sources:
- https://github.com/opendroneid/opendroneid-core-c
- https://github.com/ArduPilot/ArduRemoteID
- https://github.com/opendroneid/transmitter-linux
- https://github.com/alphafox02/droneid-go
- https://github.com/cyber-defence-campus/droneRemoteIDSpoofer
- https://github.com/RUB-SysSec/DroneSecurity
