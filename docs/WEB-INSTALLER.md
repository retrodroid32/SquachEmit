# SquachEmit browser installer and RF LAB console

## URL

When GitHub Pages has been enabled in **Settings → Pages → Source: GitHub Actions**:

- Production: `https://retrodroid32.github.io/SquachEmit/` (empty until real hardware approval)
- Unverified lab: `https://retrodroid32.github.io/SquachEmit/?lab=1`

The lab channel shows only the newest successful firmware per board, including
CYD 3.2 ST7798, C5 touchscreen and S3 with SHA/CI provenance. Firmware binaries and manifests
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

## Automatic CYD 3.2 LAB artifact inclusion

When the new firmware CI on lab/cyd32-st7798-emitter-* completes
successfully, the main Pages publisher selects the dedicated
squachemit-cyd32-st7798-firmware-unverified artifact, not a C5 or S3
binary. The publisher requires bootloader, partitions, application and
OTA-init segments and feeds them to the chipFamily ESP32 manifest with
bootloader offset 0x1000.

The published labels for the original C5 and S3 firmware remain
unchanged; the 3.2-inch firmware must have its own SHA-based notes and
appears only in the corresponding ESP32 board picker.

## Serial test console baud-rate selector

The console exposes a **SERIAL BAUD RATE** selector with preset speeds
9,600 / 57,600 / 115,200 / 230,400 / 460,800 / 921,600 / 1,000,000 /
2,000,000 / 3,000,000 and a custom integer rate (300 through 4,000,000).
Its default is 115,200 for all SquachEmit firmware targets (ESP32-C5,
ESP32-S3, and the 3.2-inch classic ESP32). The established SquachWatch CYD
serial diagnostic console may run at 2,000,000. These are firmware baud
settings, not determined automatically by a board dropdown.

The browser passes the selected rate to Web Serial's
`port.open({baudRate})`. The rate input is locked while a port is
connecting or connected and becomes editable after disconnect. Invalid
custom values are rejected before the OS port picker opens, and an
unsupported baud value produces a visible connection error. The console
reports its connected speed and includes it in the exported raw truth log.

The firmware installer is unaffected; this change does not modify
ESP Web Tools manifests, firmware, or on-air behavior.


## Latest-only firmware picker

For each target board, the LAB installer displays one firmware option: the
newest successfully completed CI artifact. Historical binaries remain in
GitHub Actions for debugging, but are not listed for installation. The CYD 3.2
publisher now recognizes all `lab/cyd32-*` branches, including the source-port
branch, and uses an exact-SHA description for `caa89d82`. This does not merge
the firmware PR or approve production distribution.

## All-segment firmware preflight (PR #3)

The installer now performs a **fail-closed checksum preflight** before enabling
flashing *or* the application-binary download. Its successful-CI catalog
contains explicit SHA-256 hashes, exact byte counts, segment names, paths and
offsets for the bootloader, partitions, OTA data and application. On each
build selection, the browser fetches the published ESP Web Tools manifest
and verifies the chip family and four exact offsets (C5 bootloader 0x2000,
CYD 3.2 classic ESP32 bootloader 0x1000, S3 bootloader 0x0). Then it
downloads and SHA-256 hashes every segment. It requires HTTPS/Web Crypto,
and a new selection invalidates stale in-flight checks. The board confirmation
remains required separately. Failed or missing validation leaves both install
and download locked and displays the reason.

The publisher retains all three board targets, public/stable v1.0.0 and LAB
channels, latest-per-board filtering, current build labels and serial baud
selection. No new release or merger is performed by PR #3.

**Limitations:** checksums protect against corruption between the generated
CI catalog and the assets obtained during preflight; they do *not*
authenticate the firmware publisher or physically identify the PCB, and
ESP Web Tools may fetch the files again during the actual flash. Use only
the trusted site, confirm the exact board, and back up the existing firmware.

### Verification checklist

1. Load LAB installer, select each of CYD 3.2, C5 and S3; confirm a
   verification message appears *before* the board checkbox enables.
2. With unmodified CI binaries, verify all four segments pass and the
   checkbox then unlocks flashing.
3. In a local staged-site copy, deliberately replace or truncate each
   segment in turn; each must fail closed with no flash/download.
4. Tamper with the manifest chip family or offset; refuse to enable flash.
5. Change board/build while verification is running; ensure the previous
   build cannot unlock the new selection.
6. Confirm baud selector and current build descriptions still work.
7. Test a known-good physical CYD 3.2 and receiver smoke test before merge.
