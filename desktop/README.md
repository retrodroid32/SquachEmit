# SquachEmit Detection Workbench — Windows Desktop LAB

This is a standalone Windows x64 GUI (Electron) around the existing 172-case Detection Workbench. The app opens two independent USB Web Serial connections and provides a right-side device desk for SquachWatch and SquachEmit. The automated test runner, per-signal friendly names, strict exact-MAC evidence matching, firmware-SHA gating and CSV export remain part of the same interface.

## Install and run

1. Open the successful **Windows Desktop Workbench** GitHub Actions run for this branch and download the **SquachEmit-Windows-Desktop-Review-x64** artifact. Extract the artifact ZIP.
2. Run the **Setup .exe**, or use the **portable .exe** without installing. LAB builds are unsigned and Windows may display a SmartScreen notice.
3. Connect SquachEmit (115,200 baud) and a separate SquachWatch (typically 2,000,000 baud) using USB data cables. Click Connect USB for each device, selecting its correct Windows COM port.
4. Select a signal by its friendly device name, enter the actual flashed firmware commit SHAs for both devices, and check the authorized controlled RF test box. Click Run selected case or Start automated tests. The full 172-case batch includes manufacturer OUI tests and requires an additional confirmation.
5. Review actual receiver LOG evidence. A matching transmitter TRUTH BEGIN/END alone is not a detection, and an absent match never automatically counts as a negative. Export the CSV before exiting; trial data is not persistently saved.

## Live views: what is and is not supported

**Live USB telemetry** is shown in each right-hand device graphic. SquachWatch provides detection LOG snapshots, classifications, MAC, RSSI and radio/scan diagnostics. SquachEmit provides transmitter TRUTH BEGIN/END/ERROR, selected case and status. The telemetry graphic is NOT a pixel-perfect copy of either LCD.

**Actual screen video** is possible using one Windows USB camera aimed at each physical device. Choose a camera source in each panel and click Show video. Both cameras can run simultaneously. Video remains local and is not recorded or uploaded.

The current USB firmware does not implement general LCD screenshot streaming or remote touchscreen coordinates. The Windows GUI cannot truthfully promise a real LCD mirror or simulate finger taps without additional firmware support in BOTH projects. The control buttons access existing serial command capabilities only.

## Available device controls

- SquachEmit: Connect/Disconnect, RUN selected Workbench signal, STOP, STATUS, IDENTITY, LIST, HELP, and the existing automatic 172-case tester.
- SquachWatch: Connect/Disconnect, LOG, RADIO, RADIO SCAN, MEM, SCAN AUTO/ACTIVE/PASSIVE, ROT, CLOCK appearance, and receiver evidence polling. SquachWatch's on-device PIN lock is respected; the GUI cannot override it.
- Neither device's RF operation starts automatically on application launch. All transmission requests require the authorized testing checkbox in the main Workbench.

## Security and build

The desktop executable serves bundled, local Workbench files from an ephemeral port bound only to 127.0.0.1. It has a strict route allowlist and content-security policy. Electron uses renderer sandboxing and disables Node integration. USB port selection is explicit. Video capture is optional and needs a camera permission grant. No network backend, video upload, telemetry upload, or remote account is involved.

On a Windows development machine with Node.js 22 or newer, open PowerShell in the desktop directory and run:

    npm install
    npm start

To make both unsigned Windows executables:

    npm run dist:win

To run desktop static/contract tests:

    npm test

The GitHub Actions workflow builds an NSIS installer and portable EXE on Windows, uploads them as temporary review artifacts, and does NOT publish GitHub releases, tags or public firmware. The Windows wrapper is a separate DRAFT PR stacked on the unmerged Detection Workbench PR #36. It still requires actual two-board Windows hardware verification.
