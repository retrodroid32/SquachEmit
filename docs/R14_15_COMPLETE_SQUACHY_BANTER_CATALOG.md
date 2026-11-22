# SquachEmit R14.17 — Squachy speech catalog (130 fixed + 2 event-only)

This is the retained phrase catalog compiled into the CYD 3.2 R14.17 LAB firmware. The words are humor, NOT receiver evidence. The library still has **172 real cases in 10 device categories**, but **only two kinds of speech are currently eligible**: when RF is OFF, 100% of bubbles are drawn from the **IDLE** bank, regardless of selected file, suite gap, playlist, Wi-Fi startup or failure; when RF is ON, 60% come from the **actual transmitting file's top-level device category** and 40% from the **TRANSMITTING** bank. An invalid on-air file ID falls back to TRANSMITTING only. NO other banks or event-only jokes are automatically displayed. This overrides R14.15's former 60/25/10/5 mix and disables rare/Easter/carrier/playlist/suite/failure/starting selection without deleting their texts. Each line is chosen ONCE at popup appearance and keeps five-second visibility and randomized gaps; active file changes trigger a fresh applicable popup. Long phrases wrap at whole words in the centered bubble. These dynamic phrases show on landscape Home only.

**Inactive banks remain preserved for future use:** Ring has no corresponding scenario; BLE/Wi-Fi/AirTag/Remote ID subtopic banks, rare/Easter humor and operational starting/failure/suite/playlist banks are retained but are NOT selected by R14.17's speech bubble. Wi-Fi diagnostic errors remain in serial diagnostics, not in off-air speech bubbles.

## Idle / waiting (11)

- `READY FOR A SIGNAL CHECK!`
- `PICK A PROFILE. I WILL EMIT!`
- `172 TESTS. ONE LITTLE SQUACH.`
- `NO RF UNTIL YOU PRESS RUN.`
- `LET US TEST THAT DETECTOR!`
- `WAKE ME WHEN WE TRANSMIT!`
- `JUST A SQUACH WITH A DREAM.`
- `I SMELL RADIO WAVES!`
- `IS THIS THING ON?`
- `MY ANTENNA IS ITCHY.`
- `HERE WATCHY WATCHY!`

## Active transmission (9)

- `SENDING SYNTHETIC SIGNAL...`
- `CHECK YOUR RECEIVER NOW!`
- `TEST SIGNAL IS ON THE AIR!`
- `PEW PEW! RADIO EDITION!`
- `I'M NOT SAYING IT'S ALIENS...`
- `BEHOLD! INVISIBLE NOISE!`
- `SQUACHY GOES BRRRR!`
- `SCIENCE! PROBABLY.`
- `SETTING PHASERS TO ANNOY!`

## Wi-Fi starting (5)

- `STARTING WIFI TRANSMISSION`
- `SUMMONING THE WIFI SPIRITS!`
- `HOLD MY ANTENNA!`
- `CONNECTING THE DOTS...`
- `WIFI WIZARDRY IN PROGRESS!`

## Wi-Fi startup failure (6)

- `WIFI START FAILED`
- `WELL, THAT WAS EMBARRASSING.`
- `THE GREMLINS DID IT!`
- `TASK FAILED SUCCESSFULLY!`
- `HAVE YOU TRIED MORE MAGIC?`
- `MY ANTENNA NEEDS THERAPY.`

## Favorites playlist (5)

- `FAVORITES PLAYLIST`
- `DJ SQUACH IS IN THE HOUSE!`
- `NOW SPINNING: RADIO WAVES!`
- `THE HITS JUST KEEP COMING!`
- `THIS MIX IS FIRE! (NOT RF)`

## Test suite (7)

- `TEST SUITE RUNNING`
- `RELEASE THE TEST GOBLINS!`
- `LET THE CHAOS BEGIN!`
- `ONE TEST TO RULE THEM ALL!`
- `I WAS BUILT FOR THIS. MAYBE.`
- `THE SCIENCE IS SCIENCING!`
- `SENDING EVERYTHING BUT THE KITCHEN SINK!`

## AXON (6)

- `CAMERA READY? ACTION!`
- `I DEMAND A BODYCAM!`
- `SMILE! YOU'RE ON SQUACH CAM!`
- `SQUACHY, TAKE ONE!`
- `BODYCAM? I NEED A FURCAM!`
- `ROLLING! BUT ONLY IN TEST MODE.`

## DRONES / REMOTE ID (7)

- `I BELIEVE I CAN WIFI!`
- `I HAVE THE HIGH GROUND!`
- `HOUSTON, WE HAVE A SQUACH!`
- `FLYING WITHOUT LEAVING HOME!`
- `MY PROPS ARE IMAGINARY!`
- `SQUACH CONTROL TO MAJOR TOM.`
- `ALTITUDE: EXTREMELY SQUACH.`

## FLOCK (6)

