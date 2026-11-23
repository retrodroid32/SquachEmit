# SquachEmit R14.35 LAB — SquachWatch upstream v1.50 detection coverage

**Baseline:** `skizzophrenic/SquachWatch-CYD` **v1.50.0** (`src/signatures.cpp` Git blob
`d359f580048e4f8ea059dea9c95fdd53ee5afec7`).
**Emitter target:** `retrodroid32/SquachEmit`, based on **main** after merged
battery PR #109. SquachWatch is READ-ONLY during this work.

## Scope and trust boundary

SquachEmit's synthetic legacy BLE advertisements and standard Wi-Fi SoftAP
beacons can check a receiver's matching behavior, but **cannot identify
real commercial devices**. A vendor-looking BSSID, SSID, name, UUID or
company ID is easy to spoof. None of these fixtures is an authenticated
drone, camera, police device, tracker, smart speaker or genuine Remote ID
broadcast. All transmit cases are user-triggered and time-limited.

No real receiver results have been supplied for R14.35, so *every* new
physical observation remains **NOT TESTED**. The new source-pinned matrix
and C++ regression prove expected data consistency, not physical on-air
reception. Negative controls are important and must NOT produce the
positive label on a v1.50 receiver.

## Detailed upstream signature audit

The upstream source contains 220 explicit lookup table entries:

| Rule family | Upstream entries | Entries with candidate emitter fixtures |
|---|---:|---:|
| Wi-Fi manufacturer OUI | 151 | 40 |
| BLE service UUID | 26 | 22 |
| BLE manufacturer ID | 9 | 9 |
| Wi-Fi SSID prefix | 27 | 27 |
| Bluetooth classic / name string | 7 | 7 |
| **Total** | **220** | **105** |

The machine-readable exact per-rule mapping is
`tests/squachwatch_v150_manifest.json`. Candidate mapping is not proof
the receiver decoded the correct vendor; some rows correspond only to
the same broad detection category. All 115 remaining signature-table
entries require dedicated coverage, another mode, or explicit unsupported
status. The count excludes parser-specific patterns that have their
own tests below.

## R14.35 changes

- Preserves the 172 prior case IDs and adds **66** bounded new synthetic
  cases: **238 total**, below the 254-case per-radio-address identity limit.
- Corrects 24 old expected-label discrepancies against *this exact*
  upstream tag: 23 old positives must now be `NONE`, while
  `BLE-NEG-PENGUIN-9` is classified `FLOCK` by upstream's broad
  Penguin substring check, even though it was originally a negative
  fixture. That mismatch is documented, not silently hidden.
- Adds 22 source-matched Wi-Fi vendor-prefix fixtures for ALPR, cameras
  and dashcams, Ring, Sonos, smart glasses, Flock, Axon.
- Adds 24 SSID cases for dashcams, setup cameras, GoogleHome, Flock and
  one negative generic camera prefix.
- Adds 9 BLE service UUID, 4 BLE manufacturer, and 7 special BLE cases:
  AirTag nearby/unpaired, AirPods negative, HomePod Siri, two Find Hub
  FMDN payload variants and BWCDEVICE.
- Creates 14 alphabetically ordered on-device test categories. New
  ALPR, CAMERA, RING and LISTENER categories use neutral generic
  Squachy speech until dedicated text is validated.
- Existing DJI Remote ID and vendor-only OUI cases remain in place.
  Manufacturer-prefix-only beacons are **NOT** authentic aircraft Remote ID.

### New positive detection categories

SquachWatch has 20 non-UNKNOWN detection types in v1.50. The expanded
R14.35 catalog contains at least one *candidate positive fixture* for
**16**: FLOCK, AXON, META, SKIMMER, RAVEN, AIRTAG, DRONE, ALPR, CAMERA,
SAMSUNG_TAG, GOOGLE_TAG, TILE, RING, IBEACON, HACKER and LISTENER.

The four without safe direct positive on-air fixtures remain:

| SquachWatch type | Why it is not validated here |
|---|---|
| DEAUTH | Requires disassociation/deauthentication frame bursts; emitter sends none |
| EVILTWIN | Requires coordinated observations from distinct BSSIDs/security modes, not a single standard AP |
| LORAWAN | Requires a compatible LoRa transceiver plus suitable profile and RF testing |
| MINE | User-configured SSID pattern, no portable universal signal to match |

### Behavior not proven by a signature

- New Trail card FOLLOWING / MET BEFORE verdicts require the detector to
  experience separated stops over time and multiple observations.
- AirTag and Find Hub owner states need carefully controlled lifecycle
  transitions; a single frame tests a recognizer, not a complete
  tracking or safety determination.
- Pwnagotchi custom 802.11 vendor IE, Wi-Fi NAN Drone ID, optional
  extended/coded BLE PHY, Bluetooth Classic inquiry and certain LoRa
  formats require different radio capabilities or separate offline
  captures. They are **not** represented by the existing SoftAP and
  legacy BLE advertisement transport.
- Non-detected prefixes for Autel, Insitu, Quantum, XAG, Hubsan, Yuneec
  and FIMI were retained as negative fixtures; upstream has no matching
  DRONE OUI rows for them. They may be valid manufacturer allocations
  but must not be reported as current v1.50 DRONE detections.

## Physical test gate (CYD 3.2 emitter to SquachWatch receiver)

1. Flash the **R14.35 LAB** target, not the production/stable build.
2. On SquachWatch v1.50.0 (or an explicitly source-verified compatible
   fork), enable the specific detection filters: notably `LISTENER`
   and `IBEACON` are normally off.
3. With devices in a permitted test environment, exercise every new
   positive and negative case. Compare emitted `TRUTH` ID and
   expected category with the receiver's actual LOG/Event result.
4. Save observed result, latency, MAC, channel and firmware hashes in
   `tests/observations.csv` or structured Workbench trials.
5. Confirm STOP, duration expiry, per-case MAC uniqueness, and that
   the test suite can be interrupted. Do not interpret source-test PASS
   as radio hardware PASS.

No SquachWatch firmware was modified. No PR is to be merged without the
owner's explicit approval.

Source:
https://github.com/skizzophrenic/SquachWatch-CYD/tree/v1.50.0

## R14.36 fork-only signature increment

SquachEmit now adds **10 individually selectable, bounded Wi-Fi AP OUI fixtures** unique to `retrodroid32/SquachWatch-CYD` default branch `src/signatures.cpp` blob `1f63a08ce3eba4148d557312bb0d1abe8b18545c`: FLOCK (3C71BF, 4827EA, A4CF12), DRONE (B030C8, EC715E, 001AF9, 547D40, 74B80F), SKIMMER (98D300), and HACKER (0CFA22). This brings the total to **248 synthetic cases** (172 original + 66 upstream v1.50 + 10 fork-only). Each OUI uses a per-case temporary AP MAC prefix and synthetic suffix; all are unverified against physical receivers. The pinned 105/220 upstream mapping is unchanged. Cases are not real devices, and a MAC-prefix match is not authenticated identity. Additional upstream and fork detection behaviors remain out of coverage, and the 254-case address scheme must be redesigned before expanding beyond its bound.
