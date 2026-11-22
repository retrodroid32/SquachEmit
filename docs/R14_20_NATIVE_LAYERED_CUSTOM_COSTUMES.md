# SquachEmit R14.20 — native-layered custom costume redesign

## Report / photos
User verified R14.19 CYD 3.2 after flashing. Black Hoodie looked olive green with no recognizable hood or pocket; Neon Lab Jacket looked like a simple cyan rectangle with two cyan circles on the arms. User explicitly says broken photos must trigger a fix, **build and LAB firmware release**, not just a handoff.

## Art approach
- Keep the approved PR #61 original SquachWatch procedural character, face, animation, backgrounds and all 20 upstream outfits unchanged.
- Keep the earlier elimination of the opaque pink background sprite (R14.18) and the corrected torso/arm coordinates (R14.19).
- Render each **distinct** custom outfit with native geometric art layers:
  - Hoodie: hood shell and rim behind animated head; RGB332-stable charcoal/navy palette; dark outlined torso, drawstrings, ribbed hem, kangaroo pocket, shaded cuffs.
  - Lab Jacket: open dark undershirt, pale lapels/collar, two pocket flaps, breast badge, contrasting longer hem and cyan coat sleeves.
- Preserve real arm endpoint tracking and real fur hands; never draw generic opaque circles onto hands.
- No custom UI sprite, no RGB565 transparency keys, no additional display allocation.

## CYD 3.2 hardware acceptance
1. Flash the R14.20 LAB prerelease complete 4-part image package with SHA-256 checks. This is a LAB release, **not a stable/production promotion**.
2. In landscape SYNTHWAVE, select Black Hoodie. It must actually read as a **dark hoodie**, not olive or flat green, with visible hood/pocket/drawstrings, no pink rectangle, no covering face.
3. Select Neon Lab Jacket. Verify separate lapels, dark shirt opening, badge, coat hem and sleeves, instead of one cyan shirt block.
4. Observe the idle arm swing and head bob for 30 seconds. Sleeves/cuffs should track arms and the hood should track head without shifting or clipping into the face.
5. Cycle original outfits (CLASSIC, TANOOKI, STRIPES etc.); they should remain exactly as approved. Return to the two custom costumes, ensuring no leftover overlays.
6. Rotate to portrait, switch between all outfits, backgrounds and back. No redraw artifacts or boxes.
7. Run/STOP bounded RF profile. R14.17 speech stays Idle-only when off-air and live-category/Transmitting only when active.
8. No major screen flicker or clipping across FULL8/FAST and fallback draw modes.

## Build and release gate
- CI must pass all four jobs: CYD 3.2, ESP32-S3, Waveshare C5, and host fixtures.
- LAB branch additionally runs a **Build and Release FW (LAB)** job only *after* those four jobs pass. It packages CYD 3.2 bootloader/partitions/ota_init/firmware with SHA-256 sums into a uniquely tagged **prerelease**.
- No stable release workflow is triggered; no merge to `main`; no SquachWatch repository edits; no branch deletions.
- A software build pass does NOT establish physical visual acceptance, which remains user-controlled.
