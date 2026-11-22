# SquachEmit R14.18 — CYD 3.2 Original-Body Custom Garments

## Physical regression shown in user's two CYD 3.2 photographs
Black Hoodie and Lab Jacket appeared as simplified custom Squachy sprites on a solid pink/magenta rectangular tile in front of the original sunset animation. Other original SquachWatch costumes rendered through a separate procedural engine correctly.

## Exact cause and source fix
- `src/cyd32_ui.cpp` previously **excluded outfit levels 1 and 2** from `Squachy::tick` and instead ran `SquachyPort::render` on an 8-bit RGB332 112×111 scratch sprite, then used `0xF81E` (16-bit RGB565) as a transparency key. The mismatched color conversion allowed pink pixels to remain opaque.
- The now-approved SquachWatch original-art source from merged PR #61 already implements two custom garments in `lib/SquachWatchOriginal/src/squachy.cpp::drawOutfit`. R14.18 copies the *exact* approved extension and declaration into the current R14.17 baseline.
- **All 22 selectable outfits now use the same original animated `Squachy::tick` body, pose and face**, while level 1 = custom dark hoodie, level 2 = custom cyan lab jacket. Levels 0 and 3–21 retain the unchanged upstream look and their original category mappings.
- `Squachy::setSqemitCustomLook(0)` and `setOutfitPreview(-1)` clear temporary costume state after every frame.
- CYD32 original-art builds no longer allocate the legacy mascot scratch sprite; the original engine draws directly into the established FULL8/HALF8 scene frame.
- The unused legacy-color-key fallback remains for builds that explicitly disable `SQE_ORIGINAL_SQUACHY`, not enabled by the CYD 3.2 build profile.

## CYD 3.2 hardware verification (ST7798)
1. Flash the `R14.18 Original-Body Hoodie & Lab Jacket` **LAB** artifact after confirming four-part SHA-256 integrity. Do not use production/stable channel.
2. Landscape Home, background = Synthwave/sunset. Select **BLACK HOODIE**: verify no solid pink rectangle and the sunset remains visible around the moving character, including during the pop-in/out speech bubble.
3. Switch to **LAB JACKET**: no pink or square backing, correct cyan jacket detail on the original face/pose. Observe at least 20 seconds of idle motion.
4. Select **CLASSIC** and at least 3 original SquachWatch costumes; confirm same proportions, original outfits, and unchanged animations. Switch to hoodie/jacket again to rule out stale preview state.
5. Background-cycle through Digital Rain, Gibson, Synthwave and one other original background. All backgrounds must remain visible around hoodie/jacket, with no color-key rectangle or seams from the mascot.
6. Enter portrait, then return to landscape; check both outfits and original art still appear without the box. Test left/right background tap, menu/back and touch controls.
7. Run a bounded synthetic profile, then STOP. Confirm the transmitter and countdown still work. **R14.17 behavior must remain**: idle speech only off air; actual file category or Transmitting speech on air.
8. Confirm animation performance and rendering are smooth in FULL8/FAST and test fallback `PUSH OFF` if needed.

## Scope and gate
SquachEmit only. The SquachWatch repo is not modified. No production merge/tag/release. CI is a build/host fixture check, **not** a replacement for physical CYD 3.2 screen inspection.
