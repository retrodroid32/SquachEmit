# SquachEmit R14.11 LAB — Portrait touch calibration on the physical device

This fixes the complaint that CYD 3.2 portrait touch cannot be calibrated using the device itself. This is a LAB-only candidate derived from R14.10. Do not merge, tag or replace production.

## Method A — working LANDSCAPE touch, no serial required
1. Start with transmitter IDLE. Rotate to landscape (top-right rotate icon if reachable, or boot with already-saved orientation as applicable).
2. Tap top-left hamburger > MENU; advance to page 2 of 3.
3. Tap SETTINGS, then the middle footer MORE > control.
4. On the SETTINGS / DISPLAY page, tap the fourth row, now clearly named **CAL PORTRAIT TOUCH**.
5. Screen changes to portrait and immediately begins the touchscreen calibration wizard. Release any held finger and tap each displayed crosshair in order, using the center of the crosshair and a stylus if needed. The wizard saves portrait touch coordinates and returns to Home.
6. Test the hamburger, background edges, TESTS, RUN/STOP, and portrait-to-landscape rotation. The landscape calibration and other settings are not erased.

## Method B — even if portrait taps are completely mis-mapped
1. Ensure no RF case, suite or playlist is active (idle).
2. In portrait Home, a small hint says **HOLD SCREEN 5s: CAL TOUCH**.
3. Touch and continuously hold an empty area of the physical display for at least 5 seconds. This reads XPT2046 raw pressure independent of saved x/y mapping; it does not need a working hamburger or Settings touch target.
4. Release the finger once the calibration crosshairs appear and complete the four-corner wizard. The active-test guard suppresses the hold gesture if RF is busy.

## Method C — Recovery page
- In landscape, MENU > page 3 > RECOVERY has a visible **CAL PORTRAIT TOUCH** button. Press it to rotate and calibrate.
- In portrait, RECOVERY has an actionable CAL PORTRAIT TOUCH row. If miscalibrated too severely to reach it, use A or B.

## USB fallback if physical touch is totally dead
While the board is fully idle and already in portrait, open a serial monitor at 115200 baud and enter:
  CALTOUCH PORTRAIT
The existing R14.10 command remains functional. TOUCH STATUS and TOUCH TRACE ON/OFF aid diagnosis.

## Regression gates
- Cold boot twice. 320x240 landscape and 240x320 portrait correct; no boot loop; portrait does not silently trust malformed NVS data.
- R14.7 FULL8/FAST display, background transitions, all outfits and one speech bubble remain unchanged.
- Portrait Home has no stray HOME title, only an optional short idle recovery hint; no TX countdown overlay flicker.
- First portrait calibration, *forced recalibration with already-saved touchport*, full-screen raw hold and Recovery UI should each invoke exactly one four-corner wizard.
- Calibrating is forbidden during active timed RF, suite, and favorites playlist; STOP still works.
- Test hamburger, left/right background taps and all footer buttons after calibration. If any remain offset, collect TOUCH TRACE ON output and report observed finger location and calibrated x/y.
- Four-part SHA-256 flash integrity preflight and CI 4/4 success are prerequisites. Native C++ tests do not establish physical touch alignment.
