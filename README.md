# SquachEmit — ESP32-S3 / ESP32-C5 detection test transmitter

**Status: v0.2.0 development. ESP32-S3 baseline retained, Waveshare ESP32-C5 2.8-inch GUI lab target added. Physical RF/hardware verification is required before a production release.**

A **separate** laboratory companion to [retrodroid32/SquachWatch-CYD](https://github.com/retrodroid32/SquachWatch-CYD).
SquachEmit emits bounded, synthetic BLE advertisements and ordinary Wi-Fi
SoftAP beacons that SquachWatch *should* classify. It also provides offline
802.11 frame fixtures for signatures an ESP32-S3 cannot safely or fully emit
on its own. **It does not install anything into SquachWatch.**

The goal is to measure **signature-level** detection, missed hits, wrong
classifications, false positives on negative controls, and latency — without
pretending that a synthetic beacon proves a physical camera/drone is there.

## ESP32-C5 Waveshare touchscreen — experimental board

The `waveshare-esp32c5-touch-28` PlatformIO environment is a **separate**
build target for the Waveshare **ESP32-C5-Touch-LCD-2.8**: ST7789 240x320,
CST3530 capacitive touch, CH32V003 backlight/reset I/O expander. This
**is not the XPT2046 resistive-touch RockBase/ZLX/LCDWiki model.**

Its touchscreen provides scenario selection, PREV/NEXT, RUN, STOP and SUITE.
Boot is radio-silent, and the existing USB serial command interface is kept.
The new target uses Arduino-ESP32 3.3.12 through Arduino CLI. The custom PlatformIO board definition is retained for future use but PlatformIO 8.0.0 cannot currently compile the C5 Arduino core. **It has not been physically tested yet.**

```bash
# Firmware CI uses Arduino CLI and the official ESP32-C5 core v3.3.12.
# See docs/WAVESHARE-C5.md for setup and flashing.
arduino-cli compile --fqbn "esp32:esp32:esp32c5:FlashSize=16M,PSRAM=enabled" \
  --build-property "build.extra_flags=-DSQE_WAVESHARE_C5=1 -DARDUINO_USB_CDC_ON_BOOT=1 -I$PWD/include" \
  --libraries "$PWD/lib" .
```

See [Waveshare C5 setup, pinout, and validation](docs/WAVESHARE-C5.md).
Confirm your actual PCB marking before flashing; the Amazon product page
alone did not provide a verifiable schematic. **Never flash S3 firmware to C5.**

## Hardware

- One **ESP32-S3 development board** with usable onboard 2.4 GHz antenna and USB data cable.
- One SquachWatch CYD 2.8-inch **or** 3.2-inch, with its normal firmware.
- PC with PlatformIO for building/flashing, and serial monitor at **115200**.
- For meaningful controlled accuracy measurements, use a shielded enclosure
  or otherwise RF-isolated, authorized lab. Use the minimum necessary range.

ESP32-S3 has 2.4 GHz Wi-Fi and Bluetooth **LE** only. It cannot transmit real
Bluetooth Classic discovery, 5 GHz Wi-Fi, or arbitrary 2.4 GHz protocols. It
uses one time-shared Wi-Fi/BLE radio, so this firmware runs one case at a time.

## Build / flash

Install [PlatformIO](https://platformio.org/install) (VS Code extension or CLI).
From the project directory:

```bash
pio run -e esp32-s3-devkitc-1
pio run -e esp32-s3-devkitc-1 -t upload
pio device monitor -b 115200
```

To flash without rebuilding in a later phase, use the GitHub CI artifact after
confirming its board target. **CI compilation is not hardware validation.**

For other S3 boards with different flash/PSRAM/pinout settings, create a board
profile before flashing. The current profile assumes the Espressif S3
DevKitC-1 compatible board.

## Radio commands

The device stays silent until explicitly instructed and stops transmitting
after the selected test duration. Send one command per line over 115200 USB
serial:

```text
HELP
LIST
RUN BLE-META-UUID 12
RUN BLE-DRONE 15
RUN WIFI-AXON 12
RUN BLE-NEG-APPLE 12
STOP
SUITE
STATUS
```

`RUN <ID> [seconds]` accepts 3–30 seconds (12 seconds by default). `SUITE`
walks every test for 12 seconds with a 2.4-second quiet gap between cases.
`STOP` terminates both current emission and suite. Device boot does not
transmit. **No unrestricted broadcasting, deauth floods, or attack frames.**

Typical truth log:

```text
TRUTH,{"event":"BEGIN","seq":1,"ms":6025,"id":"BLE-META-UUID","expected":"META","status":"ok"}
INFO,BLE-META-UUID,META,Ray-Ban Meta service UUID
TRUTH,{"event":"END","seq":1,"ms":18029,"id":"BLE-META-UUID","expected":"META","status":"ok"}
```

The `TRUTH` messages describe what SquachEmit attempted to broadcast. They
**do not** certify RF delivery, SquachWatch reception, or a match.

## Coverage — 17 SquachWatch categories

| Detection category | ESP32-S3 on-air coverage | Important limitation |
|---|---|---|
| `FLOCK` | BLE company ID/name, Wi-Fi SSID | Flock OUIs / wildcard-probe corroboration need offline or additional lab transmitter |
| `AXON` | Wi-Fi SSID prefixes | Vendor MAC OUIs not tested on-air |
| `META` | BLE service UUID and manufacturer ID | Checks labels, not an actual camera/glasses |
| `SKIMMER` | BLE SPP-like name | **Not** actual Bluetooth Classic inquiry/connection |
| `RAVEN` | BLE service UUID | Unverified against physical Raven hardware |
| `AIRTAG` | Synthetic Apple Find My payload | Doesn't reproduce full Apple tracker behavior |
| `DRONE` | 3 rotating BLE legacy OpenDroneID messages | No physical UAV; Wi-Fi beacon/NAN are offline fixtures |
| `ALPR` | Offline OUI fixture | Manufacturer-specific radio identity not replicated on-air |
| `CAMERA` | Offline OUI fixture | Manufacturer-specific radio identity not replicated on-air |
| `SAMSUNG_TAG` | BLE service UUID | No real SmartTag protocol/state machine |
| `GOOGLE_TAG` | BLE service UUID | Also matches ordinary Eddystone; ambiguous by design |
| `TILE` | BLE service UUID | No real Tile protocol/state machine |
| `RING` | Offline OUI fixture | Manufacturer-specific radio identity not replicated on-air |
| `DEAUTH` | Offline single-frame + boundary fixtures | **No** over-the-air deauth attacks/floods |
| `EVILTWIN` | Offline contrasting beacons | One S3 SoftAP cannot generate two independent BSSIDs for a complete live test |
| `IBEACON` | Complete synthetic manufacturer data | Enable iBeacon in SquachWatch's detection filters first |
| `HACKER` | BLE UUID/company ID, Wi-Fi SSID; Pwnagotchi IE offline | Not a pentest-radio emulator |

See [docs/ACCURACY.md](docs/ACCURACY.md) for a repeatable acceptance procedure,
expected outcomes, and how to score results. See
[docs/LIMITATIONS.md](docs/LIMITATIONS.md) for a breakdown of what is and isn't proven.

## Host tests and report

Without hardware, validate fixture geometry and catalog consistency:

```bash
python -m unittest discover -s tests -v
python tools/generate_vectors.py
python tools/rid_vectors.py
# With a separate local SquachWatch-CYD clone and g++:
python tools/verify_native.py ../SquachWatch-CYD
```

For real accuracy measurements:

1. Load SquachWatch onto the **2.8-inch** CYD, enable relevant detector types;
   enable iBeacon explicitly for its case.
2. Run `LIST` then one `RUN ...` case at a time on SquachEmit.
3. For each case record the **actual SquachWatch** detected type, `NONE` after
   a full clean test window, or leave blank if not yet tested.
4. Enter observations in `tests/observations.csv` and, if measured, latency in ms.
5. Score:

```bash
python tools/analyze.py tests/observations.csv --output tests/report-2.8.md
```

6. Repeat independently on **3.2-inch** CYD with its own observations CSV and
   report. Each case should have multiple independent runs and periodic negative
   controls; do not equate one successful sighting with a high detection rate.

An optional USB log recorder is available:

```bash
pip install -r requirements-host.txt
python tools/capture.py COM8
```

**Important:** `capture.py` records the SquachEmit truth serial stream only;
to automate matching, add a separate, version-specific parser for the CYD's
serial/SD log format. Current scoring is deliberately driven by actual
observations, not made-up successes.

## Project rules

- Independent repository, release history and versioning; no SquachWatch merges.
- All test profiles marked synthetic and bounded to 30 seconds per case.
- No deauth/jamming, arbitrary raw management-frame injection, credential
  collection, or impersonation of nearby networks.
- Offline fixtures are **not** over-the-air hardware evidence.
- Firmware built by CI is labelled `unverified` until tested on an S3.
- The test device should be used only on equipment and radio environments
  you control/are authorized to test, in compliance with RF regulations.

## Provenance

Compared on 2026-11-08 against SquachWatch-CYD's `include/state.h`,
`src/signatures.cpp`, `src/detection.cpp`, `src/remote_id.cpp`,
`test/emitter_roundtrip_test.cpp`, and `docs/DETECTIONS.md` (default `master`).
The fork's signature table can change; revisit the catalog for new firmware.
This project is independent; it is not affiliated with any of the product
manufacturers named in SquachWatch's detector signatures.
