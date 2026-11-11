# Extended UAS and Avionics MAC-prefix LAB — 172 cases

Draft CYD 3.2 firmware **only**. Adds 27 distinct, short-duration, user-triggered
2.4 GHz Wi-Fi OUI-only beacons to the existing 145-case SquachEmit library.
These do not transmit authentic aircraft communications or ASTM/FAA Remote ID.

## Precisely registered assignments (not aircraft model signatures)

| Test case | IEEE allocation | Registrant | Expected legacy OUI heuristic |
|---|---|---|---|
| `OUI-AUTEL-18D7936` | `18D7936` (28 bits) | Autel Intelligent | DRONE (vendor heuristic only) |
| `OUI-AUTEL-EC5BCDE` | `EC5BCDE` (28 bits) | Autel Robotics USA | DRONE (vendor heuristic only) |
| `OUI-PARROT-00121C` | `00121C` (24 bits) | Parrot SA | DRONE (vendor heuristic only) |
| `OUI-PARROT-00267E` | `00267E` (24 bits) | Parrot SA | DRONE (vendor heuristic only) |
| `OUI-PARROT-9003B7` | `9003B7` (24 bits) | Parrot SA | DRONE (vendor heuristic only) |
| `OUI-PARROT-903AE6` | `903AE6` (24 bits) | Parrot SA | DRONE (vendor heuristic only) |
| `OUI-PARROT-A0143D` | `A0143D` (24 bits) | Parrot SA | DRONE (vendor heuristic only) |
| `OUI-SKYDIO-381D14` | `381D14` (24 bits) | Skydio Inc. | DRONE (vendor heuristic only) |
| `OUI-DJIB-9C5A8A` | `9C5A8A` (24 bits) | DJI Baiwang | DRONE (vendor heuristic only) |
| `OUI-DJIB-EC72F7` | `EC72F7` (24 bits) | DJI Baiwang | DRONE (vendor heuristic only) |
| `OUI-QUANTUM-AC86D17` | `AC86D17` (28 bits) | Quantum-Systems | DRONE (vendor heuristic only) |
| `OUI-INSITU-0050C2C35` | `0050C2C35` (36 bits) | Insitu | DRONE (vendor heuristic only) |
| `OUI-INSITU-0050C2FD4` | `0050C2FD4` (36 bits) | Insitu | DRONE (vendor heuristic only) |
| `OUI-INSITU-70B3D57AD` | `70B3D57AD` (36 bits) | Insitu | DRONE (vendor heuristic only) |
| `OUI-INSITU-70B3D5865` | `70B3D5865` (36 bits) | Insitu | DRONE (vendor heuristic only) |
| `OUI-INSITU-70B3D5B3B` | `70B3D5B3B` (36 bits) | Insitu | DRONE (vendor heuristic only) |
| `OUI-INSITU-70B3D5D36` | `70B3D5D36` (36 bits) | Insitu | DRONE (vendor heuristic only) |
| `OUI-INSITU-70B3D5DD2` | `70B3D5DD2` (36 bits) | Insitu | DRONE (vendor heuristic only) |
| `OUI-FLIR-00407F` | `00407F` (24 bits) | FLIR Systems | NONE (avionics/payload context) |
| `OUI-XAG-A45129` | `A45129` (24 bits) | XAG | DRONE (vendor heuristic only) |
| `OUI-UAVNAV-B44D43A` | `B44D43A` (28 bits) | UAV Navigation | NONE (avionics/payload context) |
| `OUI-UAVIONIX-04714B1` | `04714B1` (28 bits) | uAvionix | NONE (avionics/payload context) |
| `OUI-UAVIONIX-546F71` | `546F71` (24 bits) | uAvionix | NONE (avionics/payload context) |
| `OUI-MICROH-000F92` | `000F92` (24 bits) | Microhard Systems | NONE (avionics/payload context) |
| `OUI-HUBSAN-98AAFC7` | `98AAFC7` (28 bits) | Hubsan | DRONE (vendor heuristic only) |
| `OUI-YUNEEC-E0B6F58` | `E0B6F58` (28 bits) | Yuneec | DRONE (vendor heuristic only) |
| `OUI-FIMI-6CDFFBE` | `6CDFFBE` (28 bits) | FIMI | DRONE (vendor heuristic only) |

All cases use a clearly synthetic `SQE-LAB-...` SSID. The BSSID contains
the selected IEEE vendor prefix and generated remaining bits, rather than
a recorded serial or identity from actual equipment. The generated suffix
could still coincide with an allocated address, so run only in a controlled
authorized test environment. Each case is off at boot and runs only when
selected for the standard 3–30-second time limit.

**MA-L (24-bit)** holds the first 3 bytes fixed. **MA-M (28-bit)** also
holds the high nibble of byte 4 fixed. **MA-S/IAB (36-bit)** holds the first
4 bytes and the high nibble of byte 5 fixed. Unassigned neighboring blocks
must not match. A 3-byte-only match for IEEE-held MA-M/S ranges is misleading.

## Corrected or rejected supplied attributions

