# SquachEmit Detection Workbench — Windows Desktop LAB R5

A standalone Windows x64 Electron GUI for 172 synthetic transmitter tests and exact-MAC receiver validation. The main workspace tracks ACTUAL classifications, baseline evidence, emitter TRUTH records, firmware SHAs, unverified results, batch progress, and CSV export. The right-hand desk has two **real LCD pixel canvases**, one per USB device, with pointer-to-touch support.

**Status: unmerged LAB.** Passing CI and Windows packaging are not a substitute for physical verification on CYD hardware.

## Install and run

1. Download the SquachEmit-Workbench-USB-Mirror-172Case-Windows-x64-LAB artifact from the successful Windows Desktop Workbench GitHub Actions run. Extract the unsigned Setup EXE or portable EXE.
2. Connect SquachEmit at 115,200 baud; SquachWatch on another COM port typically uses 2,000,000 baud. Assign each correct Windows COM port using the Connect USB buttons.
3. Enter both real flashed firmware SHAs, choose a device-named case, explicitly authorize RF testing, then Run selected case or Start automated tests.
4. The ACTUAL column reflects independently obtained SquachWatch LOG rows for the precise transmitter MAC, including newly seen rows later marked gone. Entries already present before RUN or observed without a valid baseline appear as UNTESTED, not automatically scored.
5. Export the CSV before closing. An emitter TRUTH BEGIN/END never constitutes receiver proof; missing LOG rows are not automatically reported as NONE.

## Live USB LCD screens and physical touchscreen control (LAB R4)

**The Windows panels show only real hardware pixel data.** The CYD3.2 emitter firmware reads its physical ST7798 TFT GRAM via `lcd.readRect`, sends a complete RGB565 keyframe, and then sends only the changed rows of later images. The EXE reconstructs the screen from real pixels, verifies each sequence and base frame, and automatically requests a complete keyframe after any lost frame. No HTML dashboard, simulated pixels, or inferred statuses are displayed as LCD screenshots.

**Live behavior:** SquachEmit capture sends a maximum of two LCD rows per firmware loop iteration, rather than blocking the radio and touch processing with a single giant frame. The Windows EXE starts mirroring automatically when the correctly connected device supports it, polls for changed-row deltas, and scales the real pixel dimensions into the right-side LCD card. Mouse/touch DOWN, MOVE and UP events are mapped to display pixels and sent into the native physical UI hit-testing path, including clicks, press-and-hold and dragging. Stop/disconnect hides old screenshots; the desktop attempts to release any outstanding remote touch first.

**SquachEmit CYD 3.2:** connect at 115,200 baud, click **Switch device UART to 2 Mbps**, disconnect without power cycling, select **2,000,000** and **Use for next connect**, reconnect the same USB COM port. If the board resets when the port is reopened, the UART resets to 115,200: reconnect at 115,200 and repeat. The current hardware readback is still experimental: if the first 16 LCD rows do not show expected color variation, firmware reports `MIRROR ERROR LCD_READ_UNAVAILABLE` instead of displaying invented pixels. This means **real ST7798 read capability and exact visual color matching remain physical-device acceptance gates**, regardless of successful CI.

**SquachWatch:** the other LCD panel requires compatible, unmerged SquachWatch PR #225 from the **separate SquachWatch project**. The desktop accepts that firmware's real RGB332 keyframes with correct portrait/landscape dimensions and remote pointer messages. No SquachWatch firmware is edited or merged in this SquachEmit work. SquachWatch PIN restrictions remain firmware-enforced. Its RGB332 transfer may approximate 24-bit screen colors due to the firmware's 8-bit color depth, so exact color fidelity on SquachWatch is not yet established.

**Other boards:** WaveShare ESP32-C5 and ESP32-S3 do not implement this LCD mirror. No fake fallback is drawn. Optional physical USB cameras can show video of the board, but they do not provide remote touchscreen input.

**Detection test concurrency:** the receiver's LOG poller is suspended while its LCD mirror is active. Disable mirroring and acquire a fresh receiver baseline before running verified detection scoring; one shared USB serial connection cannot simultaneously transfer large screenshots and unattended test evidence.

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

## R5 troubleshooting — no screen image

The R5 EXE now waits for the real firmware's `MIRROR READY` acknowledgment instead of treating a successful USB serial write as successful mirroring. A wrong UART baud or device reset is shown as an error, not a blank endless spinner. The **Diagnose physical LCD and USB baud** button sends `MIRROR DIAG` and displays the actual returned baud, calibration, dimensions and RGB565 TFT readback sample diversity.

The CYD32 firmware no longer rejects LCD screenshots just because the touch panel is not calibrated. TFT SPI read frequency is 6 MHz, improving chances of usable readback. For failure to reconnect at 2Mbps, `MIRROR FRAME` can now work at 115200 with exact RGB565 pixels, but a full initial frame may take 20–40 seconds. It is for diagnosis and slow compatibility, **not** a substitute for a responsive live screen. If the diagnostic reports `TFT_READBACK_SUSPECT` or frame capture reports `LCD_READ_UNAVAILABLE`, this ST7798 board may not expose display GRAM readback; hardware-specific framebuffer capture would be needed for exact live mirroring.

## R6 — Receiver LCD no longer blocks transmission

The SquachWatch receiver and the SquachEmit emitter use different USB COM ports. Receiver `MIRROR ON` pauses receiver `LOG` polling because its screenshot rows share the same serial stream as exact-MAC detections. In earlier EXE builds, automatically opening the receiver LCD blocked `Run selected` and the 172-case suite with "Stop SquachWatch LCD mirror first…".

R6 resolves this collision: pressing **Run selected case** automatically sends `MIRROR OFF` to the **receiver only**, stops displaying its stale LCD image, restarts receiver LOG polling and obtains a fresh detection baseline when possible, then sends the requested emitter RUN command. A missing/failed receiver LOG baseline **never** suppresses a manually requested RF test; it makes receiver scoring unverified until real evidence arrives. The unattended batch runner pauses receiver mirroring and requires a fresh LOG baseline before beginning (rather than asking users to stop mirroring manually). It avoids inferring false negatives or fabricated receiver matches.

SquachEmit's own live LCD/remote-touch panel is an **independent emitter serial connection** and is not paused by receiver LOG tests; real emitter LCD frames are permitted during the 172-case suite, with remote touches disabled during unattended batches to protect test integrity. After testing, click **Mirror live** on SquachWatch to restore its optional screen view. This pauses receiver LOG collection again; toggle it off when running another test.

This is a change to the **Windows EXE** on SquachEmit PR #37, not to the separate SquachWatch firmware.
