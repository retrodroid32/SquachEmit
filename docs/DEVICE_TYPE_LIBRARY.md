# Device-type test library (CYD 3.2 LAB)

The test library is grouped by **device type**, not radio bearer.
Categories, in alphabetical order:

- AXON
- DRONES / REMOTE ID
- FLOCK
- GENERAL / CONTROLS
- HACKER DEVICES
- IBEACON
- RAVEN
- SERIAL MODULES
- SMART GLASSES
- TRACKERS

Select TESTS on the main screen, then page through the device categories
using PREV and NEXT. A case appears in exactly one category. Within each
category, cases are sorted alphabetically by stable case ID. BLE and Wi-Fi
Remote ID all live in DRONES; BLE tracker cases never appear under a
generic BLE radio category. Negative-control tests are grouped with their
target family, while genuinely generic controls are in GENERAL / CONTROLS.

BLE versus Wi-Fi remains visible in serial truth and case metadata, and is
not a top-level library category. The proposed 172-case LAB extends the 145-case merged main library with
27 additional manufacturer OUI-only Wi-Fi fixtures. All tests retain
user-triggered bounded transmissions and appear in exactly one device type.
These vendor-prefix-only beacons are **not** ASTM Remote ID, even though
they live under DRONES / REMOTE ID for test-library convenience.
No SquachWatch receiver code is altered. Hardware GUI verification required.
