# SquachEmit v1.1.0 — 172-Case Detection Suite & Smooth Touch

**Public firmware release · November 11, 2026**

SquachEmit is a standalone ESP32 transmitter for **controlled SquachWatch detection testing**, not the SquachWatch receiver firmware. This is the next public release after v1.0.0 and incorporates the changes merged through SquachEmit PRs #32, #3, #26, #33 and #31.

## Version

- **Firmware:** `v1.1.0` (serial BOOT banner on all three supported targets)
- **Test catalog:** 172 scenarios (up from 95 in v1.0.0)
- **Target hardware:** CYD 3.2 ST7798 / ESP32-2432S032R, Waveshare ESP32-C5 Touch LCD 2.8, ESP32-S3 DevKitC-1
- **Installer:** [Regular public installer](https://retrodroid32.github.io/SquachEmit/) with four-part SHA-256 firmware validation

## What Changed since v1.0.0

1. **172 synthetic BLE/Wi-Fi test cases.** Expanded the device-type catalog from 95 to 172 opt-in, time-limited scenarios. New drone model labels, controls and manufacturer-prefix test cases improve the SquachWatch classification test matrix; an advertised model or vendor prefix does **not** prove a real drone is present.
2. **More UAV vendor-prefix tests.** Added 27 UAS manufacturer-prefix Wi-Fi beacon scenarios, including registered DJI, Autel, Parrot and other UAS-related allocations. These are intentionally generated test signals, not authentic vendor aircraft or Remote ID attestations.
3. **CYD 3.2 transmit-time editor.** Home screen `TX TIME [SET]` lets you save a **3–30 second** run length, adjust by ±1 second and reset to 12 seconds. The setting persists after a reboot and applies to touchscreen RUN/SUITE and serial RUN without explicit seconds.
4. **Less touchscreen flashing.** CYD 3.2 ST7798 touch and duration changes update only affected screen regions instead of repeatedly clearing the display. Squachy animation, STOP and auto-timeout remain.
5. **Safer browser installation.** The web installer verifies chip family, offsets, byte sizes, and SHA-256 of the bootloader, partitions, OTA selection and application before enabling installation/download. Select the **exact board**; this is an integrity check, not cryptographic publisher authentication.
6. **Clear firmware naming and changelog.** The installer now displays a descriptive build name, actual version and meaningful **What Changed** for the matching CI source; modern 172-case builds are not mislabeled as the older v1.0.0 95-case firmware.
7. **Updated README animations.** Four animated UI demonstrations captured from the real CYD 3.2 C++ renderer include the library, Squachy running test and new TX-time control.
8. **Ten DJI offline decoder checks.** Added registered-prefix Wi-Fi test frames and native verification against the real SquachWatch-CYD decoder. The registered-prefix vectors are *offline* tests; they do not change on-air transmitter behavior.

## Downloads

The release includes **three board-specific ZIP archives**; do not flash another board's image. Each archive includes firmware files, a per-file `SHA256SUMS.txt` and `RELEASE-INFO.txt`. A separate `SquachEmit-v1.1.0-SHA256SUMS.txt` checks the ZIP downloads.

| Archive | Target |
| --- | --- |
| `SquachEmit-v1.1.0-172Cases-CYD32-ST7798.zip` | CYD 3.2 ST7798, ESP32-2432S032R, resistive touch |
| `SquachEmit-v1.1.0-172Cases-WAVESHARE-C5-28.zip` | Exact Waveshare ESP32-C5 Touch LCD 2.8 target |
| `SquachEmit-v1.1.0-172Cases-ESP32-S3.zip` | ESP32-S3 DevKitC-1 serial-console version |

**Recommended:** [Install from the browser](https://retrodroid32.github.io/SquachEmit/) using desktop Chrome/Edge and a USB data cable. Choose the matching target and wait for `INTEGRITY: PASS`.

## Verification status and limitations

- GitHub CI runs host tests, verified native SquachWatch DJI OUI decoder tests, and builds all **three** firmware targets independently.
- The owner has reported successful **CYD 3.2** flashing and testing of its TX-time and touchscreen behavior in the development sequence. This does not establish hardware certification.
- **Waveshare C5 and ESP32-S3:** CI compilation is not a claim of physical-device testing.
- Valid signatures can still be spoofed. OUI-only identification is not evidence of aircraft flight, exact model or trusted Remote ID. Some advertised RF formats require hardware not present on ESP32 and are represented by offline fixtures only.
- No automatic transmission at boot; all live tests require explicit user action and enforce a bounded duration. Use equipment and spectrum only in an authorized test environment.
- Installing SquachEmit on a CYD **replaces SquachWatch** on that device; use a separate SquachWatch receiver to measure detection.

The original `v1.0.0` tag and release assets remain intact for comparison. The new `v1.1.0` tag is created only by the successful main-branch CI publishing workflow.
