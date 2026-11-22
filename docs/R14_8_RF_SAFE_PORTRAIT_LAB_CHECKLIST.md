# SquachEmit R14.8 LAB — portrait and RF acceptance

R14.8 is draft LAB firmware built on the physically improved R14.7 FULL8/FAST display. SquachWatch remains untouched; do not merge or tag.

## Safety change
The old first-time portrait rotation could enter the blocking TFT_eSPI touchscreen calibration while a timed RF test or suite was active. That can delay normal STOP and timeout processing. R14.8 checks for a saved portrait calibration BEFORE altering rotation. If no saved calibration exists and a radio test, suite or favorites playlist is active, it logs:

    CYD32,ROTATE,DEFER_TOUCH_CAL_RF_ACTIVE

It remains in landscape, retaining STOP and scheduled radio duration. When idle, first-time portrait touch calibration still works. Once calibrated, portrait rotation during RF remains available.

Serial CALTOUCH and COLORS also refuse to run while a test, suite or playlist is active; they emit ERROR,CALTOUCH_REQUIRES_IDLE or ERROR,COLORS_REQUIRES_IDLE.

## Hardware acceptance on CYD 3.2 ST7798
1. Confirm all four CI jobs are green, then use the LAB-only web installer with four-part SHA-256 flash validation. Keep R14.7 available for rollback.
2. Cold-boot twice; verify CYD32,FRAME,FULL8 and PUSH STATUS active=1. Never presume FULL8 if the log reports HALF8.
3. While idle, rotate landscape to portrait and back. If this is the first portrait use, complete calibration while idle. Confirm correct touch mapping, unchanged selection and backgrounds.
4. Check The Gibson, Starfield and Snowfall, scene changes by left/right taps, full Squachy original costumes and one speech panel. Verify no midpoint tearing and no stale rows after menus or rotation.
5. Collect several CYD32,FRAME,PERF lines in FAST mode; compare FPS, push_us and heap with R14.7 (around 16 FPS, 21.7ms push, 122KB heap on one scene). Confirm PUSH OFF restores normal transfers and PUSH ON reenables optimized mode.
6. In a controlled 2.4GHz bench environment with attendance, begin at idle: STATUS and HEALTH should show no active transmitter.
7. Run RUN BLE-AIRTAG 3; expect bounded BEGIN/END approximately 3 seconds later, then STATUS IDLE. Try STOP shortly after starting a separate 3-second run.
8. Run RUN RID-WIFI-DJI-AIR2S 3 with the same duration and STOP checks. If radio startup fails, require an explicit ERROR and no stuck transmitting state.
9. Start SUITE, then STOP promptly; verify STATE,IDLE and no follow-on cases. Do not leave the 172-case suite unattended.
10. During a bounded active test, CALTOUCH and COLORS must be refused with *_REQUIRES_IDLE. Once portrait is calibrated, rotate while a 3-second test is active and confirm STOP still responds.

## Validation limitations
The first-time calibration deferral is exercised in native C++ tests for both on-air TX and suite mode. Previously configured devices can test rotation with cached calibration without erasing settings; do not erase NVS merely to reproduce first-time calibration. Receiver signal authenticity/actual RF still requires external SquachWatch observation. CI passing is not physical certification.
