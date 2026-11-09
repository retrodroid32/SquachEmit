# SquachEmit controlled RF and protocol LAB expansion

Working branch: lab/rf-fixtures-reporting-20261109 (unmerged, NOT hardware tested).

## Sources and scope

- ESP32Marauder MIT: RemoteIdDecoder.cpp, IBeacon.h and their native unit
  tests informed independent offline-only Python validation. No RF injection
  primitives or offensive functionality were ported.
- ESP32DualBandWardriver MIT: channel-dwell observability and structured CI
  reporting concepts. SquachEmit uses fixed channels for reproducibility.
- WiFi Pineapple Pager community: CSV/JSON and multi-run reporting concepts
  only; no Linux payload scripts or proprietary DuckyScript were copied.

No deauth floods, credential interception, arbitrary 802.11 management
frame injection, jamming or unattended transmissions were added.

## New bounded on-air test cases

New Wi-Fi channel variants: WIFI-AXON-CH6, WIFI-AXON-CH11,
WIFI-FLOCK-CH6, WIFI-NEG-CH6, WIFI-NEG-CH11.
Legacy Wi-Fi cases still use channel 1.
BLE additions: BLE-IBEACON-V2, BLE-NEG-IBEACON-SHORT,
BLE-NEG-DRONE-UUID and BLE-NEG-META-ADJ.

All cases transmit only after an explicit action, for 3–30 seconds.
Status TRUTH records preserve the original fields plus carrier and channel.
All tests use ordinary 2.4GHz SoftAP or BLE advertising; 5GHz is intentionally
excluded because classic ESP32 CYD receivers do not support 5GHz Wi-Fi.

## Required physical verification

1. Flash latest unverified C5 or S3 lab image with the SquachEmit installer.
2. Confirm C5 LCD/touch, 31 LIST entries, RF silence on boot, STOP and timeout.
3. Run a positive channel case, then its matching benign negative, separately
   against CYD-2.8 and CYD-3.2; allow detector dwell/debounce to settle.
4. Use actual receiver serial/SD/GUI observations. A transmitter TRUTH BEGIN
   line does not prove reception or the correct SquachWatch detection.
5. Fill tests/trials.csv: run_id,receiver,receiver_sha,emitter_sha,id,
   emission (ok/failed), window (complete/incomplete), observed (type or NONE),
   optional latency_ms, notes. A blank observed value means UNTESTED.
6. Score trials using:
   python tools/score_trials.py tests/trials.csv --output trials-report.md --json trials-report.json

The per-CYD report includes TP/TN/FP/FN, wrong-class, precision, recall,
specificity, median and P95 first-hit latency. Failed emissions and incomplete
listening windows are excluded. Untested does not mean a miss.

## Host tests / limits

python -m unittest discover -s tests -v
python tools/generate_vectors.py
python tools/score_trials.py tests/trials.csv --output /tmp/trials.md

Protocol reference checks cover BLE Remote ID, Wi-Fi Remote ID vendor IE,
Wi-Fi NAN service hash, iBeacon fields and malformed/truncated inputs.
They establish format consistency only. True accuracy still requires
independent radio reception and SquachWatch hardware observations.
