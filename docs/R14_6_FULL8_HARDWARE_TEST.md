# SquachEmit R14.6 LAB — Full 8-bit framebuffer / actual LCD seam test

**Experimental CYD 3.2 firmware only**. Do not merge, tag, or produce a production release. SquachWatch source/reference remains unchanged.

## Why this differs from R14.2–R14.5
The previous builds tried to synchronize the TWO 320x120 / 240x160 animation passes, including the original background frame clock. Physical users still reported a visible horizontal mismatch. R14.6 attempts to avoid the split entirely: it first tries allocating a **320x240 or 240x320 8-bit RGB332 framebuffer (~76,800 bytes)**. If safe memory remains, every background and original Squachy frame is rendered **once** into that full buffer and shipped to the panel in one push. The prior HALF8 two-pass renderer is kept only as a safe fallback.

## Critical serial proof of which path runs
At boot or after rotation, read:
`CYD32,FRAME,FULL8,...,bands=1,heap=...,largest=...,fullCandidate=1,fullFree=...,fullLargest=...` — full-frame correction is actually active.

OR:
`CYD32,FRAME,HALF8,...,bands=2,...` — full-frame allocation was rejected by heap safety; **the seam may still occur because this is the old rendering path**.

The allocator requires 64 KiB free heap plus 24 KiB largest contiguous block after attempting FULL8, preserving some headroom for RF, touch and optional costumes. No overclock or risky direct-to-panel drawing is enabled.

## Hardware acceptance on CYD 3.2 ST7798
1. Flash R14.6 from the LAB installer only after all four GitHub CI jobs pass, with four-part SHA-256 preflight. Preserve your working R14.5 LAB.
2. Confirm `FULL8` at startup. If it says HALF8, capture the whole FRAME line; this release cannot prove full-frame rendering on your device.
3. In landscape, select The Gibson background, watch its vertical towers and green grid crossing former y=120 midpoint for at least 60 seconds.
4. Check Starfield and Snowfall, several original costumes and moving pose, switch backgrounds via screen edge taps, and ensure there is only one speech panel.
5. Rotate to portrait and back, check mode selection and touch again, including the former y=160 seam.
6. Capture multiple `CYD32,FRAME,PERF` lines (fps, draw_us, push_us, heap). Compare CPU paint and panel transfer time against R14.5.
7. Cold boot after 5 minutes and smoke-test RF RUN/STOP/SUITE, including no unintended transmission at boot.
8. If FULL8 shows still a seam, the issue is beyond dual-band scene updates; investigate ST7798 address windows, panel timing, or direct panel writes.

Do not claim the screen fixed from CI alone. All hardware proof remains pending.