- **DJI:** The original 10 SZ DJI MA-L assignments remain unchanged.
  Separately verified DJI Baiwang `9C:5A:8A` and `EC:72:F7` are added.
  `00:E0:4C` belongs to Realtek, not to DJI.
  `A0:6B:A6` and `D8:9C:A1` were not established as DJI and are excluded.
- **Autel:** `18:D7:93:6` and `EC:5B:CD:E` are 28-bit Autel assignments.
  `60:50:5F` was not verified as an Autel registration and is excluded.
- **Parrot:** Registry assignments are `00:12:1C`, `00:26:7E`,
  `90:03:B7`, `90:3A:E6`, and `A0:14:3D`.
  `00:1A:79` belongs to Telecomunication Technologies; do not mislabel.
- **Yuneec:** `E0:B6:F5:8` is correct (28-bit). `E4:8D:8C` belongs to
  Routerboard/MikroTik; `6C:EC:EB` belongs to Texas Instruments.
- **FIMI and Hubsan:** `6C:DF:FB:E` and `98:AA:FC:7` respectively, both
  28-bit MA-M registrations. The proposed 24-bit guesses are not equivalents.
- **XAG:** `A4:51:29` is the registered prefix, not `A8:C6:A0`.
- **Microhard:** `00:0F:92` is registered to Microhard; `00:25:0C`
  belongs to Senet.
- **UAV Navigation / uAvionix:** Exact 28-bit `B4:4D:43:A`,
  `04:71:4B:1`, plus 24-bit `54:6F:71` are used.
- **Insitu:** Exactly seven 36-bit IAB/MA-S registrations are used.
  Broad `70:B3:D5` is IEEE Registration Authority and cannot identify
  Insitu or Delair without the correct 36-bit subdivision.
- **FLIR:** `00:40:7F` is registered to FLIR Systems, but a thermal
  payload or camera is not proof of an airborne drone.

All these vendor-only tests must be scored as such. An alarm for one of
these beacons is a vendor-prefix heuristic, **not a valid Remote ID decode**.
Actual DJI/Autel model identification requires a separately verified
broadcast capture and standard/proprietary protocol evidence.

## Adjustable transmit time and touch flicker fix (same LAB branch)

On the CYD 3.2 home screen, tap **TX TIME / [SET]** in the top status row
above PROFILE. Use large **- 1 SEC** or **+ 1 SEC** buttons to select any
duration from **3 to 30 seconds**, then tap DONE. BACK also preserves
changes; RESET 12 restores the default. The setting is saved to SquachEmit's
NVS namespace across reboots, and applies to both touchscreen RUN and each
SUITE case, as well as serial RUN without explicit seconds. Serial
`RUN <ID> [seconds]` continues to override the preset for that individual test.
Existing auto-stop and STOP remain authoritative and duration cannot be
changed while RF or a suite is active.

CYD 3.2 touch now marks the full screen dirty **only for actual navigation
or radio-state transitions**, instead of clearing/repainting on every tap.
Petting Squachy, tapping unused space, hitting a page boundary, and
adjusting +/- use no full-screen clear. The TX-time number is redrawn
locally on adjustment. This is a firmware source fix, not proof of
flicker-free hardware: verify on the physical ST7798.

## CYD 3.2 ST7798 verification

Flash the unmerged LAB build from
https://retrodroid32.github.io/SquachEmit/?lab=1 using the CYD 3.2 target.
From TESTS → DRONES / REMOTE ID, select a vendor case and tap RUN.
Use the Serial Test Console at 115200 baud:

```text
RUN OUI-AUTEL-18D7936 12
RUN OUI-PARROT-00267E 12
RUN OUI-INSITU-70B3D57AD 12
```

Verify `UI,RUN_TAP`, `RADIO,AP_MAC_READY`, `TRUTH BEGIN`, and
`TEST,OUI_ONLY` with the *correct full-bit-length MAC prefix*. On STOP
or natural expiry, the state must return to IDLE. Independently check
Wi-Fi beacon reception on a separate receiver, and record detections
as OUI-only observations, not valid Remote ID.
Verify the display shows `172 CASES` at idle and only `Ns LEFT` when
transmitting. Alternate a vendor-only case with `RID-WIFI-DJI-AIR2S`
to ensure the latter still uses its locally administered `02:...` MAC.
Actual hardware RF validation is pending.

## Registry references

- https://maclookup.app/vendors/autel-lntelligent-technology-corp-ltd
- https://maclookup.app/vendors/autel-robotics-usa-llc
- https://maclookup.app/vendors/parrot-sa
- https://maclookup.app/vendors/skydio-inc
- https://maclookup.app/vendors/dji-baiwang-technology-co-ltd
- https://maclookup.app/vendors/quantum-systems-gmbh
- https://maclookup.app/vendors/insitu-inc
- https://maclookup.app/vendors/flir-systems
- https://maclookup.app/vendors/xag
- https://maclookup.app/vendors/uav-navigation
- https://maclookup.app/vendors/uavionix-corporation
- https://maclookup.app/vendors/microhard-systems-inc
- https://maclookup.app/macaddress/98aafc7
- https://maclookup.app/macaddress/e0b6f58
- https://maclookup.app/macaddress/6cdffbe
