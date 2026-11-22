# SquachEmit CYD 3.2 R14.9 LAB — portrait menu, clean Home and TX countdown

R14.9 is a LAB-only firmware candidate based on the previously user-approved R14.7 FULL8/FAST display and R14.8 portrait RF safety changes. No SquachWatch source changes, production merge or tags.

## User-reported defects and intended fixes

1. Portrait MENU was difficult to tap: the glyph/target was tiny and, during active RF, the UI forced Home every tick. R14.9 enlarges the menu glyph and offers a 65 x 42 logical tap region. Navigating menus is possible even while a bounded RF test is running.
2. The unexplained HOME label at portrait y=37 is omitted only on Home. Non-home page titles (MENU, SETTINGS and so on) remain.
3. An active TX countdown was written directly to LCD with different text and dimensions every second, even though FULL8/HALF8 also repainted the same region every frame. R14.9 removes these competing direct panel writes in framebuffer mode, leaving the scene renderer the single countdown owner.
4. To keep STOP accessible while browsing menus during an RF test, the center portrait footer shows and executes STOP on every page while TX, suite or playlist is active.

## Physical acceptance gates

- Flash ONLY the exact R14.9 LAB candidate after the 4/4 GitHub Actions checks pass, using the LAB web installer and SHA-256 four-segment preflight.
- Cold boot twice, confirm CYD32,FRAME,FULL8 and PUSH STATUS reporting active=1,frame=FULL8. If HALF8, note the heap and retest carefully; do not claim FULL8 speed.
- Landscape home and background (The Gibson, Starfield and Snowfall) must look unchanged, with the R14.7 seamless scene and responsive touch.
- Rotate to portrait. Confirm the upper-left three-line menu icon is visible and tapping across its button (especially the RIGHT or LOWER edge) opens MENU. Verify MENU rows, BACK and further screens work. Rotate portrait to landscape and back.
- On portrait HOME there should be no standalone HOME label over the background. On MENU/SETTINGS their proper titles must still be present.
- In portrait, RUN a 3-second synthetic BLE case under controlled lab conditions. The TX countdown should smoothly update without flashing, doubling, erasing adjacent text, or creating streaks. Verify the test ends after the configured time.
- While a bounded test is active, open portrait MENU, confirm MENU **stays** open and footer center displays STOP; press STOP and verify RF shuts off immediately. Repeat with short SUITE and then STOP promptly; never run unattended.
- Test portrait background edge taps, Squachy outfit, portrait calibration (if needed), and one speech bubble.
- Capture PERF before/after in same scene; R14.7 FAST hardware reference was about 16 FPS, 21.7ms push_us, 122KB free heap; physical time can vary. Monitor reboot, heap, and fallback count.
- If any blocking touch, STOP failure, RF timeout regression, panel corruption or reboot occurs, revert to R14.7 or R14.8 LAB and retain logs.

These checks require a physical CYD 3.2; CI host tests cannot certify real touch calibration or SPI transfer behavior.
