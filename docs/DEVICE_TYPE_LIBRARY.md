# Device-type test library (CYD 3.2 LAB)

## R14.37 LAB — upstream v1.50.0 plus fork-only signatures (362 fixtures)

This LAB extends the public 172-case catalog by 66 upstream-derived and
10 fork-only synthetic fixtures, and corrects 24 expected labels against the pinned
SquachWatch v1.50.0 source. Its **14** alphabetic device libraries are:
ALPR; AXON; CAMERAS / DASHCAMS; DRONES / REMOTE ID; FLOCK;
GENERAL / CONTROLS; HACKER DEVICES; IBEACON; LISTENERS; RAVEN; RING;
SERIAL MODULES; SMART GLASSES; TRACKERS.

Every case remains in exactly one library. Presence of a vendor-looking
SSID/BLE field/OUI is not authentication of any real device.
R14.36 physical RF verification is **PENDING**. See
[the complete pinned audit](SQUACHWATCH_V150_COVERAGE.md).

---


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
