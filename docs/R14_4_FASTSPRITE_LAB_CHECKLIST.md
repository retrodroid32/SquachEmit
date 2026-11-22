# SquachEmit R14.4 LAB — CYD 3.2 FastSprite / native frame cadence

**Scope:** hardware-test firmware only. Do not merge PR #66, tag or publish a production release. All changes stay in SquachEmit; SquachWatch is the read-only reference.

## Source comparison and fix
- SquachWatch's standard ESP32-2432S032R uses two RGB332 screen bands at 40 MHz, a 33 ms CLEAR redraw target, and optimized FastSprite framebuffer primitives.
- SquachEmit R14.3 used those two bands and 40 MHz but the standard TFT_eSprite and a 65 ms NORMAL frame limit.
- R14.4 ports SquachWatch's original optimized drawPixel, drawFastVLine, drawLine and GLCD drawChar methods, **including its memory-safe setWindow clipping guard for band viewports**, while keeping the existing 8-bit two-band renderer.
- It targets 33 ms on NORMAL (LOW 100 ms; HIGH 25 ms). Actual measured FPS may be lower; no 80 MHz overclock has been enabled.
- Original screen timing (R14.3), first-band-only state advancement, portrait original Squachy, and single speech bubble remain intact.

## Hardware verification: ESP32-2432S032R ST7798
1. Flash R14.4 only via the LAB channel after successful CI and verified four-part SHA-256 preflight; preserve your known-good R14.3 build.
2. Cold-boot several times; confirm no reset loop, screen corruption, misplaced outfits or duplicate speech bubble.
3. Set animation to **Normal**. Record 3–5 `CYD32,FRAME,PERF,fps=...,draw_us=...,push_us=...,frame_us=...,heap=...` serial lines on Synthwave, Snowfall, Aquarium, Starfield, Flying Toasters and Fireflies.
4. Compare the same background and NORMAL setting against SquachWatch on equivalent 3.2 hardware. If SquachWatch is using an 80 MHz fast firmware variant, identify it; R14.4 uses 40 MHz and cannot be compared as a clock-equal control.
5. Watch character motion and top/bottom seam at landscape y=120 and portrait y=160 for at least 30 seconds per scene; rotate back and forth and change outfits/backgrounds.
6. Confirm touch/edge background switching and controlled RUN/STOP/SUITE radio tests work. Check reported free heap, counts and screen updates.
7. If motion is still jerky, send the performance lines. Large `draw_us` calls for computational primitive/band work; large `push_us` points to SPI transfer limit.

No claimed performance improvement without these physical measurements.
