# R14.2 LAB — CYD 3.2 original SquachWatch two-band scene synchronization

**LAB only** — stacked from R14.1, PR #63. No production tag or merge.
Hardware-reported symptom: top and bottom of animated Synthwave/Snowfall scenery and Squachy are visibly out of step across the 320x240 ST7798 horizontal midpoint.

## Changes in this candidate
- Use one captured `logicalFrameMs` for both 320x120 landscape or 240x160 portrait band submissions.
- Keep `advanceScene` true only for the first pass, with shared `frameNow()` in background/character rendering.
- Supply DrawBand clipping for each panel-space half and disable clipping after the frame.
- Correct the compile-time portrait branch: original Squachy now runs with `SQE_ORIGINAL_SQUACHY` rather than being omitted.
- Original landscape Squachy does not depend on separate custom-costume sprite allocation.
- Retain the R14.1 single-bubble suppression fix.

## Physical acceptance (not yet performed)
1. Cold boot CYD 3.2 ST7798 and verify stable HOME, responsive touch and no boot loop.
2. Check Synthwave and Snowfall on landscape, especially the horizontal midpoint at y=120 as background and character move.
3. Cycle all ten backgrounds using left/right edge taps; verify no seams, flashing or stale scene elements.
4. Switch original Squachy costumes; watch body/head/feet crossing band boundary for several animation cycles.
5. Tap ROT into portrait: original figure must remain original, with seamless transition at y=160; rotate back.
6. Exercise TESTS, RUN/STOP and SUITE at safe controlled output, check status/countdown and memory stability.
7. Confirm only one top dialogue box and no native Squachy duplicate in both orientations.

## Build/publication
The separate LAB branch is intentionally used so a successful push CI can be discovered by the LAB-only Web Flasher. No stable release is created.
