# Review: jayofelony/pwnagotchi

Reviewed 2026-11-09, default branch `noai` (not `main`).
Repository: https://github.com/jayofelony/pwnagotchi

## What applies to SquachEmit

1. State-driven character reactions: `pwnagotchi/ui/view.py`,
   `pwnagotchi/ui/state.py`, `pwnagotchi/ui/faces.py`.
   **Adopt the idea, not GPL source/graphics.** The new CYD32 design
   independently draws an original Squachy and switches status by actual
   IDLE / SUITE / ON AIR states. The LCD is TFT_eSPI RGB565; Pwnagotchi's
   Pillow / framebuffer + Raspberry Pi display modules are not portable.
2. Change tracking: `pwnagotchi/ui/state.py` marks dirty elements rather
   than invoking an unrelated UI update. SquachEmit's new renderer only
   redraws the full 3.2 LCD when a menu/case/state changes, and updates the
   small countdown field every second while emitting.
3. Session statistics: `pwnagotchi/plugins/default/session-stats.py`.
   A future independent HOST reporter could show completed trials, positive
   and negative detections, missed cases, and per-CYD trends from
   `tests/trials.csv`. SquachEmit already has `tools/score_trials.py`.
4. Channel history: `pwnagotchi/strategy/channels.py` and
   `pwnagotchi/strategy/statistics.py` track channel observations and
   revisit active channels. Useful as **experimental SquachWatch receiver**
   scan diagnostics, not as adaptive on-air emission. An emitter should
   stay on known channels 1/6/11 for reproducible test results. Compare
   the established ESP32DualBandWardriver channel dwell metrics.
5. Peer presence: `pwnagotchi/mesh/peer.py`, `mesh/wifi.py`.
   An opt-in *lab-only* receiver/emitter session marker is conceivable,
   but independent serial event timestamp capture is more trustworthy
   than assuming an 802.11 advertisement means the other board received
   the frame.
6. Plugin hooks: `pwnagotchi/plugins/default/example.py` demonstrates
   lifecycle events, but Linux threads, Python, `prctl` and Bettercap
   don't translate directly to ESP32. SquachEmit can eventually use
   compile-time registered C++ test modules and a bounded event queue.

## What does NOT apply

- Bettercap handshake capture, PMKID capture, credential-cracking workflows,
  deauthentication and association attacks. These are outside SquachEmit's
  opt-in, non-disruptive, synthetic-identity RF validation purpose.
- AI-based automated channel/attack control. The current Pwnagotchi README
  explicitly says the AI was removed to improve stability and battery life.
- Raspberry Pi Linux display drivers and e-ink-specific refresh schedules:
  the owner's 3.2 CYD is a classic ESP32 TFT with touch, not Raspberry Pi.
- Pwnagotchi's GPLv3 source/graphics cannot be copied into SquachEmit's
  MIT-licensed fork without considering the resulting GPL obligations.
  The CYD32 UI code here is newly written and only uses broadly known
  design ideas and SquachWatch's own color conventions.

## Proposed follow-up work, separate PRs

1. Passive `TRUTH` / SquachWatch receiver session timeline and report
   viewer, with no unverified detection inferred from transmitter output.
2. Explicit opt-in channel 1/6/11 dwell diagnostics and false-negative
   analysis on SquachWatch, physically tested on both 2.8 and 3.2.
3. Tiny bounded C++ test event hook interface for optional synthetic
   RF fixture packs; no Linux plugin system and no unattended RF.

These are recommendations, **not implemented** by the UI PR.
