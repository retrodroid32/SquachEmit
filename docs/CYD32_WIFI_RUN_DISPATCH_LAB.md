# CYD 3.2 Wi-Fi DJI RUN button: distinguish UI touch from radio startup

The operator reports that **RUN works for other signal types** but seems
to do nothing for selected Wi-Fi DJI profiles. This is not assumed to be
a general touch-calibration or debounce problem.

This LAB keeps touchscreen input timing unchanged and instruments the
existing Wi-Fi startup path:

- On accepted RUN: `UI,RUN_TAP,<case>,WIFI_RID`, then
  `UI,RUN_DISPATCH,<case>` and a visible **STARTING WIFI TRANSMISSION**.
- If nothing is logged: the UI didn't dispatch the requested touch.
- If accepted: `RADIO,WIFI_ATTEMPT,<case>,1` reports Wi-Fi mode startup.
- If startup fails: `ERROR,WIFI_STAGE,<stage>,<code>,<name>` and a
  **WIFI START FAILED** screen, with one bounded teardown/retry.
- A `TRUTH BEGIN` means the driver accepted configuration; independent
  radio reception on SquachWatch is still necessary to prove on-air output.

## Compare from the web installer's 115200 baud Serial Test Console

1. Flash the CYD 3.2 ST7798 LAB build containing this change.
2. Select `RID-WIFI-DJI-MAVIC2PRO` on-screen; tap the same RUN button
   that works for BLE. Watch for `UI,RUN_TAP` and `STARTING WIFI`.
3. Repeat with `RID-WIFI-DJI-P4PROV2` and `RID-WIFI-DJI-MINI3PRO`.
4. Run a known-good BLE case between Wi-Fi attempts.
5. On failure, report exact profile ID plus the first
   `UI,RUN_TAP` / `ERROR,WIFI_STAGE` or `ERROR,WIFI_CASE` line.
6. The right-side case count/countdown should never overlap.

No copied serials or genuine DJI protocol. This is a synthetic ASTM-shaped
receiver test, not a real DJI aircraft transmission. No merge authorized.
