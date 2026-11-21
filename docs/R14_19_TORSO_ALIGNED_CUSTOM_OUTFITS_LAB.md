# R14.19 LAB — custom outfit alignment (CYD 3.2 ST7798)

## Visual evidence / regression
Two user-supplied photos of R14.18 show the NEON LAB JACKET and BLACK HOODIE wearing the original SquachWatch face correctly but with an oversized clothing panel placed over the mouth, cheeks, and upper chest. The old pink rectangle was eliminated; outfit positioning was still wrong.

## Root cause
The custom `s_sqemitCustomLook` garment block was placed at the top of the original `drawOutfit()` function. `drawBody()` calls `drawOutfit(t, cx2, hh,...)` with **hh = animated head origin**, while the original torso is drawn at `hy + S(23)`, width `S(2 * torsoHalf())`, height `S(18)`. Using `hh + S(19)` and a fixed 26x18 coat shape drew the garment above the body, including over the face. The separate round hood remained a floating ring.

## R14.19 correction
- The custom hoodie/jacket torso is drawn **inside drawBody at the original torso location**, before animated arms and head. It uses the original torsoHalf(), height and roundness, not an oversized separate rectangle.
- Custom sleeves draw on the real arm segments from the pose-updated `s_armL0x/s_armL1x` and `s_armR0x/s_armR1x` endpoints, stopping short of the furry hands. Sleeves have subtle cuffs.
- The hood, only for Black Hoodie, is painted **behind the head** at `hy + s_headDrop + actHead`, so the original face and shades render on top and move naturally.
- Removes the incorrect custom branch from `drawOutfit()` entirely. All 20 SquachWatch originals remain in their original draw paths.
- R14.18 direct original-body rendering (no magenta 112×111 sprite), R14.17 idle/transmitting speech selection, backgrounds and RF logic are not modified.

## Physical test matrix
1. Flash the `R14.19 Torso-Aligned Custom Hoodie & Lab Jacket` **LAB** firmware on CYD 3.2 ST7798 using the four-part SHA-256-verified layout.
2. Display NEON LAB JACKET on SYNTHWAVE background. Jacket should cover torso below chin, not glasses, cheeks or mouth; fur hands/legs should remain visible; background still visible around the entire character.
3. Display BLACK HOODIE on SYNTHWAVE. Hood should follow the head, not float as a big cyan ring; chest/pocket and sleeves remain below face and follow idle sways.
4. Cycle original costumes (CLASSIC, TANOOKI, STRIPES etc.) and back to the two customs: originals must render identically and no stale garment overlay remains.
5. Set FAST, NORMAL and SLOW, then compare moving arms and head bob, watching sleeves and hood; no jumpy stationary garments, no screen flicker.
6. Repeat both custom outfits in portrait and landscape, switch through multiple original backgrounds, test touch/rotation.
7. Run/STOP a bounded profile. Idle speech stays idle-only; transmitting speech stays active category or transmitting bank (R14.17 unchanged).
8. Check serial or settings for allocation failures, verify the CYD display remains responsive. Original RGB332 FULL8/HALF8 fallback remains unchanged.

## Gates
All four CI jobs (native host, CYD32, S3, Waveshare C5) must pass. Builds are automated; **visual correctness requires physical CYD 3.2 approval**. Do not merge, delete branches, create stable tag or production release without approval. Do not edit the SquachWatch repository.
