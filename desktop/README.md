# SquachEmit Detection Workbench — Windows Desktop LAB

A standalone Windows x64 Electron GUI for 172 synthetic transmitter tests and exact-MAC receiver validation. The main workspace tracks ACTUAL classifications, baseline evidence, emitter TRUTH records, firmware SHAs, unverified results, batch progress, and CSV export. The right-hand desk has two **real LCD pixel canvases**, one per USB device, with pointer-to-touch support.

**Status: unmerged LAB.** Passing CI and Windows packaging are not a substitute for physical verification on CYD hardware.

## Install and run

1. Download the SquachEmit-Windows-Desktop-Review-x64 artifact from the successful Windows Desktop Workbench GitHub Actions run. Extract the unsigned Setup EXE or portable EXE.
2. Connect SquachEmit at 115,200 baud; SquachWatch on another COM port typically uses 2,000,000 baud. Assign each correct Windows COM port using the Connect USB buttons.
3. Enter both real flashed firmware SHAs, choose a device-named case, explicitly authorize RF testing, then Run selected case or Start automated tests.
4. The ACTUAL column reflects independently obtained SquachWatch LOG rows for the precise transmitter MAC, including newly seen rows later marked gone. Entries already present before RUN or observed without a valid baseline appear as UNTESTED, not automatically scored.
5. Export the CSV before closing. An emitter TRUTH BEGIN/END never constitutes receiver proof; missing LOG rows are not automatically reported as NONE.

## Actual USB LCD screens and remote touchscreen

The screen panels no longer show simulated telemetry as if it were a framebuffer. The desktop explicitly sends MIRROR ON and MIRROR FRAME and decodes entire RGB332 or RGB565 serial frames (MIRROR BEGIN/ROW/END). A canvas displays only complete real pixel data with row/sequence validation. The app requests one frame at a time, using backpressure. Mouse/touch coordinates are mapped into native LCD pixels via MIRROR TOUCH x y DOWN/MOVE/UP. PIN lock checks remain in firmware.

**SquachWatch:** requires unmerged SquachWatch-CYD PR #225 in the **separate SquachWatch session**. Its RGB332 TFT-sprite capture runs on actual physical redraws. No physical pixel fidelity or remote-touch behavior has yet been certified on 2.8-inch and 3.2-inch CYDs. Low-memory direct-LCD fallback and incomplete banded pushes cannot supply a valid screen image.

**SquachEmit:** the experimental CYD 3.2 ST7798 LAB firmware streams RGB565 TFT GRAM readback. For 2,000,000-baud capture: connect the CYD 3.2 emitter first at 115,200, click **Switch device UART to 2 Mbps**, disconnect the serial COM port without power-cycling, set **2,000,000** in Emitter baud and **Use for next connect**, reconnect and select **Mirror live**. A reboot normally resets the UART to 115,200. Some USB serial adapters reset the MCU on reconnect; this and ST7798 GRAM-read support are hardware gates, not guaranteed working features. S3 DevKit and Waveshare C5 do not yet expose mirrored LCDs.

**No misleading fallback:** if firmware has no mirror support, frames are corrupted, no LCD push occurs, or the serial baud is wrong, the canvas remains empty (or retains the last valid real frame) with an explicit status. No dummy replica is drawn.

**Concurrency:** SquachWatch LOG polling is suspended while its LCD mirror is active and resumes after MIRROR OFF. Stop mirroring and obtain a fresh receiver baseline before starting an accuracy benchmark. LOG and large LCD frames deliberately do not overlap on that serial link. Optional Windows USB cameras can show physical screens, but camera footage is not remote touch control and is not uploaded.

## Controls and security

The desk still supports SquachEmit STATUS, IDENTITY, LIST, HELP, RUN selected via the authorized Workbench, and STOP. SquachWatch supports LOG, RADIO, RADIO SCAN, MEM, SCAN AUTO/ACTIVE/PASSIVE, ROT and CLOCK appearance. No radio or pixel stream starts automatically at application launch. Receiver PIN lock cannot be bypassed.

Bundled assets are served from an ephemeral 127.0.0.1 port with an allowlisted file router, strict CSP, isolated renderer, Web Serial port chooser and optional camera permissions. No account, network listener, cloud telemetry, or video upload is involved.

Windows development commands (from desktop/, Node.js 22 or newer):

    npm install
    npm test
    npm start
    npm run dist:win

GitHub Actions builds unsigned Setup and portable x64 EXEs for review artifacts only; it does not create a release, tag or merge.

## Unmet physical merge gates

- SquachWatch 2.8 and 3.2: compare every captured LCD frame with the real screen; verify 1-/2-band redraw, orientation, gestures, PIN lock and LOG coexistence after mirror-off.
- SquachEmit CYD 3.2: verify UART baud handoff, LCD RGB565 readback/colors, portrait, gestures, taps, long holds and disconnect safety.
- Use both COM ports to run a representative BLE and Wi-Fi sequence including a full 172-case unattended test; verify continuous progress, complete ledger ACTUAL values, and real wrong-class/negative outcomes.
- Verify exact-MAC matching on new and newly gone rows; ensure stale detections and missing baselines never inflate precision, recall or pass rates.
- Validate STOP, reconnect, serial fault recovery, manual CSV import/export and no accidental RF emissions.

PR #37 remains a draft stacked on #36; PR #35 is an independent audit. No merge without explicit approval.
