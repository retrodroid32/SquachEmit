# SquachEmit R14.20 — CYD 3.2 LAB firmware acceptance

## Build-and-release contract
This LAB branch adds an extra CI job named **Build and Release FW (R14.20 LAB)**. It runs **only on pushes to this exact LAB branch**, and only when every existing firmware and host fixture job succeeds. It packages the complete CYD32 four-part flash set, verifies SHA-256 hashes, uploads a ZIP and publishes a uniquely tagged **GitHub prerelease** (never stable/latest). No public installer changes or merge to main.

## Test observations reported from R14.19 photos
- Black Hoodie was olive green and appeared like a plain shirt with small hanging circles on arms; missing recognizable hood and pocket.
- Neon Lab Jacket was a cyan torso block and sleeve circles; no real collar/opening/pockets.
- Original character, animation, sunset and 20 upstream costumes are approved; do not alter them.

## Physical acceptance
- [ ] Black Hoodie: dark charcoal/navy (not olive), visible hood shell behind hair/face, dangling strings, kangaroo pocket, waist hem and moving cuffs.
- [ ] Neon Lab Jacket: two pale lapels, open dark shirt, chest badge, two side pockets and extended coat hem.
- [ ] Both: body and sleeves line up during walking, idle sway, head bob and alert mouth animations; original hands remain furry.
- [ ] No pink rectangle, face overlap, floating hood ring, shoulder gaps, or screen tearing.
- [ ] Rotate portrait/landscape and cycle all original art; original costumes unchanged and no stale coat.
- [ ] R14.17 speech idle-only off-air, real active-category/Transmitting when RF on-air; STOP and 172 cases unaffected.

## Release gate
Compilation alone does not prove pixel-art appearance. Firmware prerelease is marked **LAB, hardware-unverified** until the owner's visual pass. Keep source PR #102 and LAB PR draft until approval.
