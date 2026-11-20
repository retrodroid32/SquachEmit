# SquachMesh to SquachEmit — feasibility assessment

SquachWatch uses BLE manufacturer data (company ID 0xFFFF, SQM1 version 1) for peer appearances and NimBLE advertising / scan responses for messages, not a single standalone ESP-NOW library. Reviewed upstream include/squachmesh.h, src/squachmesh.cpp, src/mesh.cpp, src/detection.cpp, include/meshmsg.h, src/meshmsg.cpp, src/meshtalk.cpp and src/squachy.cpp.

- Appearance codec: platform-neutral packed nickname/outfit/shade identity, 20 outfits and optional custom names.
- Runtime discovery: BLE adverts, scan callbacks, peers and stale timeout, optional guest Squachy appearance.
- Full messages: separate transport, five-word phrase key derivation, AES-128-CCM, replay protection, pairing/invites, split BLE frames, persistent identity.
- UI and settings: squad menu, inbox, friends, invitation dialogs and avatar interactions require separate integration.

SquachEmit already uses one NimBLE advertising object for controlled synthetic BLE transmissions. It also changes Wi-Fi mode/MAC for synthetic Wi-Fi cases, and its existing CYD32 build has SQUACH_MESH=0. Enabling mesh radio automatically risks contaminating receiver test evidence, breaking START/STOP, increasing heap and flash pressure, and competing with existing adverts.

Recommendation: separate optional mesh feature, with explicit ownership of the shared NimBLE host/radio. Phase 1: passive decode of authentic SquachWatch peers while Emit is idle. Phase 2: optional idle-only appearance advertising and mutual peer verification. Phase 3: Squad UI, preserving Emit RUN/STOP and FULL8 frame pacing. Phase 4: compatible encrypted messaging, pairing and golden vector tests. Pause all mesh scan/adverts during every RUN/SUITE/favorites session and gaps, verify 2.8 and 3.2 hardware both ways, preserve unique real MAC (not synthetic identity), and validate timeouts, memory and receiver detections. No mesh code or radio operation is changed in R14.13.
