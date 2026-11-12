# SquachEmit Detection Workbench (PR #36)

The [Detection Workbench](https://retrodroid32.github.io/SquachEmit/workbench/)
is a proposed **static, browser-local** test dashboard for measuring how often
SquachWatch actually detects SquachEmit test scenarios. The GitHub Pages URL
will become available **only after this PR is approved and merged**, followed by
a successful Pages deployment. No server receives logs, MAC addresses,
device serial numbers, or other captured RF information.

## Testing this draft PR before merge

GitHub CI on PR #36 creates the downloadable artifact
`squachemit-detection-workbench-review-preview`. Open the PR's successful
Actions run, download that artifact and extract it to a folder on your PC.
From **PowerShell**, inside the extracted folder, run:

```powershell
py -m http.server 8000
```

Open `http://localhost:8000/workbench/` in desktop Chrome or Edge.
Localhost is a trustworthy origin for Web Serial. This preview intentionally
does **not** change or deploy the public firmware installer, and it includes
**no firmware binaries**. Disconnect any other serial console before
connecting either board.

## Hardware and browser

- Desktop Chrome or Edge, HTTPS (or localhost), USB data cables.
- Connect **SquachEmit** over Web Serial at **115,200 baud**.
- Optionally connect a *separate* **SquachWatch** at **2,000,000 baud** (other
  baud options are selectable) to preserve raw receiver serial evidence.
- Have a separately visible SquachWatch LOG/UI so you can verify the real
  detected classification. The workbench deliberately does **not** guess
  detections from undocumented receiver debug text.
- Close existing serial consoles and Web Installer serial connections before
  selecting either port; the same OS serial port cannot be opened twice.

## Per-case procedure

1. Enter a run ID, receiving board (CYD 2.8 or 3.2), and the actual **Git
   commit SHA** of both firmware builds. At least seven hexadecimal characters
   are required to count a result. Don't substitute a marketing release name.
2. Connect the emitter and receiver if available; the receiver USB connection
   is optional because screen/LOG observations can be recorded manually.
3. Select a test from the actual **172-case catalog** and 3–30-second transmit
   duration. Confirm authorized RF testing and click **Run selected case**.
   Nothing transmits until this explicit button is pressed.
4. Clicking RUN does not create a result. Only an actual serial
   `TRUTH,{"event":"BEGIN",...}` from the transmitter creates a trial.
   `TRUTH END` with status `ok` records an **emitter-reported successful
   end**, not proof of physical airtime, a SquachWatch alert, or a correct
   classification.
5. Select the new trial, confirm the actual SquachWatch observed category
   **or** `NONE` (only after a *complete listening window*), and enter
   timing/notes if available. If interrupted, classify the window as
   `incomplete` so it is excluded. Stopped/failed emissions are excluded too.
6. Save the observation and export the CSV frequently. The current preview
   holds trials **only in the browser session** and never syncs to a server.
   You can import a previously exported CSV to continue local analysis.

The exporter uses the precise columns accepted by
`tools/score_trials.py`: `run_id,receiver,receiver_sha,emitter_sha,id,
emission,window,observed,latency_ms,notes`.

Example after exporting:

```bash
python tools/score_trials.py my-trials.csv --output accuracy.md --json accuracy.json
```

## Scoring and evidence

**UNTESTED:** no completed emitter result, missing firmware SHA, missing actual
receiver classification, or an unconfirmed listening window. A TRUTH BEGIN
alone always stays untested.

**EXCLUDED:** emitter failure/early STOP or an interrupted/incomplete listening
window.

**TP / TN / FP / FN / WRONG:** only counted with successful emitter END, a
complete listening window, an independently confirmed observation and both
firmware SHAs. An unexpected class is **WRONG** and contributes to both a
missed target and a false-positive category in precision/recall.

The dashboard displays recall, precision and median latency only when
corresponding confirmed data exists; a blank dataset shows dashes rather than
invented success percentages. Timing is manually entered for true-positive
observations; the transmitter and receiver have unsynchronized clocks, so
automatic latency from their USB log timestamps would be misleading.

## Scope and limitations

This workbench does not modify SquachEmit RF firmware, create or trigger
automatic continuous transmitting, read secrets, upload device logs, or
authenticate manufacturer/vendor OUIs. The **37 preexisting OUI-only
scenarios** transmit spoofable registered vendor prefixes by design; use
those only in a controlled, authorized test environment. The workbench
records what the human tester verified; it is not an RF power meter,
certified aviation Remote ID verification device, or an independent
SquachWatch hardware test result by itself.
