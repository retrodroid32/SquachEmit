# R14.17 LAB — strictly state-locked Squachy speech

## User-requested acceptance policy
- **IDLE / no RF**: the speech bubble draws exclusively from the 11 IDLE lines. This includes a selected-but-not-running file, failed/starting Wi-Fi, suite/playlist cooldown gaps, and after STOP.
- **ON AIR**: 60% of new bubbles use the actual transmitted file's one of 10 device-category banks, 40% use the nine generic TRANSMITTING lines.
- **On-air active case changes**: do not keep the old device category's bubble, even if no UI tick occurs during the idle transition.
- **Other banks disabled** for selection: starting, failure, favorites, suite, BLE/Wi-Fi/AirTag/RemoteID carrier subtopics, rare, Easter, Ring, repeated RUN and long-idle event quips. Strings remain present in the catalog but must not appear automatically.
- Never use the highlighted menu file in place of the active transmitter's scenario.

## Build gates
1. Four CI jobs green: native host fixtures, CYD 3.2 ST7798, S3 and Waveshare C5.
2. Verify the CI run exports the correct firmware artifact, then do four-part SHA-256 LAB flashing preflight; do not create a public release/tag or merge.
3. Verify CYD 3.2 R14.12 portrait touch, landscape controls, smooth FULL8/FAST animation and R14.14 centered bubble / TX icon.

## CYD 3.2 physical acceptance
1. Leave TX idle with FLOCK file selected: 5+ popups show **IDLE ONLY** (e.g. HERE WATCHY WATCHY!, IS THIS THING ON?). No bird jokes.
2. Select HACKER profile but do not RUN: still **IDLE ONLY**. No HACK THE PLANET! while off air.
3. RUN HACKER for a bounded 20-30 seconds: only HACKER or TRANSMITTING text eligible. In particular HACK THE PLANET! and SETTING PHASERS TO ANNOY! are now allowed.
4. STOP: active-category and transmitting bubble disappears immediately; later idle popup must only use IDLE.
5. RUN FLOCK: only FLOCK or TRANSMITTING. No hacker, drone, Apple, Easter, or idle phrases while active.
6. Start short attended test suite / favorites playlist: during RF only current transmitting file's category or TRANSMITTING; in the gaps IDLE only. No TEST SUITE RUNNING or DJ SQUACH lines.
7. Wi-Fi starting/failure: off-air bubble stays IDLE, error details remain in serial diagnostics.
8. Confirm selecting another menu file while a test is on air does not switch its spoken category. Run, STOP, countdown and radio timing must be unaffected.

Because popup timing is random (five-second display, randomized gaps), one individual phrase need not appear during a short RF run; judge the allowed **bank**, not the presence of a particular quote.

## Project isolation
SquachEmit ONLY. No SquachWatch changes. Preserve PR branches; do not merge, tag or issue a production release without explicit approval.