- `FLOCK AROUND AND FIND OUT!`
- `WHO LET THE BIRDS OUT?`
- `NO BIRDS WERE HARMED!`
- `THE PENGUINS ARE UP TO SOMETHING!`
- `I'M JUST HERE FOR THE BIRD SEED.`
- `FLOCK YEAH! TEST MODE!`

## GENERAL / CONTROLS (6)

- `THIS ONE'S A CONTROL GROUP!`
- `FALSE ALARM? THAT'S A PASS!`
- `BORING SIGNAL. IMPORTANT TEST.`
- `I'M PAID IN DEBUG LOGS.`
- `NO SPOOFED MAGIC TODAY!`
- `THE CONTROL GROUP HAS SPOKEN.`

## HACKER DEVICES (7)

- `FLIPPER? I HARDLY KNOW HER!`
- `HACKERMAN HAS ENTERED THE CHAT!`
- `MY HOODIE HAS ROOT ACCESS.`
- `PINEAPPLE ON THE RADIO MENU?`
- `THIS IS A LAB, NOT A HEIST!`
- `SQUACH.EXE HAS STOPPED CARING.`
- `HACK THE PLANET!`

## IBEACON (6)

- `IBEACON, THEREFORE I AM.`
- `BEACON OF HOPE? MORE LIKE BLE.`
- `MY BEACON BRINGS ALL THE BARS.`
- `YOU HAD ME AT ADVERTISING.`
- `SMALL PACKET. BIG PERSONALITY.`
- `I HAVE A MAJOR IDENTITY CRISIS.`

## RAVEN (6)

- `NEVERMORE... UNLESS IT'S A RETEST.`
- `QUOTH THE RAVEN: BLE!`
- `SQUACHY HEARD A RAVEN CALL!`
- `CROAK? NO, THAT'S MY DEBUGGER.`
- `BIRDS OF A FREQUENCY FLOCK.`
- `EDGAR ALLAN RADIO WAVES!`

## SERIAL MODULES (6)

- `AT+SQUACH=1`
- `MY BAUD RATE HAS FEELINGS.`
- `SERIAL KILLER. OF BUGS!`
- `HELLO FROM THE UART SIDE!`
- `THE MODULE SAYS HELLO!`
- `I SPEAK FLUENT AT COMMANDS.`

## SMART GLASSES (6)

- `RAY-BANS? I WEAR FUR SHADES.`
- `LOOK, MA! HANDS-FREE RF!`
- `SQUACHY HAS MAIN CHARACTER VISION.`
- `I SEE WHAT YOU TRANSMITTED.`
- `MY SHADES HAVE DEBUG MODE.`
- `SMILE FOR MY IMAGINARY GLASSES!`

## TRACKERS (6)

- `BEEP BEEP! WHO'S THERE?`
- `TRACK ME? CATCH ME FIRST!`
- `SQUACHY KNOWS WHERE HE IS.`
- `THIS ISN'T A TREASURE MAP.`
- `I LOST MY OTHER ANTENNA!`
- `DON'T TAG MY FUR, PLEASE.`

## AirTag subset of Trackers (3)

- `I'M NOT LOST. YOU ARE!`
- `WHERE'S MY OTHER SOCK?`
- `FOLLOW THE BEEP!`

## BLE transport (3)

- `LOW ENERGY. HIGH DRAMA.`
- `BLUETOOTH AND CHILL?`
- `BLE ME AWAY!`

## Wi-Fi transport (3)

- `BEACON AND EGGS, ANYONE?`
- `SSID? MORE LIKE SOS!`
- `THE WIFI IS STRONG WITH ME.`

## Remote ID transport (3)

- `IDENTITY CRISIS INCOMING!`
- `HELLO! I AM A TEST DRONE!`
- `MY ID IS COMPLICATED.`

## Ring — dormant (no test cases) (3)

- `DING DONG! IT'S SQUACH!`
- `SPECIAL DELIVERY: RF!`
- `YOUR DOORBELL KNOWS TOO MUCH.`

## Rare sarcastic (5)

- `I KNOW WHAT YOU DID LAST WIFI.`
- `I PUT THE SQUACH IN SQUELCH.`
- `MY OTHER ANTENNA IS A YAGI.`
- `I CAN HEAR YOUR TOASTER.`
- `THIS IS FINE. PROBABLY.`

## Very rare Easter eggs (5)

- `THE GOVERNMENT STOLE MY WIFI.`
- `ERROR 404: COMMON SENSE.`
- `I'M 90% FUR, 10% RF.`
- `DON'T TELL THE FCC ABOUT ME!`
- `SHHH... I'M DOING SCIENCE.`

## Event-based extra lines

- `AGAIN? SERIOUSLY?` — when the user taps RUN three times within approximately 45 seconds, next available active-TX popup.
- `DID YOU FORGET ABOUT ME?` — after two idle minutes without a touchscreen press, on the next idle popup.

**Both event-only messages are preserved in source but are disabled for display in R14.17.** Neither changes timers, RF packets, START/STOP, or detection results. Ring remains intentionally inactive.
