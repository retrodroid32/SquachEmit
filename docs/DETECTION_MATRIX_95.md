# SquachEmit: historical SquachWatch detection matrix (95 on-air cases)

**Historical v1.0.0 snapshot; not current v1.1.0 behavior.**
Current public firmware has 172 cases, including **37 explicit on-air
manufacturer-OUI Wi-Fi beacon tests** (10 DJI + 27 other UAS-related /24,
/28 and /36 allocations). Other cases use locally administered Wi-Fi or
static-random BLE identities. An OUI is only a spoofable vendor hint,
never evidence of real equipment, exact drone model, flight or Remote ID.

Source comparison: SquachWatch-CYD `src/signatures.cpp` and
`docs/LAB_V141_SOI_FULL_HWTEST.md`, inspected 2026-11-10.

The 49-case baseline adds **46** opt-in scenarios, now **95** synthetic
RF cases. It includes Raven 0x3200–0x3500, Tile 0xFEEC, Flipper
0x3082/0x3083/name, Meta/Luxottica/Snap company IDs, Flock accessory
names and SSID cases, generic serial module names, Axon AB3/AB4/AXON
SSIDs, Hak5 Pineapple Pager exact SSIDs and negatives, Wi-Fi channels
3/9/11, and Remote ID beacon channels 1/11.

## Important interpretation

The original 95 cases transmitted ordinary BLE legacy advertisements and
Wi-Fi SoftAP beacons with synthetic locally administered MAC addresses.
The **172-case public v1.1.0** has 37 exceptions noted above.
**Detection of a marker does not authenticate a product or imply criminal
activity.** Generic serial modules are not confirmed skimmers. Meta and
Snap company IDs are not proof of smart glasses. The generic Eddystone
UUID 0xFEAA alone is *not* Google FMDN evidence and its expected result
was corrected to NONE. Negative cases test specificity, not merely hit
counts.

## Other SquachWatch detections: not supported by on-air transmitter

- Historical v1.0.0 did not contain on-air manufacturer-OUI cases.
  Current v1.1.0 explicitly emits only 37 named UAS/vendor Wi-Fi beacons
  with registered prefixes and synthetic suffixes for OUI-only tests;
  other vendor families remain offline-only fixtures. These prefixes
  do not authenticate any manufacturer or aircraft.
- Pwnagotchi vendor-IE handshake counter, NAN Remote ID, Wi-Fi deauth
  threshold and evil-twin correlation require different 802.11 frames or
  multiple senders/observations; existing test/offline_vectors.json covers
  representative cases. No deauth emission or management-frame injection.
- BLE5 coded/extended PHY, BT Classic inquiry/SPP, full Apple Find My
  state transitions, advanced Google FMDN service payloads and full
  OpenDroneID message pack variations are not represented by the current
  single classic ESP32 BLE legacy + Wi-Fi SoftAP path.
- Detecting suspected Flock sources through wildcard probe behavior must
  be evaluated offline; generic Qualcomm OUIs must not be forged or
  interpreted as proof of Flock.

## Hardware gate

1. Use the web installer `?lab=1` on the CYD 3.2 ST7798 board.
2. Check 95 cases only on the archived v1.0.0 baseline, or 172 on
   current v1.1.0, including its smooth-touch and TX-time editor.
3. Exercise representative BLE, Wi-Fi and Remote ID positives and negatives
   against a separate SquachWatch 2.8 or 3.2 receiver.
4. Check per-case MACs, STOP and expiry, roundtrip results and frame rate.
5. Run SUITE only when attended; the case count and per-case duration
   differ by release. Interrupt with STOP as needed.
6. Physical RF validation is **not** supplied by CI. Do not merge firmware
   unless explicitly approved by the owner.

No SquachWatch receiver firmware changes are included here.
