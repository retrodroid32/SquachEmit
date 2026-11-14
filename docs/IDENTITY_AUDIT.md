# SquachEmit v1.1.0 — On-air identity audit

## Ground truth from the actual firmware

SquachEmit's 172-case catalog begins with deterministic, chip/case-derived
Wi-Fi locally administered unicast MACs (`02:`) and static-random BLE MACs.
No path modifies the chip's permanent eFuse identity.

There are **37 deliberate exceptions**, all ordinary, explicitly selected
2.4 GHz Wi-Fi SoftAP beacon scenarios:

- 10 `OUI-DJI-*` cases use exact registered DJI **24-bit** MA-L blocks.
- 27 additional `OUI-*` UAS/vendor cases use precise registered **24-bit,
  28-bit or 36-bit** prefixes.
- In those exceptions only, the temporary SoftAP MAC is replaced with the
  selected registered prefix and a *synthetic* remaining suffix. Each case
  has an `SQE-LAB-...` SSID and a `TEST,OUI_ONLY,...,NO_REMOTE_ID`
  serial truth marker.
- None of these 37 beacons constitutes authentic vendor hardware, an
  identified flying aircraft, a validated model, or an aviation Remote ID
  broadcast. Manufacturer-OUI lookup is trivially spoofable.

The other 135 scenarios retain their default local Wi-Fi or static-random
BLE identities. No emitted attack frames or unsolicited transmissions
are added by this audit.

## Why this documentation was corrected

The first 95-case public version did not contain the 37 on-air OUI
exceptions. Later firmware added deliberately selected registered-prefix
test cases, while historical documents and one DJI registry regression
test still said all on-air MACs were locally administered. That was
incorrect for public v1.1.0.

PR #35 corrects those claims and adds native code coverage. **It does
not alter transmitted MAC derivation, RF behavior, scenarios, flash
images, or release tags.** Do not conflate source-code matching with
on-air reception.

## Verification scope

Run `python -m unittest discover -s tests -v`. The new audit compiles
the actual C++ firmware prefix helpers and checks all 37 selected
scenarios against the 172-case catalog, the DJI and extended vendor
prefixes, and the bit boundaries of 24/28/36-bit allocations. Existing
identity regression expands from 49 to all 172 default cases.

A physical receiver test still needs a separately observed SquachWatch
event, both firmware SHAs, a complete listening window, and actual
received classification. `TRUTH BEGIN` indicates emitter intention,
not proof of transmission or reception. Test registered-prefix cases
only in an authorized, controlled setting where the deliberately
spoofable vendor identity cannot cause confusion.
