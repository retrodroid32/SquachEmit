# CYD 3.2 DJI Wi-Fi Remote ID startup recovery LAB

Applies to the **unmerged 135-case synthetic DJI BLE/Wi-Fi LAB**, not the
public v1.0.0 release. This change addresses intermittent Wi-Fi startup
and exposes failures that previously looked like an unresponsive RUN key.

## Recovery and diagnostics

Each Wi-Fi Remote ID test logs `RADIO,WIFI_ATTEMPT,<ID>,1`. A failed
driver step emits `ERROR,WIFI_STAGE,<STEP>,<CODE>,<ESP-IDF-NAME>`.
The code fully stops the Wi-Fi AP, allows a 120 ms recovery interval and
retries **once**. It does not downgrade to another transmission mode.

Failure after both attempts displays **WIFI START FAILED** and its stage
on the CYD 3.2 speech bubble, prints `ERROR,WIFI_CASE,<ID>,<STEP>,<CODE>`
and emits `TRUTH ERROR`, not a false `BEGIN`.

## Hardware tests — LAB Web Installer, CYD 3.2 ST7798

Install from https://retrodroid32.github.io/SquachEmit/?lab=1.
Use **Serial Test Console at 115200 baud**.

1. Start `RUN RID-WIFI-DJI-MAVIC2PRO 12`. Look for
   `RADIO,WIFI_ATTEMPT` and `TRUTH BEGIN`.
2. Repeat with `RID-WIFI-DJI-P4PROV2`, `RID-WIFI-DJI-MAVIC3`,
   `RID-WIFI-DJI-MINI3PRO` and `RID-WIFI-DJI-AIR3`.
3. Alternate BLE/Wi-Fi profiles ten times; confirm no leftover radio
   state, display overlap or missed STOP/timeout.
4. If a case fails, collect its **ERROR,WIFI_STAGE** and
   **ERROR,WIFI_CASE** lines so we can isolate the exact driver step.
5. Independently verify reception on a SquachWatch CYD 2.8 or 3.2.
   `TRUTH BEGIN` proves firmware startup success, **not RF reception**.

Model labels and serials are synthetic. There is no genuine DJI proprietary
DroneID/OcuSync transmission or operational FAA Remote ID certification.
