# Drone Remote ID LAB profiles — v0.4.0 (unmerged)

This feature tests how SquachWatch classifies **synthetic** Remote ID, not
whether any genuine DJI, Autel or Parrot radio was reproduced. The model
names are human-readable **lab labels** in Self-ID; all Basic IDs are
obviously fictitious SQE-prefixed serial strings. They are not manufacturer
authenticators, DJI OcuSync, Autel proprietary RF, or Parrot flight links.

## Firmware, on air only after selecting RUN

Ten BLE4 legacy test variants: DJI Mini, Air, Mavic, Phantom, Matrice;
Autel EVO II, EVO Max, EVO Lite; Parrot ANAFI, ANAFI USA.

Additional cases:
- RID-GENERIC-BLE4: generic Basic/Location/System/Self-ID rotation.
- RID-WIFI-BEACON: 2.4 GHz channel-6 WPA2 SoftAP, ASTM Remote ID
  vendor-specific information element in its ordinary beacon.
- RID-WIFI-PARROT: same, with Parrot-style vendor OUI. This is **not** a
  Parrot radio or authentic serial, only the OpenDroneID IE structure.
- RID-NEG-APP: BLE 0xFFFA with incorrect application code; expect NONE.
- RID-NEG-SHORT: BLE 0xFFFA with insufficient service-data bytes; expect NONE.
- RID-NO-OPERATOR: valid Basic+Location (but no System); expect DRONE, and
  verify no operator coordinates are displayed for a fresh record.
- RID-STALE-LOC: valid Remote ID with old location TimeStamp=0.1 seconds
  after the hour; expect DRONE. A successful detection is NOT proof of
  timestamp freshness enforcement.
- RID-BAD-SERIAL: invalid IDType 0xF in otherwise valid BLE transport;
  expect DRONE until receiver performs stricter ID validation.

The existing BLE-DRONE 3-message fixture remains unchanged. New valid
profiles add Self-ID as a fourth rotating 25-byte message (except the
operator-absent case, which deliberately rotates two). Each case is limited
to 3–30 seconds; default 12, with no RF emissions on boot. Ordinary Wi-Fi
vendor IE is configured via esp_wifi_set_vendor_ie, cleared on STOP/timeout.

## Offline-only Bluetooth 5 / NAN and failure modes

To avoid claiming unsupported RF, BLE5 extended/coded PHY and Wi-Fi NAN
exist as a deterministic fixture generator only. Run:

    python tools/generate_drone_vectors.py --output tests/drone_vectors.json

It includes positive and negative BLE4/BLE5/Wi-Fi beacon/NAN examples,
wrong application codes, invalid IDType, truncated message packs, missing
operator, and old location timestamp. These bytes DO NOT transmit over BLE5
or NAN. A BLE5 advertising-data payload is not proof of an actual coded-PHY
signal, and generic Wi-Fi NAN public-action payloads are not produced by a
normal SoftAP.

The on-air Wi-Fi beacon is only 2.4 GHz. Hardware validation is required
for both C5 and S3; final receiver scoring is separate for CYD 2.8/3.2.
The standard CYDs cannot receive BLE5 coded PHY; a separate receiver would
be required for live coded-PHY validation.

## Required receiver acceptance

1. Verify silent boot, 49 cases in LIST, normal C5 touch navigation,
   STOP and automatic timeouts.
2. Verify DJI, Autel and Parrot profiles all classify DRONE on both CYDs.
3. Check synthetic Basic ID, synthetic aircraft coordinates and the
   distinct synthetic operator coordinates. Model name Self-ID is NOT
   currently rendered by SquachWatch's legacy decoder.
4. Verify RID-NEG-APP and RID-NEG-SHORT never classify DRONE.
5. Verify RID-NO-OPERATOR still classifies DRONE without operator location
   after clearing stale display state; stale-time is not validated by
   the existing receiver.
6. Verify standard and Parrot-style Wi-Fi beacon vendor IE are seen as
   DRONE through the SquachWatch Wi-Fi promiscuous receiver path.
7. Record actual results separately for CYD 2.8 and 3.2 using the
   tools/score_trials.py pipeline. Do not infer any PASS from CI alone.

No production release, merge, or tag is authorized by this change.
