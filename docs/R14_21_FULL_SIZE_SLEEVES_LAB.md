# SquachEmit R14.21 — custom sleeves match the original arm geometry

## Hardware feedback
R14.20 repaired original-style Hoodie and Neon Lab Jacket details, but the user reports that the sleeves/arms are not fully matched in **size** to Squachy's real arms.

## Source diagnosis
R14.20 drew custom sleeves as a fixed-width `S(7)` line, with extra outline width and only 80% of real arm length. That does not match the original SquachWatch:
- IDLE/WALK/BOUNCE/standing arms use rounded rectangles with **width S(8) and height S(22)**, centered at `s_armL/R0`.
- Moving arms use `limbTo()` with a variable stroke width, generally `S(7)`, sometimes `S(8)` or another pose-specific width.
- The old costume code did not retain which arm rendering path was used, so sleeve dimensions could not match the actual silhouette.

## R14.21 changes
- Track the original actual arm width per side from `limbTo()` and retain rest/rounded-rectangle flags in `restArms()`. Arms can use distinct geometries when only one moves.
- In rest/idle/walk, fill the **same native roundrect S(8) x S(22)** already used by SquachWatch; keep the native keyline, add appropriately sized cuff and subtle seam.
- In posed modes, use the **actual recorded stroke width** without the old extra 2 pixels of artificial bulk; cover 94% of the arm's length instead of 80%, with the last small portion remaining Squachy's original fur hand.
- Preserve R14.20 native hood, hoodie pocket and drawstrings, lab collar/lapels/pockets, original 20 costumes and backgrounds.
- Do not modify the SquachWatch repository, RF library, R14.17 speech gating, or original face/leg geometry.

## CYD 3.2 physical test
1. Install the **R14.21 LAB Build and Release FW** for CYD 3.2 ST7798, using the complete four-part ZIP and SHA-256 checksums.
2. Check hoodie arms while idle: the colored sleeves should line up with **both sides** of the original Squachy shoulder and actual arm width through the cuffs.
3. Check Neon Lab Jacket arms while idle, then with multiple animated poses/sways; confirm both sides are full-size and follow the moving hands without narrow colored tubes or colored circular fingertips.
4. Switch to an original outfit and back. Confirm body, head and limbs retain original proportions.
5. Rotate into portrait and back, change scenes, and ensure no new pink square, flicker, clipping or broken speech.
6. Verify R14.17 speech logic while idle vs TX and STOP/RUN functionality.

## CI and release
All four CI jobs must succeed. The LAB push pipeline then packages the four CYD 3.2 images, validates SHA-256 and publishes an **unverified prerelease**, not a public/stable release. Visual verification remains required before merging.
