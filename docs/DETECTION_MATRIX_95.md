# SquachEmit: SquachWatch detection matrix (95 on-air cases)

Source comparison: SquachWatch-CYD `src/signatures.cpp` and
`docs/LAB_V141_SOI_FULL_HWTEST.md`, inspected 2026-11-10.

The 49-case baseline adds **46** opt-in scenarios, now **95** synthetic
RF cases. It includes Raven 0x3200–0x3500, Tile 0xFEEC, Flipper
0x3082/0x3083/name, Meta/Luxottica/Snap company IDs, Flock accessory
names and SSID cases, generic serial module names, Axon AB3/AB4/AXON
SSIDs, Hak5 Pineapple Pager exact SSIDs and negatives, Wi-Fi channels
3/9/11, and Remote ID beacon channels 1/11.

## Important interpretation

These transmit ordinary BLE legacy advertisements and ESP32 Wi-Fi SoftAP
beacons only, with per-case locally-administered synthetic MAC addresses.
**Detection of a marker does not authenticate a product or imply criminal
activity.** Generic serial modules are not confirmed skimmers. Meta and
Snap company IDs are not proof of smart glasses. The generic Eddystone
UUID 0xFEAA alone is *not* Google FMDN evidence and its expected result
was corrected to NONE. Negative cases test specificity, not merely hit
counts.

## Other SquachWatch detections: not supported by on-air transmitter

- Wi-Fi vendor OUI (ALPR/Motorola/Genetec, Ring, cameras, DJI/Autel and
  more): synthetic LAA MAC addresses cannot validly prove an IEEE OUI.
  Use offline frames and receiver fixture tests, **not forged vendor MACs**.
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
2. Check banner reports 95 cases, no touch/display/color regression.
3. Exercise representative BLE, Wi-Fi and Remote ID positives and negatives
   against a separate SquachWatch 2.8 or 3.2 receiver.
4. Check per-case MACs, STOP and expiry, roundtrip results and frame rate.
5. Run SUITE only when attended: 95 tests x 12s plus gaps can take
   approximately 23 minutes; interrupt with STOP as needed.
6. Physical RF validation is **not** supplied by CI. Do not merge firmware
   unless explicitly approved by the owner.

No SquachWatch receiver firmware changes are included here.
