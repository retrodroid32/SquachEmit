# SquachEmit R14.22 — exact left/right sleeve alignment

## Device report
The R14.21 full-size custom Black Hoodie and Neon Lab Jacket sleeves appear **shifted to the right** of Squachy's original arms. User explicitly requests fix + rebuild + **Build and Release FW** on each broken-hardware report.

## Root cause
The native SquachWatch renderer draws idle/rest/walk arms directly at:
- LEFT: `fillRoundRect(cx2 - S(18), hy + S(22) + armSwingL, S(8), S(22), S(3))`
- RIGHT: `fillRoundRect(cx2 + S(10), hy + S(22) + armSwingR, S(8), S(22), S(3))`

R14.21 custom art reconstructed its left/right sleeve x using `s_armL/R0x - S(4)`, where `s_armL/R0x` was a separately scaled shoulder CENTER (cx2 ± S(14)). Truncated integer scaling means this reconstructed value does not necessarily equal the original native arm's exact left edge. The error can shift custom sleeve pixels right relative to the real arm.

## R14.22 correction
- `restArms(dL,dR)` records the **native** left and right roundrect top-left x/y from the exact expressions above, including per-side animated Y swing.
- `dressArm` uses these exact four saved bounds for native-rectangle poses instead of recomputing x or y from the center.
- When `limbTo` is used, the original pose's exact arm endpoint coordinates and width still control sleeves as in R14.21.
- Hoodie and jacket colors, native body proportions, hand shapes, animations, backgrounds, portrait/landscape, speech selection and 172 RF cases remain unchanged.

## CYD 3.2 hardware acceptance
1. Flash the LAB R14.22 four-part package with its SHA-256 checksums on the CYD 3.2 ST7798.
2. Observe Black Hoodie on an original background. In IDLE, the colored left and right sleeves must sit directly on the original fur arm silhouettes **without a rightward gap on either side**.
3. Repeat with Neon Lab Jacket. Check shoulders, cuffs and real exposed hands while left/right idle sway runs.
4. Test moving arm poses, original outfit switch, landscape/portrait rotation and multiple backgrounds; make sure no new misalignment, pink box, redraw or flicker.
5. Check R14.17 state-scoped speech remains idle-only when RF off, active category/Transmitting when RF on; RUN/STOP still work.

## Build and release gate
Four GitHub CI jobs must pass. LAB-only Build and Release FW publishes the complete four-part CYD32 package plus SHA-256 as a uniquely tagged GitHub prerelease. Production `main`, stable installer, SquachWatch repo and historical PR branches remain untouched. A compilation pass is not the same as visual acceptance on the hardware.
