# README UI animations: actual SquachEmit C++ draw code

These GIFs are **native firmware UI render captures**, not AI illustrations,
hand-drawn UI mockups, or evidence that physical CYD hardware passed validation.

The generator builds these production source files **unchanged**:

- `src/cyd32_ui.cpp` — exact CYD 3.2 screens, menus, titles and buttons
- `src/cyd32_squachy_port.cpp` — source-derived Squachy animation geometry
- `src/scenarios.cpp` — 95 test records used by the on-screen library

Host-only substitutes for `Arduino.h`, `Preferences.h`, and
`TFT_eSPI.h` implement pixel drawing, simple text metrics and synthetic touches.
The host adapter produces RGB565 frames; Pillow encodes animated GIFs at 2x
nearest-neighbor scale for the README.

**Limitations:** The stand-in text rasterizer uses a DejaVuSansMono-derived
5x7 pixel font, not the TFT_eSPI font bitmaps. Slight text differences and
panel-specific inversion/antialiasing may therefore occur. The animations
are not photos, do not depict actual RF transmissions, and do not establish
physical-board or detection performance.

Regenerate in an authorized development checkout:

```bash
python -m pip install pillow==11.3.0
python tools/readme_capture/generate.py --verify
```

Outputs:

- `docs/squachemit-demo.gif` — full home → categories → library → active case
- `docs/squachemit-device-library.gif` — alphabetized device menu transitions
- `docs/squachemit-test-running.gif` — Squachy's actual C++ on-air animation

The one-time docs branch workflow also builds and commits these three GIFs.
The final generated images can be reviewed from the README PR before merge.

The core source-derived Squachy animation retains GPL-3.0-only provenance;
see `docs/SQUACHY-PORT.md`.

## Generated demo checks

The current README capture run compiled both unmodified CYD 3.2 C++
renderers and produced **110 native TFT frames** before building the
three animated sequences. Output frames were checked for GIF decodability
and 640 × 480 embedded dimensions. The demo and category previews are
separate animations; the hardware photographs are historical and collapsed
below them. The public firmware release itself is unchanged.
