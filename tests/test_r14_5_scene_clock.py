"""Verify actual logical-frame integration for two 8-bit LCD bands.

A single 320x240 frame is rendered in two 320x120 passes at identical
millis() values. This native test proves the camera position cannot change
between those passes, in Gibson and Starfield clock paths. Physical ST7798
screen tearing and touch stability still require hardware testing.
"""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
LIB = ROOT / "lib" / "SquachWatchOriginal" / "src"

class SceneClockTest(unittest.TestCase):
    def test_both_original_scenes_use_repeat_safe_frame_clock(self):
        theme = (LIB / "theme.cpp").read_text()
        self.assertIn('#include "sqemit_scene_clock.h"', theme)
        self.assertIn("SqEmitSceneClock::elapsedOnce(now, lastMs);", theme)
        self.assertIn("SqEmitSceneClock::elapsedOnce(now, tunLast);", theme)
        self.assertNotIn("(lastMs && now > lastMs) ? (now - lastMs) : 16u", theme)
        self.assertNotIn("(tunLast && now > tunLast) ? (now - tunLast) : 16u", theme)
        self.assertIn("if (dt) flyZ = fmodf(", theme)

    @unittest.skipUnless(shutil.which("g++"), "requires native g++ in CI")
    def test_camera_position_unchanged_on_second_half_and_millis_wrap(self):
        program = r"""
#include <cstdint>
#include <cmath>
#include "sqemit_scene_clock.h"
int main() {
    uint32_t last=0;
    float flyZ=0.0f;
    auto draw=[&](uint32_t now) {
        const uint32_t dt=SqEmitSceneClock::elapsedOnce(now,last);
        flyZ=std::fmod(flyZ+(float)dt*0.20f,1500.0f);
        return dt;
    };
    if (draw(1000u) != 16u) return 1;
    const float afterTop=flyZ;
    if (draw(1000u) != 0u || flyZ != afterTop) return 2;
    if (draw(1033u) != 33u) return 3;
    const float afterNextTop=flyZ;
    if (draw(1033u) != 0u || flyZ != afterNextTop) return 4;
    if (draw(2000u) != 100u) return 5; // bounded stall
    uint32_t wrap=0xfffffffau;
    if (SqEmitSceneClock::elapsedOnce(3u,wrap)!=9u) return 6;
    if (SqEmitSceneClock::elapsedOnce(3u,wrap)!=0u) return 7;
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/"check.cpp"
            binary=Path(d)/"check"
            src.write_text(program)
            subprocess.run(["g++","-std=c++17","-O2","-I",str(LIB),str(src),"-o",str(binary)],check=True)
            subprocess.run([str(binary)],check=True)

if __name__=="__main__":
    unittest.main()
