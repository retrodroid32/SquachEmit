# SquachEmit R14.22 — LAB firmware acceptance (CYD 3.2 ST7798)

**Reported problem:** On R14.21, hoodie/lab jacket sleeves are shifted to the RIGHT of the original character's arms.

**Code change:** Native idle/walk arm rectangles are rendered at `cx2-S(18)` (left) and `cx2+S(10)` (right), with top `hy+S(22)+armSwing`. R14.21 used `s_arm0x - S(4)` and `s_arm0y-S(2)`; truncation of independently scaled integers causes misaligned custom colors. R14.22 records each real top-left arm rectangle origin directly for painting sleeves, without changing sleeve sizing or artwork.

## Check on real device
- [ ] Black Hoodie, landscape and idle: color exactly covers both native arm outlines, no rightward offset.
- [ ] Neon Lab Jacket: same alignment on left and right, both shoulders and cuffs; verify slow idle sway.
- [ ] Check WAVE, WALK and other arm poses to confirm the arm layers follow actual moving positions.
- [ ] Portrait and landscape rotation do not change sleeve alignment.
- [ ] Approved original 20 outfits and animated scenes remain correct.
- [ ] R14.17 speech eligibility (idle vs TX category) still works, plus RUN and STOP.

## Release gate
All four jobs (`host-fixtures`, CYD 3.2 ST7798, ESP32-S3, Waveshare C5) must pass. Then `Build and Release FW (R14.22 LAB)` produces the four-part CYD32 ZIP, checksum file and GitHub **LAB prerelease**. Never merge to production `main` or modify SquachWatch without explicit approval.
