# SquachEmit browser installer and RF LAB console

## URL

When GitHub Pages has been enabled in **Settings → Pages → Source: GitHub Actions**:

- Production: `https://retrodroid32.github.io/SquachEmit/` (empty until real hardware approval)
- Unverified lab: `https://retrodroid32.github.io/SquachEmit/?lab=1`

The lab channel shows up to three recent successful C5 lab builds and the latest
successful S3 baseline with SHA/CI provenance. Firmware binaries and manifests
are copied into the Pages artifact (same origin, without GitHub API CORS/login).
The `esp-web-tools` installer recognizes ESP32-C5 and ESP32-S3 and refuses a
mismatched chip family, but it **cannot** distinguish different display/touch
boards built around the same ESP32-C5. A mandatory board confirmation is shown.

The in-browser serial console sends `HELP`, `LIST`, `RUN`, `STATUS`, `STOP`,
`SUITE` over Web Serial at 115200 and supports exporting the actual truth log.
Disconnect the console before flashing and reconnect afterward.

## How CI publishes LAB builds

`SquachEmit CI` keeps the S3 and C5 build jobs independent. The S3 job uploads
bootloader, partition table, OTA-selection bytes and application; the C5 job
already produces a complete Arduino-CLI package on its separate unmerged lab
branch. `.github/workflows/publish-installer.yml` runs on successful CI
completion, main-site changes, or manual dispatch. It reads only successful
run artifacts from this repository, for the baseline `main` S3 and allowed
`lab/**` branch C5 profiles. Downloaded files are **data**, never executed.

`tools/build_installer_site.py` validates the chip header, partition table,
relevant offsets (C5 bootloader is 0x2000), OTA data and application content,
then emits per-build ESP Web Tools manifests and SHA256 checksums. The default
site has no fake stable release: production becomes available only with a
separate hardware-approved release pipeline in future work.

The site workflow deploys from the default `main` branch; **it does not merge
or release any C5 lab branch**, and changes nothing in SquachWatch-CYD.

## Hardware validation

1. Confirm Waveshare ST7789 / CST3530 / CH32V003 board identity.
2. Confirm normal USB data cable, Chrome/Edge Web Serial, the correct C5/S3
   chip family shown by the installer, and take a backup before initial flash.
3. Flash one LAB build. Check boot, screen, touch, RUN/STOP and output timeout.
4. Connect the terminal, send `LIST` and a single `RUN BLE-META-UUID 12`.
5. Observe actual detections on SquachWatch 2.8 and 3.2 separately, including
   `WIFI-AXON`, `BLE-NEG-UUID`, and the suite. Record false positives/negatives.
6. Do not merge the C5 LAB PR or label it hardware verified without explicit
   approval after these physical tests.

## Publishing prerequisite

If GitHub Pages is not enabled, `actions/deploy-pages` cannot publish.
In the repo, open **Settings → Pages → Build and deployment → Source:
GitHub Actions**. This is a one-time repository-owner setting; it cannot be
performed by the GitHub file-edit actions in this assistant environment.

Use HTTPS; Web Serial cannot run from arbitrary local files (`file://`).

## Descriptive firmware selection labels

Firmware dropdown options show the change category, LAB version and short
commit SHA instead of only a date and hash. The selected-build panel explains
the exact additions, limitations, board, complete SHA, build timestamp and
application SHA256. Existing chip selection and mandatory hardware confirmation
are unchanged.

Notes are intentionally pinned to **full 40-character source commit SHAs** in
`tools/firmware_build_notes.json`. When a new build is published, add a
reviewed entry for that exact SHA before giving it feature-specific claims.
An unknown SHA receives an explicit **review exact changes** warning, even
if it comes from an older LAB branch; it never silently inherits the features
of a nearby or newer commit. These notes describe source changes, NOT
physical hardware verification.

Historical C5 builds are distinguished:
- fb57b628: v0.4.0 synthetic DJI/Autel/Parrot Remote ID profiles, 49 cases.
- 6c08d0de: v0.3.0 same 31 RF cases as e5cf01bf, plus S3 artifact packaging.
- e5cf01bf: initial v0.3.0 31-case Wi-Fi channel/negative test matrix.
- 2d6e9f3f: original C5 touch-display baseline, 22 cases.
- ee7295ac: original S3 baseline, 22 cases.

No firmware files, flashing manifests or LAB branch merges are changed by
this installer-label-only improvement.

## CYD 3.2-inch ST7798 board option (unmerged)

The LAB installer adds a distinct target for the owner's ESP32-2432S032R
3.2-inch ST7798-compatible resistive-touch CYD, separate from Waveshare
ESP32-C5 2.8-inch and ESP32-S3. Chip family MUST be classic ESP32;
its bootloader is at 0x1000 (S3 is 0x0; C5 is 0x2000).
The app starts at 0x10000 and OTA selection uses 0xe000.

The firmware has a 49-case touchscreen menu and first-boot touch calibration.
Flashing SquachEmit onto a 3.2-inch CYD overwrites SquachWatch there;
another physical SquachWatch is needed to assess actual detections.
No production releases and no device auto-flash. Requires physical check.
