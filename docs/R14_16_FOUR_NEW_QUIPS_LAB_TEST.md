# R14.16 LAB — Four additional Squachy blurbs

This LAB candidate adds the four requested speech phrases on top of R14.15 (126 fixed + 2 event-only), preserving all existing words and behaviors.

- IDLE: `HERE WATCHY WATCHY!`
- HACKER: `HACK THE PLANET!`
- TRANSMITTING: `SETTING PHASERS TO ANNOY!`
- TEST SUITE: `SENDING EVERYTHING BUT THE KITCHEN SINK!`

The user clarified that **EVERYTHING** is the correct spelling. Do not ship the misspelled `EVERYTING` variant.

## Build and physical acceptance
1. All four CI jobs pass, including host C++ phrase tests and CYD 3.2 firmware. All 172 RF cases remain unchanged.
2. Run four-part SHA-256 LAB flashing preflight. Do not release production firmware, merge PRs or tag.
3. Leave the landscape Home screen idle. The IDLE phrase can be selected by the existing 60/25/10/5 weighted popup system.
4. Select a Hacker test case and observe occasional HACK THE PLANET! bubbles.
5. Run a short bounded synthetic case and check the Transmitting phrase selection. STOP and timeout must remain reliable.
6. Use a short attended test suite and confirm the corrected EVERYTHING phrase can be shown. Long phrase wraps at whole words inside the centered speech bubble.
7. Check R14.14 radio icon, background animation and text centering, R14.12 portrait touch mapping, and no startup/heap regressions.
8. Full catalog: `docs/R14_15_COMPLETE_SQUACHY_BANTER_CATALOG.md` (updated for R14.16).

The new phrases may not appear on every popup because timing and random selection are unchanged. No RF or detection protocol functionality is changed by this patch.
