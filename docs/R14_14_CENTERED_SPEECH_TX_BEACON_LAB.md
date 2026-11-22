# SquachEmit R14.14 LAB — centered speech and redesigned TX beacon

Base: user-validated R14.12 SquachWatch-style 5-target portrait touch, plus R14.13 randomized speech display. No radio/transmitter behavior changes. LAB only, no merge or production release.

## Physical CYD 3.2 acceptance
1. After GitHub CI is fully green 4/4 and the LAB firmware manifest passes the four-part SHA-256 preflight, flash R14.14 using the LAB-only installer.
2. Landscape Home: wait for the first randomly timed bubble (15–30 seconds). Confirm the text is truly centered horizontally, and the border hugs the line with about 13 pixels of horizontal margin and a relaxed ~7px vertical inset.
3. Observe several different idle phrases including long and short ones: short lines should draw visibly narrower bubbles; no 306-pixel fixed-width banner; all margins balanced and no clipped letters.
4. Observe 220ms bubble pop-in and disappearance after about 5 seconds. Background animation must remain intact; no direct LCD overlay, ghost rectangle, or flash.
5. Check the top-right transmit icon. It is now a symmetric radio-wave beacon, cyan while idle, vivid green while RF is on. No old triangle/antenna remains. It must remain distinguishable against the gradient header.
6. Start a short, attended RUN (3 seconds), check the beacon turns green, transmitted text remains centered, countdown is smooth, then verify automatic timeout and manual STOP. Be sure no new radio behavior or ACTUAL receiver changes.
7. Trigger/inspect a Wi-Fi start failure only if it occurs naturally; its two-line speech box must center both lines with appropriate width and height. Do not intentionally cause unnecessary radio errors just to test layout.
8. Confirm landscape lower PROFILE label still displays selected case (and the bubble never repeats it). Test switching outfit/background and navigating menu; no overlap.
9. Rotate portrait and verify the new beacon also appears at the top-right. Confirm R14.12 touch calibration works; do not force recalibration unless needed. Test portrait menu and controls.
10. Capture CYD32,FRAME,PERF and PUSH STATUS. Ensure FULL8/FAST, heap and speed remain near the verified baseline, with no new flicker, resets or graphics corruption.

Do not treat green CI as proof of physical screen alignment. Keep existing successful LAB firmware for rollback.
