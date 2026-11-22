# SquachEmit R14.10 LAB — physically unresponsive portrait hamburger

This LAB build is for CYD 3.2 ST7798. R14.9's enlarged menu hitbox alone did not fix the physical hamburger. TFT_eSPI may discard raw touch pressure/coordinates before any application hit testing, especially close to the glass bezel. R14.10 adds a portrait-only edge recovery and direct diagnostics.

## Firmware verification

1. After 4/4 successful CI jobs, flash the descriptive R14.10 LAB firmware using the four-part SHA-256 integrity preflight. Do not use production. Keep previously hardware-approved R14.7/R14.9 available for rollback.
2. Cold boot; confirm FULL8 and PUSH STATUS active=1. Confirm landscape touch/menu/backgrounds still work.
3. Rotate to portrait. **Before any RF test**, open serial on 115200 8N1 and type:
   TOUCH STATUS
   TOUCH TRACE ON
4. Tap the physical hamburger button once (hold a fraction of a second). The console should report a CYD32,TOUCH,TRACE line with accepted, x, y, raw, z, and menu. For recovered out-of-bounds/low-pressure touches expect CYD32,TOUCH,MENU_EDGE_RECOVERED and CYD32,TOUCH,MENU_OPEN. Menu should open and stay open. Do not infer success from the printed rescue line alone: verify the actual screen.
5. If the hamburger still does not open, capture trace lines during a top-left tap and also a tap near the center, then use TOUCH TRACE OFF. Preserve full serial lines rather than guesses.
6. **If touch calibration looks wrong:** while fully idle and already in portrait, enter CALTOUCH PORTRAIT. Follow the four corner crosshairs. Confirm CYD32,TOUCH,PORTRAIT_CAL_DONE then retry the hamburger. This does not erase unrelated settings. Do not run the wizard during an RF test/suite.
7. If serial touch commands work but physical taps never register, try the existing logical remote-control validation: UI TOUCH 25 18 DOWN, then UI TOUCH 25 18 UP. That establishes whether the UI routing itself works independently of the physical XPT2046.
8. Confirm HOME title remains removed, buffered countdown does not flicker, R14.7 FULL8/FAST graphics look good, and landscape behavior is unchanged. Check short attended RUN/STOP on a synthetic case; suite should stop promptly.
9. Optional trace disable: TOUCH TRACE OFF. Trace is OFF on every reboot.

## Serial reference

TOUCH STATUS -- current orientation, portrait NVS calibration, trace state and raw edge rescue count
TOUCH TRACE ON / TOUCH TRACE OFF -- raw calibrated XPT2046 taps and pressure
CALTOUCH PORTRAIT -- portrait-only touchscreen four-corner recalibration, idle only
PUSH STATUS -- FULL8/FAST state

### Safety
Raw touch rescue checks pressure >=230 and two stable raw XY readings, applies signed portrait calibration, and ONLY permits positions inside / slightly outside the hamburger corner. Other action buttons use normal validated touch. RF/radio settings, original artwork, 40MHz SPI, full-frame renderer and the 172 time-limited synthetic cases are otherwise preserved. Physical success cannot be certified by CI alone.
