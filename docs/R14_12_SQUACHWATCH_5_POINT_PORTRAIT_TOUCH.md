# SquachEmit R14.12 LAB — SquachWatch five-target portrait touch calibration

This LAB-only change replaces R14.11's old Bodmer TFT_eSPI corner routine. That routine loops four corner arrows but does not require release between targets. If a finger lingers on corner three past the 1-second delay, it can silently satisfy corner four. SquachWatch's actual code waits for release, collects stable median raw samples from FIVE inset targets (held about 1 sec each), fits a six-parameter affine map, and verifies a separate dot before saving.

## Physical CYD 3.2 test
1. After 4/4 GitHub CI success, install R14.12 LAB using SHA-256 Web Flasher. No production merge/tag/release.
2. Ensure transmitter idle. From working landscape, MENU > page 2 > SETTINGS > MORE > CAL PORTRAIT TOUCH. Alternative: in portrait, five-second idle raw hold. Serial CALTOUCH PORTRAIT remains an alternative.
3. Release the finger that initiated calibration. TOUCH SETUP appears; tap anywhere to start.
4. Each of five inset ring targets displays TAP AND HOLD 1 SECOND N/5. Keep your finger steadily pressed until green. Lift completely between targets. Sequence top left, top right, bottom right, bottom left, center.
5. After five independent taps, NOW TAP THE DOT appears. Tap it; verified fit yields TOUCH SET. A poor fit is retried or NOT SAVED, never silently accepted.
6. The serial log must show CYD32,TOUCH,TARGET,1/5 through 5/5, CYD32,TOUCH,CHECK_DOT and CYD32,TOUCH,5_POINT_VERIFIED_SAVED. No return after three corners.
7. Test the hamburger, footer, backgrounds and rotation. TOUCH STATUS should report affine5=1. Check after reboot too.
8. Confirm FULL8/FAST animation and bounded 3-second synthetic RUN/STOP; verify STOP is available.

If first-time calibration fails, firmware should return to landscape rather than strand the user on nonfunctional portrait touch. Existing verified fit is preserved if a recalibration fails. Physical XPT2046 verification is still needed.
