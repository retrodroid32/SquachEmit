# R14.19 — CYD32 hardware acceptance

Firmware is built from the exact R14.19 source branch. User supplied R14.18 photos show the pink square was eliminated but both custom outfits cover too much of the face and do not line up with moving arms.

## Pass/fail checks
- [ ] Neon Lab Jacket begins below the cheeks, fits the existing torso (no floating oversized rectangle) and has animated sleeves that stay aligned with arms.
- [ ] Black Hoodie fits the same torso; hood ring remains behind the face and follows head bobbing, rather than sitting as a floating cyan circle.
- [ ] Both outfits stay aligned in moving poses and on SYNTHWAVE, DIGITAL RAIN and another original scene.
- [ ] Portrait and landscape are both correct. Switch between these and original costumes: nothing persists or leaks.
- [ ] Original costumes, R14.17 Idle/Transmitting speech, button touch, 12s timer, STOP and bounded RF cases are unaffected.
- [ ] No flicker, pink backing square, large intrusive jacket panel or heap allocation failure.

## CI
Require all four jobs: host-fixtures, firmware-cyd32-st7798, firmware-waveshare-c5, firmware (S3). LAB artifact is *unverified* pending physical checks.

No mainline merge, tag, stable release or SquachWatch repository changes.
