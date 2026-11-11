# README UI animations: actual SquachEmit C++ draw code

These GIFs are **native firmware UI render captures**, not AI illustrations,
hand-drawn UI mockups, or evidence that physical CYD hardware passed validation.

The generator builds these production source files **unchanged**:

- `src/cyd32_ui.cpp` — exact CYD 3.2 screens, menus, titles and buttons
- `src/cyd32_squachy_port.cpp` — source-derived Squachy animation geometry
- `src/scenarios.cpp` — 172 test records used by the current development UI

Host-only substitutes for `Arduino.h`, `Preferences.h`, and
`TFT_eSPI.h` implement pixel drawing and synthetic touches. The font rasterizer
now uses the **actual TFT_eSPI GLCD 5x8 (Font 1) and proportional 16px
(Font 2) bitmap glyphs, advance widths, and textWidth calculations** rather
than the previous scaled DejaVu mock font. The original upstream TFT_eSPI
license is preserved in `tools/readme_capture/TFT_ESPI_LICENSE.txt`.
The host adapter produces RGB565 frames; Pillow encodes animated GIFs at 2x
nearest-neighbor scale for the README.

**Limitations:** Text is rasterized using the actual TFT_eSPI font pixel data.
Panel-specific inversion and hardware rendering details may still differ.
The animations
are not photos, do not depict actual RF transmissions, and do not establish
physical-board or detection performance.

Regenerate in an authorized development checkout:

```bash
python -m pip install pillow==11.3.0
python tools/readme_capture/generate.py --verify
```

Outputs:

- `docs/squachemit-demo.gif` — home → TX-time editor → categories → library → active test
- `docs/squachemit-tx-time.gif` — actual 3–30-second editor (plus/minus/reset/done), captured without a full-screen redraw for value changes
- `docs/squachemit-device-library.gif` — alphabetized 172-case device menu transitions
- `docs/squachemit-test-running.gif` — Squachy's actual C++ on-air animation with the selected transmit duration

The docs-branch workflow also builds and commits these four GIFs.
The final generated images can be reviewed from the README PR before merge.

The core source-derived Squachy animation retains GPL-3.0-only provenance;
see `docs/SQUACHY-PORT.md`.

## Generated demo checks

The README capture builds the current **172-case** CYD 3.2 firmware renderer,
using its actual home, TX TIME editor, navigation and countdown logic.
It verifies the 12-second initial duration, increments, decrements, reset and
13-second configured run preview through actual synthetic touch callbacks.
The output is four GIFs, verified for decodability and **640 × 480** embedded
dimensions. These images are host-rendered firmware UI previews, not proof
of physical RF performance. The separate **v1.0.0 public release still
contains its original 95-case firmware**; updating the README does not
change that tagged release.
