# DJI OUI-only transmitter LAB: CYD 3.2 verification

Ten new user-triggered OUI-only Wi-Fi beacon cases; proposed catalog has **145 cases**.
These are separate from the already merged 40 synthetic DJI BLE/Wi-Fi
Remote ID cases. They test only how SquachWatch handles registered DJI
MAC prefixes, not DJI manufacturer authentication or true Remote ID.

| Test case | Assigned vendor prefix | Test-only SSID |
|---|---|---|
| OUI-DJI-04A85A | 04:A8:5A | SQE-LAB-DJI-04A85A |
| OUI-DJI-0C9AE6 | 0C:9A:E6 | SQE-LAB-DJI-0C9AE6 |
| OUI-DJI-34D262 | 34:D2:62 | SQE-LAB-DJI-34D262 |
| OUI-DJI-481CB9 | 48:1C:B9 | SQE-LAB-DJI-481CB9 |
| OUI-DJI-4C43F6 | 4C:43:F6 | SQE-LAB-DJI-4C43F6 |
| OUI-DJI-58B858 | 58:B8:58 | SQE-LAB-DJI-58B858 |
| OUI-DJI-60601F | 60:60:1F | SQE-LAB-DJI-60601F |
| OUI-DJI-882985 | 88:29:85 | SQE-LAB-DJI-882985 |
| OUI-DJI-8C5823 | 8C:58:23 | SQE-LAB-DJI-8C5823 |
| OUI-DJI-E47A2C | E4:7A:2C | SQE-LAB-DJI-E47A2C |

Each case runs one standard 2.4 GHz WPA2 SoftAP beacon on channel 6.
Only the fixed three-byte prefix is taken from the DJI MA-L registry.
Last three BSSID bytes are generated from the local board/test identity;
the firmware never changes the chip eFuse. The suffix cannot be guaranteed
not to coincide with a real issued DJI address. This is an explicitly
labeled manufacturer-OUI emulation, not a DJI device or an OcuSync signal.

**No ASTM/OpenDroneID message pack or DJI proprietary DroneID is emitted
by these ten cases.** Existing 135 cases retain their previous behavior.

## Hardware checks

Flash the draft LAB on CYD 3.2 ST7798 from:
https://retrodroid32.github.io/SquachEmit/?lab=1

On the touchscreen, open DRONES / REMOTE ID, select a case beginning
with OUI-DJI, and press RUN. The row explicitly says
DJI VENDOR OUI / NO REMOTE ID.

With the Web Installer Serial Test Console at 115200 baud, run:

    RUN OUI-DJI-04A85A 12

Confirm the following log signals:

- UI,RUN_TAP,<case>,WIFI_OUI
- RADIO,AP_MAC_READY,<case>,04:A8:5A:...
- TRUTH BEGIN, then TEST,OUI_ONLY,<case>,<mac>,NO_REMOTE_ID
- TRUTH END and STATE,IDLE on STOP; timeout must also shut off the AP
- SquachWatch sees the prefix as a manufacturer-OUI detection (not RID)
- Switching back to RID-WIFI-DJI-AIR2S retains its 02:... synthetic MAC
- Home screen displays 145 CASES at idle and countdown only while active

Test all ten; confirm each emits the correct three-octet prefix and
that case MACs do not repeat. Don't score these vendor-OUI beacons as
valid FAA Remote ID broadcasts or confirmed actual DJI drones.

Safety: Only transmit while attending your own receiver test in a
controlled environment where you have permission. Other nearby scanners
could falsely attribute the OUI to DJI; keep duration brief.

PR remains unmerged and physical validation is not yet complete.
