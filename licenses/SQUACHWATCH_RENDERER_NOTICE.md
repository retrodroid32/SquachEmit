# SquachWatch rendering code incorporated into SquachEmit LAB firmware

The original SquachWatch-CYD visual rendering source is licensed under the
GNU General Public License, version 3. Its license text is reproduced in
`licenses/SquachWatch-GPL-3.0.txt`.

Original upstream project: https://github.com/skizzophrenic/SquachWatch-CYD
Source examined from fork: https://github.com/retrodroid32/SquachWatch-CYD

Imported and adapted in this repository:
- `include/squachwatch_synthwave_exact.h` — original Synthwave renderer
- `include/squachwatch_fireflies_exact.h` — original Fireflies renderer
- `include/squachwatch_terminal_exact.h` — original Terminal Log renderer
- `include/squachwatch_digital_rain_exact.h` — original Digital Rain renderer

The imported renderer source has adaptations for SquachEmit's own hardware,
full-band drawing, mascot footprint, and memory cleanup. It is not a faithful
one-for-one implementation of every upstream screen, palette, interaction, or
outfit and should not be described as such.

**License boundary:** The separate MIT license for independently authored
SquachEmit components does not revoke the GPLv3 license for the imported
SquachWatch source. Firmware binaries that combine the imported GPLv3
renderer code with the rest of SquachEmit are derivative combined works and
must be distributed in compliance with GPLv3, including providing the
corresponding source and notices. Do not describe such combined firmware as
MIT-only.

This notice does not change the ownership or licensing of independently
authored MIT components, and it does not authorize a production merge.
