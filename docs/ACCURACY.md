# Repeatable SquachWatch accuracy test protocol

## Test setup

Use each of the two separately verified SquachWatch variants (CYD 2.8 ST7789
and CYD 3.2 ST7798) with a known firmware SHA/version. Record firmware version,
build variant, scan window, automatic alert settings, and device distance.
Disable per-device snooze or WATCH rules that might hide an otherwise valid
match. Set detection filters to include all targets; iBeacon starts disabled.

The emitter's UART messages tell you *what it intended to transmit*. Check
that SquachWatch actually received it. The S3 runs one case at a time because
BLE and Wi-Fi share a physical RF chain; keep power and antenna orientation
constant.

## Per-case procedure

1. Start SquachWatch SCAN/normal surveillance and let old detections clear.
2. Confirm the environment is free of unexpected matches to the case.
3. On the S3 run `RUN <CASE-ID> 12`. Record `TRUTH BEGIN` timestamp.
4. Watch SquachWatch until test end. Record actual category or `NONE` if no
   matching event occurs despite a complete listening window. A valid match
   in LOG counts even when Alert Rules intentionally suppress a popup.
5. Note time to first classified hit, confidence, evidence code, number of
   detections, and any wrong-class alert. If scanner was in a sleep/dwell gap,
   do not silently count the trial as an RF negative.
6. Wait for the S3 to return to idle; for tracker tests, allow debounce and
   flood suppression state to decay or use fresh controlled test conditions.
7. Run the matching negative control; repeat each positive at least 10 times
   and each negative at least 10 times in the same physical configuration.

Use `tests/observations.csv` for one full pass. Save copies with run IDs and
board IDs for replicated passes. `observed` is the SquachWatch type string,
`NONE` for confirmed absence, **blank if untested**. Don't put "FAIL" in
`observed`: it is not a recognized type.

## Acceptance targets (proposed, not measured results)

- At a fixed close-range position, target >= 95% detection rate per positive
  signature over at least 20 valid exposure windows before calling it stable.
- Target zero false positives on negative controls; separately quantify the
  false positive rate with a larger, real-world benign-device sample.
- Target median < 3 seconds to first hit under normal scan duty, then capture
  p95. These are goals, not claims about the current firmware.
- For BLE Remote ID, validate not only `DRONE` but the synthetic serial,
  QUAD/UA type, aircraft coordinates, altitude, and *different* operator
  coordinates. Start with the same sequence as `test/emitter_roundtrip_test.cpp`.
- Test the three evidence paths (name, company ID, UUID) independently to
  localize failures. Manufacturer ID alone is not proof of product identity.
- Repeat on both CYDs; regressions specific to 3.2 must not be hidden by
  combined success metrics.

## Scoring interpretation

`tools/analyze.py` reports TP, TN, FP, FN, precision, recall and specificity.
A wrong class counts as both missed target (FN) and spurious other class (FP).
Unfilled observations are UNTESTED and excluded from denominators. Raw
confidence percentage shown by SquachWatch is a heuristic, NOT empirical
accuracy; actual sensitivity and false alarm rates must be measured here.

If the emitter fails `radio_start_failed` or a Remote ID rotation fails,
exclude that window from detection accuracy; it's an emission failure, not a
SquachWatch miss. Verify radio delivery with a second independent receiver
when measuring protocol fidelity.

## Known risk of inference

A manufactured advertisement matching `GOOGLE_TAG` only proves SquachWatch
recognizes UUID `0xFEAA`; Google's service is shared with Eddystone beacons.
Similarly, a spoofed company ID or name is not a real Flock/Meta/Flipper
product. No set of synthetic positives alone can estimate detection accuracy
on real surveillance devices. Include physical reference devices/captures
when available, and report them as a separate real-world test cohort.
