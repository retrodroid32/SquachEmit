# R14.3 LAB acceptance — animation stepping and pacing

This is an **unmerged LAB-only** candidate stacked on R14.2. Do not create a production release or tag and do not edit the SquachWatch repository.

## Source-level correction
- SquachEmit R14.2 called the individual original SquachWatch background renderers twice per LCD frame. These scene functions do not by themselves manage the original per-frame `s_animK` animation factor.
- R14.3 instead calls SquachWatch `Theme::drawActiveBackground` to advance stepping animations in band 0 and freeze frame-to-frame increments in band 1. Both bands still share the same millisecond timestamp.
- The normal animation cadence target is now 65 ms (formerly 130 ms). Actual LCD FPS is measured, not assumed.

## CYD 3.2 ST7798 hardware verification
1. Flash R14.3 LAB only when all CI jobs pass and the LAB installer lists this exact label.
2. Cold boot. Confirm firmware, touch, no boot-loop, no purple squares, no duplicate mascot speech bubble.
3. Observe moving Synthwave, Snowfall, Starfield, Aquarium, Flying Toasters, and Fireflies scenery for at least 30 seconds each. At y=120 landscape / y=160 portrait, check that moving objects and Squachy are a single coherent image.
4. Tap left/right edges to cycle all ten original backgrounds. Rotate portrait and landscape while retaining scene and outfit; check screen consistency and touch.
5. Open the serial console at its configured baud and capture `CYD32,FRAME,PERF,fps=...,draw_us=...,push_us=...,frame_us=...,heap=...` at idle. Compare 2-3 backgrounds. A slow frame may mean CPU rendering or SPI transfer bottleneck; target 65 ms does not guarantee 15 fps.
6. Confirm RUN/STOP/SUITE still work for time-limited synthetic RF test cases, and check free heap and SD state if applicable.

Any remaining midpoint tear after this update may involve ST7798 panel transfer timing, or other remaining state mutations inside Squachy's source. Physical tests are decisive.
