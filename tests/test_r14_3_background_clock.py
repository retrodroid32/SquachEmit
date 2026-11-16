"""R14.3: SquachWatch authoritative background frame clock and perf guards.

These source guards prove wiring only. Frame seam and displayed FPS require CYD 3.2
ST7798 hardware testing (not simulated by host TFT mocks).
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class SceneTimingGuards(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ui = (ROOT/"src"/"cyd32_ui.cpp").read_text()
        cls.theme = (ROOT/"lib"/"SquachWatchOriginal"/"src"/"theme.cpp").read_text()

    def test_squachemit_uses_authoritative_upstream_background_wrapper(self):
        body = self.ui.split("void drawOriginalScene(int top,int bottom){",1)[1].split("\n}\n\n#endif",1)[0]
        self.assertIn("Theme::drawActiveBackground(lcd,frameNow(),top,bottom,noReceiver,advanceScene);", body)
        self.assertIn("Settings::previewBackground(bg)", body)
        self.assertIn("backgroundLevel==9 ? 10 : backgroundLevel", body)
        self.assertIn("Theme::setBackgroundFloor(bottom)", body)
        self.assertIn("Theme::clearBackgroundFloor()", body)
        for name in ("drawStarfield","drawSnowfall","drawAquarium","drawFireflies","drawFlyingToasters"):
            self.assertNotIn("Theme::"+name+"(",body)

    def test_second_band_freezes_frame_step(self):
        theme = self.theme.split("void drawActiveBackground(",1)[1].split("const Settings::Background bgNow",1)[0]
        self.assertIn("if (advance)",theme)
        self.assertIn("s_animK = 0.0f;",theme)
        self.assertIn("const uint32_t dt = last ? now - last",theme)
        self.assertIn("advanceScene=(pass==0);",self.ui)
        self.assertIn("logicalFrameMs=frameMs;",self.ui)

    def test_cadence_and_perf_diagnostics(self):
        self.assertIn("frameCadence=animationLevel==0?110u:animationLevel==1?65u:50u;",self.ui)
        self.assertNotIn("frameCadence=animationLevel==0?200u:animationLevel==1?130u:95u;",self.ui)
        self.assertIn("CYD32,FRAME,PERF,",self.ui)
        self.assertIn("sumDrawUs+=drawUs;",self.ui)
        self.assertIn("sumPushUs+=pushUs;",self.ui)
        self.assertIn("if(frameMs-lastReportMs>=5000u)",self.ui)


if __name__=="__main__":
    unittest.main()
