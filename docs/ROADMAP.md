# Roadmap (no work merged into SquachWatch)

- v0.1: bounded BLE and WPA2 SoftAP scenarios, offline frame fixture catalog,
  truth markers and manual scoring tool.
- v0.2: real S3 compile log and physical board verification; record independent
  2.8/3.2 detection matrices and correct emission bugs, then publish firmware
  artifacts marked hardware-tested.
- v0.3: optional small local display/button selector on a known ESP32-S3 board;
  no radio activity at boot, no automatic resuming.
- v0.4: calibration fixture modes (fixed 100/250/500 ms intervals; controlled
  single active signature; receiver missed-dwell analysis), with calibrated
  external attenuation when available.
- v0.5: CI correlation against SquachWatch source's unit tests and a
  signed/hashed signature-catalog snapshot; fail builds when upstream rules
  have changed and scenarios have not been reviewed.
- Later: external genuine Bluetooth Classic module/board and two independent
  access points for controlled Evil Twin validation, without including an
  over-the-air deauth attack engine.
