# R14.18 — Hardware acceptance log (CYD 3.2 original-body custom outfits)

**Baseline:** R14.17 validated idle/transmitting speech, original backgrounds/costumes, FULL8/FAST draw, 172 RF cases.

**Bug photo:** Black Hoodie and Lab Jacket render as simplified custom mascot on opaque pink 112×111 tile. Original SquachWatch art/background behind that rectangle is otherwise working.

**Expected R14.18:** Both custom garment choices retain the original SquachWatch face/body/pose/animation; transparent scene around the body; no magenta, square, or solid backing; original 20 costumes unaffected. The former scratch RGB332 tile is not allocated on CYD 3.2.

**Test checklist (check on real board):**
- [ ] At Black Hoodie selected, sunset visible through all spaces around character; body and hood style match original SquachWatch character.
- [ ] At Cyan Lab Jacket selected, same result. Inspect poses/bobs and speech bubble showing/hiding for at least 20 seconds.
- [ ] Switch Classic -> original stripes -> another original outfit -> Hoodie -> Jacket: no stale overlay.
- [ ] Cycle Synthwave, Gibson, Digital Rain plus another background; no pink rectangle or display tearing at costume edges.
- [ ] Landscape -> portrait -> landscape; preserve screen navigation, touch, and original art.
- [ ] STOP and bounded RUN work. R14.17 strict idle/category/transmitting speech remains correct.
- [ ] Device shows no new flicker; FAST/standard fallback work as expected.

**CI gate:** native host fixtures, CYD32 firmware, S3 firmware, Waveshare C5 firmware. The resulting CI artifacts are LAB/unverified until hardware checks are marked PASS. No merge, tag, release, or mainline promotion.
