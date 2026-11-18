# SquachEmit R14.7 LAB — Fast LCD FramePush A/B verification

**Test-only CYD 3.2 ST7798 firmware**. SquachWatch is reference only; no merge, tag or public release.

## Why
On the physical CYD 3.2, R14.6 FULL8 mode improved background synchronization and logged:
`CYD32,FRAME,FULL8,320x240,bands=1,heap=139044,largest=65524`
`CYD32,FRAME,PERF,fps=12,draw_us=15321,push_us=38557,frame_us=54035,heap=126368`

R14.7 retains the single-pass FULL8 scene and ports SquachWatch's tested FramePush system to the SAME classic ESP32 SPI/TFT path. It can skip unchanged LCD rows and overlap RGB332→RGB565 conversion with prior FIFO transmission. NO 80MHz SPI overclock, radio or scene engine modifications.

## Mandatory flash/boot acceptance
- Use LAB Web Flasher only after all firmware and host jobs are green; verify all four segments against SHA-256 manifest before flashing.
- Cold boot twice; verify `CYD32,FRAME,FULL8`, no reboot loop and expected backgrounds, outfits and one speech bubble.
- Default optimized transfer ON only for FULL8. If `HALF8` appears because of RAM, the baseline TFT_eSPI push still runs.

## Head-to-head A/B from Windows CMD
Connect USB serial at 115200. With the SAME background, outfit, animation setting and rotation, let the display run 15–30 sec per state:
1. Type `PUSH STATUS` to confirm availability, frame and active mode.
2. Type `PUSH ON` — observe for glitches, capture 3–5 successive `CYD32,FRAME,PERF` lines.
3. Type `PUSH OFF` — observe same scene, capture another 3–5 PERF lines.
4. Type `PUSH ON` again — confirm image returns correctly and capture another 3–5 PERF lines. For faulty output use `PUSH OFF` immediately.

Expected diagnostics now retain previous fields (`fps,draw_us,push_us,frame_us,heap`) and append `push_mode,rows,hash_us,wire_us`. There is no guaranteed speedup: dynamic scenes may change most rows. Watch the difference in `push_us` and measured FPS rather than comparing the theoretical 33ms target.

## Regression matrix
- Gibson city/grid and Starfield tunnel are continuously aligned across previous y=120 seam in landscape; Snowfall particles and Squachy's body align, no flicker.
- Test background edge taps, all original outfits and scene transitions.
- Rotate portrait and back; verify no stale lines, touch works and status FULL8/HALF8 logs match actual allocation.
- Verify text color order, inversion, icons and touch after `PUSH ON` and `PUSH OFF`; no stale frame after leaving menu or color bars.
- Smoke-test the 172-case bounded synthetic RF engine with RUN/STOP/SUITE while monitoring heap and serial.
- Confirm no spontaneous transmissions on boot and USB commands remain available.

## Limitations
Host tests verify RGB332 wire conversion and changed-row span arithmetic, not physical SPI FIFO behavior. Only CYD 3.2 device observations establish correctness and a real FPS gain.
