# R14.13 CYD 3.2 LAB — landscape timed Squachy speech

Based on user-confirmed working R14.12 five-target portrait calibration. Do not modify touch, FULL8/FAST, case library, or RF timing.

## Test on physical CYD 3.2
1. Only flash after all four CI jobs pass. Use LAB installer with four-part SHA-256 preflight; do not merge or release production.
2. In landscape Home, verify selected test name is still present in the lower PROFILE strip and is NEVER duplicated in the speech box.
3. With RF idle, observe the upper speech area for at least 60 seconds. A short quip should pop in, stay around five seconds, then disappear entirely, exposing the original background. Next appearance should occur after a non-fixed randomized gap rather than on a fixed 6.5s beat. Initial wait ranges 15 to 30 seconds; later gaps range 9 to 22 seconds.
4. Watch bubble entry: a brief ~220ms growth from the center, no permanent blank box, no partial text before full growth, no background flashing, no second Squachy speech bubble.
5. Press RUN on a short synthetic case under controlled lab conditions. A brief context quip may appear, then disappear while the TX countdown and lower selected profile remain readable; manual STOP and automatic 3-second STOP must still work.
6. Enter/leave TESTS, SETTINGS, BACKGROUNDS, and return Home; popup must not cover controls or freeze the sprite. Verify costume animation and left/right background changes.
7. Verify R14.12 portrait five-target calibration still works and portrait touch is aligned. The R14.13 change affects only landscape speech.
8. Collect several CYD32,FRAME,PERF lines: ensure FULL8/FAST FPS/push/heap remain near R14.12 under equivalent scene conditions. The bubble scheduler must not perform direct TFT writes outside the framebuffer.
9. If flicker, failure to hide, false repeated profile, or failed STOP occurs, preserve logs and revert to R14.12 LAB.

## SquachMesh
R14.13 does not enable SquachMesh or change NimBLE behavior. Separate review in docs/SQUACHMESH_PORT_ASSESSMENT.md: requires idle-only radio arbitration, phased peer decode, safe advertising, UI and eventual encrypted messaging.
