# SquachEmit Windows Workbench — R7.1 camera-free native interactive device UI

The Windows EXE includes the 172-case detection Workbench and a right-side **software-rendered SquachEmit CYD 3.2 screen**. It is an independent PC renderer driven by firmware navigation state — **not a photograph, LCD screenshot, RAM framebuffer readback, RGB565 stream, or simulated hardware status**. It redraws locally every 120 ms from the device's live 180 ms state updates.

## Correct setup

1. Flash the matching **CYD 3.2 ST7798 R7 LAB** firmware.
2. Launch **SquachEmit Windows Workbench R7.1 v0.5.1**, choose the SquachEmit emitter COM port and connect at **115,200 baud**. No UART switching or 2 Mbps connection is required.
3. The emitter's **Sync device UI** begins automatically after connection. Firmware answers `UI READY 1 320 240 MODEL` and streams compact `UI STATE` lines containing the current screen, category/page, selected scenario, duration, live radio status, countdown, calibration and animation clock. These are **not display images**.
4. The PC reconstructs the native 320×240 UI in Canvas2D from the matching firmware geometry, colors, menus and procedural Squachy artwork; click, hold or drag the PC image to send `UI TOUCH x y DOWN|MOVE|UP` back to the firmware's existing hit-testing code.
5. Physical touches and RF state changes are visible on the PC because the firmware continuously sends its authoritative state. When USB data stops for 2.5 seconds, the image is hidden instead of impersonating a live screen.
6. **Refresh state** asks for one state snapshot. **Stop** unsubscribes, cancels animations and releases remote touch.

**Important acceptance gate:** The layout and procedural graphic geometry were ported from SquachEmit's own CYD32 firmware, but browser text rasterization and anti-aliasing can differ from TFT_eSPI. Physical pixel-perfect appearance, remote touch response and long-press behavior still need hardware verification. The EXE should never say it is showing captured LCD pixels.

## SquachWatch receiver and RF testing

The *separate* SquachWatch receiver panel can still use compatible SquachWatch LAB USB screenshots. Its `MIRROR ON` mode shares the same receiver serial channel as exact-MAC `LOG` evidence, so receiver mirroring **does not auto-start on connection**. When users press Run selected or start the 172-case suite, the Workbench automatically pauses SquachWatch mirroring, resumes LOG polling and requests a fresh baseline. An unavailable receiver LOG does **not** block a manual transmitter RUN; such trials remain unverified. Batch tests require a real receiver LOG baseline.

The SquachEmit screen and transmitter control live on their *own* serial connection, so its software-rendered UI remains available while testing. Production SquachEmit v1.1.0 is not changed and the 172-case signal catalog remains intact.

## Protocol / compatibility

- R7 uses `UI ON`, `UI OFF`, `UI GET`, and `UI TOUCH <x> <y> <DOWN|MOVE|UP>`.
- Firmware sends `UI READY 1 320 240 MODEL` and bounded `UI STATE 1 ...` records. Coordinates are 320×240 landscape UI coordinates.
- R7 does not call TFT `readRect`, transmit RGB rows, or adjust serial baud.
- Old R4–R6 EXEs that expect `MIRROR FRAME` are **not compatible** with R7 firmware. Install the matching R7 EXE and R7 LAB firmware as a pair.
- This experimental feature is available only on SquachEmit CYD3.2, not Waveshare C5 or ESP32-S3.

## No camera access

The optional physical webcam preview has been removed from both device cards. This build neither requests Windows camera permission nor enumerates video devices. SquachEmit's working native UI and SquachWatch's optional USB display mirroring are unaffected.

## Packaging and security

Electron serves the Workbench and native renderer only from local loopback with a restrictive asset allowlist, CSP and sandboxed renderer. USB COM selection is user-initiated; remote UI commands never transmit RF automatically. The standalone unsigned Windows x64 NSIS installer and portable executable are produced by the Windows Desktop Workbench Actions workflow. No public release or merge is implied by an artifact build.
