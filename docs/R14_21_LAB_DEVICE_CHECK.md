# R14.21 — CYD 3.2 LAB hardware verification

**Problem reported:** R14.20 Hoodie and Neon Lab Jacket sleeves do not fully match Squachy's original arm size. The previous custom sleeve engine covered only 80% of the arm using a fixed S(7) stroke, while original idle arms are S(8)×S(22) native rounded rectangles and articulated arms use pose-dependent widths.

**Expected fix:** Original-character-sized sleeves, no reduced arm thickness or short detached cuffs, both sides correctly sized even when one arm waves.

## On-device acceptance
- [ ] Black Hoodie at IDLE: sleeve fully occupies same original arm rounded-rectangle silhouette (S(8) width, S(22) height); natural cuffs and original fur hands, no skinny separate tube.
- [ ] Neon Lab Jacket at IDLE: same full original arm shape, cuffs and collar look proportionate.
- [ ] Observe arms in walk, wave, dance and alert poses: sleeve size follows each native arm width independently, with approximately 94% coverage of the moving segment.
- [ ] Rotate to portrait and back to landscape; confirm both outfits, backgrounds, face, hair, legs, speech bubble and original 20 styles remain correct.
- [ ] Confirm R14.17 idle-only/off-air and category+Transmitting/on-air speech, 172-case library, RUN and STOP behavior are unchanged.

## Mandatory build and release gate
The **Build and Release FW (R14.21 LAB)** GitHub Actions job runs only on push to `lab/cyd32-r14-21-full-size-custom-sleeves-20261120` and only after 4/4 CI jobs pass. It validates a four-part CYD 3.2 firmware set, creates SHA-256 sums and a ZIP, and uploads both an Actions artifact and a **LAB-only prerelease**. No `main` changes, no stable installer promotion, no release marked latest, no SquachWatch repository edits.

**Compilation does not prove display correctness.** Await the user's physical CYD 3.2 validation before merging the source/LAB PRs.
