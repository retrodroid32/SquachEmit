# R14.5 LAB — Gibson and Starfield screen-seam correction

**Scope: SquachEmit CYD 3.2 only.** PR #68 holds the code change; this LAB branch is for CI/Web Flasher discovery. SquachWatch repository untouched. No production merge, tag, or release.

## Confirmed source defect
R14.4 passed one `frameNow()` to the two 8-bit display bands, but Gibson's `drawGibson` camera movement still interpreted an unchanged timestamp as a new 16 ms step. The second pass moved the camera another 3.2 world units while painting the bottom half; buildings and the neon grid could not align with the top half. The Starfield tunnel's scroll/rotation had the identical fallback.

The SquachEmit-only fix uses `SqEmitSceneClock::elapsedOnce(now, previous)`: same timestamp -> zero motion; next logical frame -> elapsed time, capped at 100 ms; first frame -> 16 ms seed.

## Acceptance on the physical CYD 3.2 ST7798
1. Verify R14.5 LAB / 172 cases and SHA-256-checked four-part flashing. Cold boot twice, no reboot loop.
2. Open **THE GIBSON** (neon city + green floor), leave it moving for 60 seconds. Compare vertical tower and perspective lines immediately above/below the **y=120** horizontal band seam. They must align as one scene. Repeat on several scene animation frames and a high-motion pass.
3. Open **STARFIELD** and watch the rotating tunnel across y=120. It must use the same tunnel phase and angular rotation on both halves.
4. Test Snowfall and other moving backgrounds, left/right edge switching, and all original Squachy costumes. Watch the head/body line at y=120.
5. Rotate to portrait and verify same at y=160, rotate back; confirm touch and screen content remain aligned.
6. Capture the serial `CYD32,FRAME,PERF` records (fps, draw_us, push_us, frame_us, heap) for Gibson and Starfield. Smoothness may still be lower than an 80MHz SquachWatch fast build; this change retains the standard **40MHz** display clock.
7. Smoke-test controlled RUN/STOP/SUITE 172-case functions, with no RF emission before RUN.

**Build CI passing does not validate a physical screen.** If the seam remains despite this correction, investigate LCD push timing or another scene-state mutation before adding more performance work.
