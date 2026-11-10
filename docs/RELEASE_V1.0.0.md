# SquachEmit v1.0.0 — First Public Release

SquachEmit is a separate ESP32 radio-signal **test transmitter** for evaluating
SquachWatch detection logic. This is the first publicly published version.

## Included
- 95 bounded synthetic test cases, plus offline receiver-only vectors.
- Device-type library with alphabetical categories and test IDs (CYD 3.2):
  Axon, Drones / Remote ID, Flock, General / Controls, Hacker Devices,
  iBeacon, Raven, Serial Modules, Smart Glasses, and Trackers.
- Drone tests appear only in Drones / Remote ID regardless of BLE/Wi-Fi carrier.
- Animated Squachy UI on the CYD 3.2 ST7798, touch calibration and panel inversion.
- Targets: CYD 3.2 ST7798 / ESP32-2432S032R, Waveshare ESP32-C5 2.8",
  and ESP32-S3 DevKitC-1.
- Browser-based firmware installation through GitHub Pages.

**HARDWARE VERIFICATION PENDING.** All targets compile in GitHub CI, but this
does not establish that every physical board, screen, RF profile, or detector
combination works. Please verify against a separate SquachWatch receiver.
No signals transmit automatically at startup; explicit tests expire.
Synthetic advertisements do not prove that the purported device is nearby.

**Web installer:** https://retrodroid32.github.io/SquachEmit/

These release assets are provided for reproducibility; the recommended
installation path is the web installer.
