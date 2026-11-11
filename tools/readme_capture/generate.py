#!/usr/bin/env python3
"""Capture the *real* SquachEmit CYD 3.2 C++ UI under a host TFT pixel sink.

The firmware source src/cyd32_ui.cpp and src/cyd32_squachy_port.cpp are both
compiled unmodified. Host stand-ins provide only Arduino, Preferences and
TFT_eSPI draw operations. No actual RF transmission occurs.
"""
from __future__ import annotations
import argparse
from pathlib import Path
import subprocess
import tempfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
CAPTURE = Path(__file__).resolve().parent
DOCS = ROOT / "docs"


def images_at(folder: Path):
    return [Image.open(p).convert("RGB") for p in sorted(folder.glob("*.ppm"))]


def gif(frames, name: str, delay=150):
    if not frames:
        raise RuntimeError("No frames for " + name)
    # 2x nearest keeps true embedded 320x240 TFT graphics pixel sharp.
    output = [im.resize((640, 480), Image.Resampling.NEAREST).quantize(colors=96)
              for im in frames]
    path = DOCS / name
    output[0].save(path, save_all=True, append_images=output[1:], loop=0,
                   duration=delay, disposal=2, optimize=True)
    if path.stat().st_size < 10000:
        raise RuntimeError("Unexpectedly empty GIF: " + str(path))
    print(f"Generated {path.relative_to(ROOT)} ({len(output)} frames, "
          f"{path.stat().st_size:,} bytes)")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--verify", action="store_true")
    args = parser.parse_args()
    DOCS.mkdir(exist_ok=True)
    # Firmware Font 1 and Font 2 are stored as exact TFT_eSPI bitmap glyphs.
    with tempfile.TemporaryDirectory() as td:
        dest = Path(td)
        binary = dest / "firmware-ui-capture"
        subprocess.run([
            "g++", "-std=c++17", "-O2", "-DSQE_CYD32=1",
            "-I", str(CAPTURE/"include"), "-I", str(ROOT/"include"),
            str(ROOT/"src"/"cyd32_ui.cpp"),
            str(ROOT/"src"/"cyd32_squachy_port.cpp"),
            str(ROOT/"src"/"scenarios.cpp"), str(CAPTURE/"capture.cpp"),
            "-o", str(binary)
        ], check=True)
        subprocess.run([str(binary), str(dest/"frames")], check=True)
        raw = dest/"frames"
        home, duration, menu, active = [images_at(raw/part)
                                        for part in ("home", "duration", "categories", "emitting")]
        # Capture names are sequential per group but continuity is only visual;
        # the screen renderer itself controls all UI animation states.
        gif(home[:19] + duration + home[19:] + menu + active + home[-8:],
            "squachemit-demo.gif")
        gif(duration, "squachemit-tx-time.gif", delay=360)
        gif(menu, "squachemit-device-library.gif")
        gif(active, "squachemit-test-running.gif")
    if args.verify:
        minimums = {"squachemit-demo.gif": 16,
                    "squachemit-tx-time.gif": 6,
                    "squachemit-device-library.gif": 6,
                    "squachemit-test-running.gif": 12}
        for name, minimum in minimums.items():
            p = DOCS/name
            with Image.open(p) as im:
                # Static menu pages are optimized into single GIF frames.
                assert im.n_frames >= minimum, (p, im.n_frames)
                assert im.size == (640,480), (p,im.size)
        print("Frame counts, output dimensions and decodability verified.")


if __name__ == "__main__":
    main()
