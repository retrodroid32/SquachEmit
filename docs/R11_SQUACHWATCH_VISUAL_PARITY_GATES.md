# SquachEmit CYD3.2 R11 — SquachWatch visual parity and rotation gate

**Scope:** SquachEmit-only draft PR #57; base R10 PR #55. SquachWatch is read-only. Do not merge, tag or publish production firmware without explicit owner approval.

## Source parity achieved in R11
- `include/squachwatch_synthwave_exact.h`: original SquachWatch `Theme::drawSynthwave`, its RGB565 interpolation, sky, sun, stars, birds, water and reflection. Original function and helpers imported from `retrodroid32/SquachWatch-CYD/src/theme.cpp` blob `0b4ee801e295d924a15934d2f2324f1475cfd482`. The band clipping helper is adapted for direct full-band drawing.
- `include/squachwatch_fireflies_exact.h`: original `Theme::drawFireflies` including fog, moon, insects, wind, meadow and occasional events. Full-band draw adapter and default text-floor.
- `include/squachwatch_terminal_exact.h`: original `Theme::drawTerminalLog`, dual-column animated text, original flavor messages, XYZZY overlay. The overlay's SquachWatch *outfit unlock* behavior is not wired to SquachEmit's outfit system.
- Existing Squachy sprite background is made transparent so selected scene remains visible. Direct redraw is confined to the scene region to avoid full-screen clear flicker.

## Gesture/navigation parity
- The first/last 10% of the Home display width switch to previous/next scene on new touch, excluding title and footer zones in both orientations.
- Cycles in SquachWatch's selectable order: DIGITAL RAIN, STARFIELD, FLYING TOASTERS, AQUARIUM, TERMINAL LOG, FIREFLIES, FIRE, SNOWFALL, THE GIBSON, SYNTHWAVE. The retired TUNNEL and BORING-only BLACK are not in the normal cycle.
- Background is saved to `bg_sqwatch`; older R10 `bg` values migrate to the same displayed scene.
- Rotate action is shared across pages, preserves logical screen and selected signal, clears old ST7798 GRAM, re-applies panel inversion, loads separate portrait/landscape XPT2046 calibration, and saves orientation.
- Native regression test covers landscape/portrait/landscape and left/right taps without starting RF.

## Not yet 1:1 — do not mislabel as done
- **Seven background effects:** Digital Rain, Starfield, Flying Toasters, Aquarium, Fire, Snowfall and The Gibson still use R10 reduced renderers. SquachWatch's animation engine includes particle simulation, collision/tappable props, detector-state integration and scenes with large dependencies.
- **All Squachy outfits:** R10's 22 selectable wardrobe entries include Classic/Black Hoodie/Neon Lab Jacket and the original 19 SquachWatch names, but their drawing is still the compact adapted geometry, not source-identical `Squachy::drawBody/drawOutfit`. Exact costume behavior requires porting SquachWatch's original figure/render state machine and unlock/event systems, not merely label data.
- Upstream palette presets, background event unlocks, mascot motion timing and direct-panel/banded-render semantics are not automatically equivalent even in copied scene routines.
- Full authenticated 1:1 parity must be verified visually against *physical SquachWatch* on the same orientation, theme and pose.

## Physical LAB acceptance (CYD 3.2 ST7798)
1. Power on with existing settings; no startup bootloop, no heap allocation failure. Track free heap and largest block.
2. Verify 10 background entries and previous/next wrap around from far edges of Home in both orientations. Selected test never changes.
3. Open Details, Library, Menu, Health, Outfits; rotate and verify same logical page/content and tappable controls, then rotate back. STOP must always remain reachable during active TX.
4. Verify persistent portrait setting, saved touch calibration, original/saved backgrounds after power cycle.
5. Compare Synthwave/Fireflies/Terminal against physical SquachWatch using same scene/theme; note any color, layering, animation or frame-timing discrepancy.
6. Verify radio/Suite run-time scan responsiveness; detect redraw stalls, blink/flicker and terminal heap fragmentation under repeated background switching.
7. Do **not** merge until the user validates on hardware. No OTA claims; huge_app remains single-slot.

Firmware from CI is **compile-only until physically validated**.
