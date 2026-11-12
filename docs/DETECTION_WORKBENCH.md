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
- Connect a *separate* **SquachWatch** at **2,000,000 baud** (selectable).
  Its existing firmware supports the read-only serial **`LOG`** command,
  returning a structured table of type, MAC, RSSI, hits, state and first-seen
  time. **No SquachWatch firmware change is required.**
- Once connected, the Workbench automatically requests a `LOG` snapshot
  approximately every 1.5 seconds. It displays matched receiver evidence
  and automatically fills the observed type if a **new** table row has the
  **exact MAC transmitted by SquachEmit**. The match must follow a complete
  pre-test baseline; unchanged historical sightings cannot produce a pass.
- Keep SquachWatch's LOG/UI available for manual review of **preexisting
  identities, ambiguous matches and cases where nothing was detected**.
  The workbench never parses unrelated diagnostics as an alert.
- Close existing serial consoles and Web Installer serial connections before
  selecting either port; the same OS serial port cannot be opened twice.

## Per-case procedure

1. Enter a run ID, receiving board (CYD 2.8 or 3.2), and the actual **Git
   commit SHA** of both firmware builds. At least seven hexadecimal characters
   are required to count a result. Don't substitute a marketing release name.
2. Connect the receiver **first**. Confirm the status says **AUTO LOG ACTIVE**
   and displays its latest row count. The serial port cannot be used by a
   separate terminal at the same time. Then connect the emitter. If the
   receiver is not connected, you can still record observations manually.
3. Select a test from the actual **172-case catalog** and 3–30-second transmit
   duration. Confirm authorized RF testing and click **Run selected case**.
   Nothing transmits until this explicit button is pressed.
4. When the receiver is connected and you press RUN, the Workbench requests
   a **fresh `LOG` baseline before sending any transmitter command**. The
   actual `TRUTH BEGIN` creates a pending trial with its precise MAC. A
   later successful receiver `LOG` snapshot with a newly detected or
   reactivated **same MAC** automatically records the observed detection
   class, RSSI and receiver-log evidence, while the emitter is still running.
   A matching `TRUTH END` with `ok` then completes the scored observation.
   This is automatic receiver evidence, not just a transmitter claim.
5. If no fresh exact-MAC match appears, the Workbench **does not automatically
   mark NONE**. It cannot infer uninterrupted receiver listening from a
   missing `LOG` entry. Check the actual SquachWatch display/LOG, then
   manually set `NONE` *only* if the entire listening window was complete.
   A matching device already present in the baseline, a re-used MAC without
   a new sighting, or multiple different classifications for one MAC
   requires manual review. Stopped/failed emissions are excluded.
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
complete listening window, an actual automatically correlated SquachWatch
LOG hit or manually confirmed observation, and both firmware SHAs.
A fresh exact-MAC hit provides positive evidence even if a full timeout
window has not elapsed. Absence of any hit is **not** automatically a
negative. Unexpected classes are **WRONG** and contribute to both a missed
target and false-positive category in precision/recall.

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

## How automatic detection matching works

The Workbench uses an already available SquachWatch serial command
`LOG` (see SquachWatch-CYD `src/clock.cpp` and
`src/detection.cpp::logDump()`). A response has this actual structure:

```text
[log] 1 rows, newest first
row,type,mac,rssi,hits,state,when
0,DRONE,04:a8:5a:12:34:56,-53,1,here,+1:04
```

The browser records a completed `LOG` table before RUN, then accepts only
a *later, complete* table with an exact-MAC `here` row **not seen in the
baseline**, or a row whose hit count increases on reacquisition. It also
requires the matching emitter `TRUTH BEGIN` (containing the precise MAC),
and normal `TRUTH END` before scoring.

`LOG` is a snapshot of a limited receiver RAM ring, **not an event-stream
subscription**. A device present before a trial may remain continuously
active and be impossible to distinguish from the next test using this
interface. Likewise, a receiver that stops scanning can produce no hits.
The Workbench rejects stale rows and does not convert silence into
TP/TN/FN. For conclusive automatic negative results or exact measurement
timestamps, SquachWatch would need a separately approved, structured
detection/event streaming interface and its own independent verification.

Automatic positive matching does not synchronize clocks; the Workbench
doesn't invent millisecond latency from poll timestamps. Both firmware
SHAs are still required for scored performance metrics. Changes in this
PR affect **only SquachEmit's browser Workbench**, not SquachWatch or the
public v1.1.0 firmware.
