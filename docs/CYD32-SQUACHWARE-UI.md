# SquachEmit SquachWare-style CYD 3.2 display redesign

This LAB commit changes the SquachEmit **ESP32-2432S032R ST7798/XPT2046**
display **only**. Base: the existing 49-case Smooth UI emitter in PR #6.

## Original SquachWatch UI reference
See retrodroid32/SquachWatch-CYD, `docs/SQUACHWARE-AESTHETIC.md`,
`include/theme.h`, `src/theme.cpp`, and `src/ui_settings.cpp`.
SquachEmit independently draws a compact original Sasquatch mascot;
it does **not** copy large theme.cpp, costumes, backgrounds or the
handshake-capture/adversarial functionality of Pwnagotchi.

Landscape 320x240 layout:
- vaporwave purple → cyan → pink title stripe and neon branding
- original miniature Squachy reacts to idle / suite / radio-on states
- mascot speech panel with selected scenario/active timer
- ALL / DRONE / BLE / WIFI filters, three clear test rows per page
- pinned PREV/NEXT and RUN/STOP/SUITE touch controls
- update only timer digits while RF runs; redraw page on changes only
- no large framebuffer, no BLE/Wi-Fi changes, no emissions on boot
- NVS key `sqemit32/touchland` for landscape touch calibration; the old
  portrait `touch` key is preserved but not used, preventing rotated fit.

Pwnagotchi concepts evaluated: character statuses (`ui/view.py`),
UI change tracking (`ui/state.py`), and channel observation stats.
These are ideas rather than GPLv3 code reuse; the device still uses
SquachWatch's recognizable color vocabulary and appearance.

## Hardware checklist
1. Verify firmware targets 3.2-inch ESP32-2432S032R only, never the C5.
2. Flash experimental binary and confirm titlebar, landscape orientation,
   correct BGR colors, backlight and Squachy icon. First boot will prompt
   touch recalibration because the screen now rotates to landscape.
3. Calibrate five corners and verify ALL/DRONE/BLE/WIFI filtering; select
   an item on each page; PREV/NEXT, RUN/STOP/SUITE.
4. Verify serial LIST still shows 49 cases and STOP stops any active RF.
5. Run a 12-second controlled case and confirm seconds update without
   repaint/flicker, then verify timer and state reset on STOP/timeout.
6. Switch off and power back up; ensure landscape touch calibration persists.
7. Check CALTOUCH at 115200 returns to recalibration and does not
   silently erase the unrelated portrait key or SquachWatch settings.
8. Receiver evidence must come from a separate physical CYD. A CI PASS
   proves compilation only, not visible hardware correctness.

No PR merging, releases, tags or SquachWatch firmware changes.

## Redesign R3 — separate Squachy Home and Test Library

The first redesign remained crowded: the character, four category tabs,
case rows, a speech bubble and navigation all competed for 240 vertical
pixels. This revision is intentionally a **two-screen interface**:

**Home** (starts at boot): 17px SquachWare vaporwave titlebar, real
RF OFF/ON status pill, enlarged original hand-drawn Sasquatch silhouette,
speech/active test card, separate profile summary, and four large bottom
buttons: TESTS, RUN, STOP, SUITE. Tapping Squachy or the bubble opens the
test library. This is much closer to the dominant Squachy/home-screen
composition of SquachWatch than the old compact administrative list.

**Library**: 4 filters ALL/DRONE/BLE/WIFI, four large item rows per page,
prominent pink selected row, visible expected class, and bottom
HOME / PREV / NEXT / RUN (or STOP when emitting). A full page is redrawn
only for navigation or radio-state changes, and on Home the remaining
seconds update within a small text rectangle.

The same `sqemit32/touchland` calibration still works. No partition,
serial protocol, RF fixture or transmitter power change is introduced.
All new art is original drawn C++ primitives, not GPL code copied from
SquachWatch or Pwnagotchi.

### Verifying R3

1. First boot should open a **large Squachy home**, not a grid of test
   buttons. RF pill must say OFF / IDLE and no emissions should start.
2. Touch TESTS, change filters, scroll multiple pages and select a test;
   touch HOME and verify the selected ID appears in Squachy's speech card.
3. RUN should show the ON AIR status and seconds; the latter must update
   without flickering or repainting the mascot.
4. STOP and the automatic timeout should return to RF OFF / IDLE.
   Verify SUITE and STOP can be operated from Home.
5. Compare actual detector results on a separate SquachWatch CYD.
6. Validate touch after power cycle and use CALTOUCH at 115200 if needed.
