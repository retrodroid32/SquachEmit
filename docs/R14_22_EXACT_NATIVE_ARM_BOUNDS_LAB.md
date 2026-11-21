# SquachEmit R14.22 — native arm alignment, no shifted sleeve overlay

## Hardware defect
The user reports the Black Hoodie and Neon Lab Jacket sleeves in R14.21 are shifted **to the right** of Squachy's original arms.

## Root cause
R14.21 rendered the real SquachWatch arms in fur, then repainted a **second independent sleeve** using a reconstructed arm origin `s_arm0x-S(4)`, a guessed arm pose, and separate geometry. Those painted rectangles can be offset from original SquachWatch's true native arm fill locations, particularly at fractional display scales and non-rest poses.

## R14.22 correction — eliminate duplicate sleeve geometry
1. In `drawBody()`, define `armFill` as the original `furLight` for all approved SquachWatch outfits, or the correct Hoodie/Lab Jacket fabric color for SquachEmit's two custom styles.
2. Use `armFill` **directly in the native arm `fillRoundRect` and `limbTo` → `wideLine` calls**. This means the costume sleeves have exactly the same rendered x/y, width, height and silhouette as Squachy's real arms, by construction.
3. For custom styles only, add **small cuffs and furry hand tips** within that original arm using recorded exact left/right native rectangle bounds, live Y sway and pose widths. No duplicate full-arm colored overlays.
4. Leave the Hoodie hood, strings, pocket, Lab Jacket lapels/pockets/hem, original face, original costumes, 10 backgrounds, 172 scenarios, screen orientation and R14.17 Idle/TX speech unchanged.

## On CYD 3.2 ST7798
- Flash R14.22 **LAB** with four verified bootloader/partition/OTA/firmware images and SHA-256 checks.
- Both Hoodie and Lab Jacket colored arms must now match the actual original arm silhouettes with **no right shift, detached tube or shoulder gap**.
- Observe 20+ seconds of idle arm sway and moving poses, portrait and landscape, and repeat after cycling original approved outfits.
- Verify no new pink sprite box, redraw/flicker, misplaced hood, or changed original costumes; RUN/STOP and speech behavior remain correct.

## Build and release
All four CI jobs must pass, followed by the `Build and Release FW (R14.22 LAB)` job. That job issues a uniquely tagged **LAB prerelease** with complete four-part CYD 3.2 ZIP and SHA-256. Physical visual approval is still required; no production merge or stable firmware changes.
