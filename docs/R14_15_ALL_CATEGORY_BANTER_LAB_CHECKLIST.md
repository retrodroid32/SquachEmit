# R14.15 LAB — 126 fixed Squachy phrases for ALL ten real categories

This is a LAB-only, descriptive CYD 3.2 firmware candidate. No production merge, tag, release, SquachWatch repo changes, or added RF signal class.

## Tests to perform on the physical CYD 3.2
1. Flash only after 4/4 CI green and a four-part SHA-256 manifest preflight. Keep R14.12 / R14.14 available for rollback.
2. Landscape Home: let randomized dialogue appear. It must show only one bubble at a time, about 5 seconds long, no duplicate selected-profile text, centered with 13px margins. Entire bubble disappears between appearances.
3. Pick scenarios from EACH of ten real menu categories: AXON; DRONES/REMOTE ID; FLOCK; GENERAL/CONTROLS; HACKER DEVICES; IBEACON; RAVEN; SERIAL MODULES; SMART GLASSES; TRACKERS. Return Home and wait several popups per category. Category-specific humor should appear some of the time, while general/rare lines keep variety.
4. Test subtopics with an AirTag, generic BLE, Wi-Fi and Remote ID profile. All phrases are jokes, not actual detection claims. RING jokes in catalog are intentionally unused: Ring does not exist in the present 172-case emitter library.
5. Check longer messages such as "I'M NOT SAYING IT'S ALIENS...", "SQUACHY HAS MAIN CHARACTER VISION.", and "THE GOVERNMENT STOLE MY WIFI." No clipping, truncated words, overlaps or ghost rectangles. Long speech should wrap at a whole word into an automatically taller centered bubble.
6. Leave Home idle and untouched for over two minutes. Eventually observe a special "DID YOU FORGET ABOUT ME?" line. Touching resets idle timer.
7. Press RUN on three separate short, attended cases within 45 seconds, STOP each safely; look for "AGAIN? SERIOUSLY?" on next active-tx popup. This joke must not start, stop, extend or accelerate RF.
8. Trigger an ordinary Wi-Fi start failure only if one happens naturally. The actual Wi-Fi failure stage must remain visible on second line; text selection must never misrepresent a failed test as a confirmed reception.
9. Check R14.14 new radio beacon idle cyan / active green; R14.12 five-target portrait touch, menu, backgrounds, costumes, speech pop timing and lower selected PROFILE remain intact.
10. RF tests: 3-second BLE and Wi-Fi case with manual STOP and auto timeout. Verify SquachWatch classifications separately; no fabricated receiver evidence.
11. Check PUSH STATUS and CYD32,FRAME,PERF, confirm FULL8/FAST under equivalent conditions, no memory drop, boot loop, clipping or frame flashing.

Full all-phrase inventory: docs/R14_15_COMPLETE_SQUACHY_BANTER_CATALOG.md.
Hardware verification is not implied by CI passing. RF remains limited to existing bounded synthetic test scenarios.
