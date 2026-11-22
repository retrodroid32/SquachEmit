# SquachEmit — Approved artwork consolidation (R14.18–R14.22)

**Hardware approval:** User confirmed R14.22 works on CYD 3.2 on 2026-11-21 and authorized consolidation/closure of superseded artwork PRs. This document does not assert that any additional boards were physically tested.

## Scope

- **Base:** `lab/cyd32-r14-17-state-locked-speech-20261120` (user-validated R14.17 idle/transmitting text behavior; 172 bounded scenarios).
- **Approved source:** `fix/cyd32-r14-22-exact-native-arm-bounds-20261120`, after merges of PRs #107 and #106. Its Git tree matches the published R14.22 LAB head `0ab533bc02a894a02f4c98b87c141c0e68539411` exactly (`4e3acd223f47dfe7dacccd3a146042b261d6d828`).
- **Consolidated branch:** `integration/r14-18-to-r14-22-approved-artwork-20261121`, based on that approved R14.22 source. It targets the R14.17 development baseline, **not `main`**.
- **Prior PRs covered:** #98/#99 (original-body render / removal of pink tile), #100/#101 (true torso anchor, sleeves and hood), #102/#103 (distinct Hoodie and Neon Lab Jacket pixel art), #104/#105 (full-size sleeves), and already-merged #106/#107 (native arm drawing rather than an offset duplicate sleeve).
- **Source parity:** the latest firmware source, tests, CYD build metadata, original 20 SquachWatch outfit paths and 172-case RF scenario behavior come from the same R14.22 accepted Git tree. No SquachWatch repository modification.

## CI cleanup

The per-LAB-branch `Build and Release FW (R14.22 LAB)` job was removed **from this integration branch only**. The remaining `.github/workflows/ci.yml` exactly matches the previously approved R14.17 baseline (blob `466d46d7db82afddbe22173c7fdf6defd9aadf15`). The dedicated LAB branches and published R14.22 GitHub prerelease retain their prior workflows and remain available.

## Release and verification

- Latest **physically accepted** CYD 3.2 LAB prerelease: https://github.com/retrodroid32/SquachEmit/releases/tag/lab-r14.22-cyd32-0ab533bc
- CI on this consolidation PR must pass all four gates (host tests, CYD 3.2, ESP32-S3, Waveshare ESP32-C5) before marking it ready for review.
- Keep this PR draft and **do not merge it into the R14.17 baseline without a separate explicit merge decision**. Keep R14.17 PR #97 targeting `main` unchanged.
- Close superseded PRs #98–#105 with links to this consolidation PR, preserving all feature/LAB branches. Previously merged #106/#107 remain merged.
- Leave Windows Workbench PRs #52/#53/#56, test-suite PR #40, other unrelated PRs and `main` unchanged.

## Hardware smoke test before mainline promotion

Preserve R14.17 Idle-only off air and active-category or Transmitting-category text while transmitting. Confirm classic and original 20 costume behavior, Black Hoodie and Neon Lab Jacket shoulder/cuff alignment and full animation, left/right background changes, landscape/portrait screen redraw, bounded RUN/STOP, and CYD 3.2 firmware version/heap checks. The user has already confirmed the R14.22 artwork; revalidation is only needed if the final integration build changes firmware source.
