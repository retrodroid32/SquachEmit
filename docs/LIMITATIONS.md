# On-air vs offline and hardware boundaries

**On-air verified** requires a real transmitter, a separately observed
SquachWatch receiver result and both firmware SHAs. CYD 3.2 flashing and UI
tests during v1.1.0 development do not by themselves validate all 172 RF
cases. C5 and S3 have successful CI builds, not complete physical validation.

- **BLE legacy:** Advertised UUIDs, company identifiers, name profiles,
  synthetic iBeacon/Find My and 31-byte Remote ID are programmed using
  NimBLE-Arduino. Scanner behavior, combined Wi-Fi/BLE coexistence and
  actual reception still require physical validation.
- **BLE Classic:** ESP32-S3 has no Bluetooth Classic stack/radio support. The
  `BLE-SKIMMER` case tests the *string classifier*, not Classic device
  discovery or SPP pairing. Use an original ESP32 or appropriate BT radio
  for real Classic tests.
- **Wi-Fi SSID:** A single genuine, password-protected SoftAP tests prefixes,
  not camera hardware or OUI matching. One S3 cannot emit arbitrarily many
  independent concurrently active SSIDs/BSSIDs as a normal SoftAP.
- **Wi-Fi OUIs:** Public v1.1.0 deliberately includes 37 bounded OUI-only
  ordinary SoftAP beacon cases: 10 registered DJI /24 prefixes and 27 UAS
  supplier allocations spanning /24, /28 and /36. Only those explicit cases
  override the default locally administered *temporary on-air* AP identity
  with a registered prefix and synthetic suffix. Many other vendor families
  (Flock, ALPR, cameras, Ring) have offline receiver fixtures only.
  Registered prefixes are spoofable clues, not genuine vendor hardware,
  aircraft, flight or trusted RID. Offline frames cannot measure actual
  RF capture/dwell/RSSI.
- **Wi-Fi attack indicators:** `DEAUTH` is a rate detector; one offline frame
  should not classify as a burst. `EVILTWIN` needs two BSSIDs/security states.
  No deauth floods or arbitrary 802.11 management-frame RF injection is
  implemented. Compare offline traces using SquachWatch native unit tests;
  add controlled two-AP lab testing only when separately authorized.
- **Pwnagotchi:** Self-describing vendor IE is an offline frame, not an on-air
  Pwnagotchi clone.
- **Remote ID:** Legacy BLE packets are synthetic, with a test serial and fixed
  non-operational location fields. Not a certified aviation Remote ID module.
  Wi-Fi Remote ID and NAN require additional host tests or compatible radio
  tooling. Follow local regulatory requirements before transmitting such
  signals, even in a lab.
- **Signal power/accuracy:** BLE/Wi-Fi emitted nominal power does not measure
  receive sensitivity. Actual field power needs calibrated RF equipment,
  attenuation or enclosure, and repeatable geometry. RSSI is not ground-truth
  RF level.
- **Timing:** `TRUTH BEGIN` indicates the API reported a start. It does not
  prove physical airtime or synchronize the two board clocks.
