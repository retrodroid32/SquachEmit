# R8.9.3 authentic SquachWatch LCD streaming — compatibility handoff

Scope: SquachEmit Windows Workbench only. **No changes to SquachWatch firmware.** SquachWatch PR #238 `CYD32-EARLY-BUFFER-AUTHENTIC-MIRROR-R1` must implement the firmware half independently.

## Confirmed bottleneck
SquachWatch's CYD32 capture reportedly copies at most two genuine RGB332 LCD rows per render opportunity. A 240-row baseline therefore needs about 120 opportunities, potentially longer than its six-second capture-progress timeout. Free heap ~10,756 bytes, largest contiguous block ~724 bytes: no second full-screen ESP32 framebuffer.

## Workbench implementation
- Host `MirrorAssembler` owns 320×240/240×320 RGBA framebuffer, decodes RGB332 rows, verifies dimensions, row uniqueness/length, sequence, baseline and delta integrity.
- Full baseline is hidden until all rows and `MIRROR END` verify; wrong sequence, malformed row, incomplete transfer, disconnect, rotation/screen invalidation or timeout revoke host baseline.
- One active capture at a time. Timeout policy: 30 seconds without advancing progress, 120 seconds absolute cap per receiver capture. Host retries boundedly, then stops with explicit error instead of inventing a screen.
- `MIRROR PROGRESS` is optional: existing `MIRROR ROW` records already update progress. Progress counters must *advance*; duplicate heartbeats do not extend idle timeout.
- `MIRROR INVALIDATE` forces immediate host image invalidation and a new complete baseline.
- Existing firmware still uses `MIRROR FRAME` with no argument. Optional sequence argument is sent only when `MIRROR READY ... SEQBASE` advertises support.
- Native UI v2 remains explicitly labeled partial diagnostic data, never a fake physical pixel view.
- SquachWatch receiver LOG polling is suppressed while pixel mirror active; detection tests pause mirror and restore exact-MAC LOG evidence. No new scoring shortcuts. Existing serial 2,000,000 baud remains supported.

## Minimal optional firmware protocol extensions (SquachWatch session)
Keep existing `MIRROR ON` / `MIRROR READY RGB332 320x240` / `MIRROR FRAME` / `MIRROR BEGIN <seq> <width> <height> RGB332` / `MIRROR ROW <seq> <y> <base64>` / `MIRROR END <seq>` / `MIRROR OFF`.

Recommended additions:
1. `MIRROR PROGRESS <seq> <capturedRows>/<totalRows>` when rows are captured but not transmitted. Count must be monotonic within one baseline; host verifies actual row payload separately.
2. `MIRROR INVALIDATE <reason>` on screen generation change, rotation, display reset, or incomplete/aborted capture. Require a new full keyframe; never reuse the prior image.
3. `MIRROR READY RGB332 320x240 SEQBASE` only if firmware actually accepts `MIRROR FRAME <lastAcceptedSeq>`. Host sends the bare legacy command otherwise.
4. For changed rows, `MIRROR DELTA <newSeq> <baseSeq> <width> <height> RGB332` followed by changed `MIRROR ROW` and `MIRROR END`. Host rejects if baseline sequence, dimensions or pixel format mismatch.
5. Device-side capture progress timeout must be measured against actual render opportunities/forward progress, not an unconditional 6-second wall clock. Do not send `MIRROR END` until every baseline row has been captured and delivered.

No additional large MCU buffers are requested. Use bounded row buffers and serial backpressure; pixel correctness and scan/render budget remain firmware responsibilities.

## Physical acceptance
- At 2,000,000 baud: first baseline completes and is visibly *identical* to the device.
- Menus/alerts/outfits/animation update using valid deltas, new baselines when screen generation changes, without stale carryover.
- Portrait rotation yields matching dimensions and correctly mapped remote touch.
- Slow first capture does not cause repeated MIRROR OFF/ON; stalled progress eventually reports an explicit error.
- Receiver exact-MAC detection tests can still pause mirror, collect LOG, and resume after the test.
- Confirm no added full-screen MCU allocation, radio regression or boot loop.

LAB PR only; **do not merge either repository without explicit approval**.
