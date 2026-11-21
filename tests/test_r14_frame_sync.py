"""R14.2 regression checks for CYD32 half-frame timing and real portrait Squachy.

Source-level checks complement physical TFT validation; host mocks do not emulate
TFT_eSprite pixel offsets or timing of the actual ST7798 panel.
"""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class OriginalArtBandSync(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ui = (ROOT / "src" / "cyd32_ui.cpp").read_text()

    def test_both_bands_share_one_animation_clock(self):
        s = self.ui
        self.assertIn("logicalFrameMs=frameMs;", s)
        self.assertIn("uint32_t frameNow(){return bandFrameActive?logicalFrameMs:millis();}", s)
        self.assertIn("Theme::drawActiveBackground(lcd,frameNow(),top,bottom,noReceiver,advanceScene);", s)
        self.assertIn("drawSquachyFrame(emitting,suite,frameNow());", s)
        self.assertNotIn("Squachy::tick(lcd,120,68,138,millis()", s)
        self.assertNotIn("Squachy::tick(lcd,160,65,116,millis()", s)
        self.assertIn("Squachy::tick(lcd,120,68,138,frameNow()", s)

    def test_second_band_does_not_advance_character(self):
        s = self.ui
        start = s.index("void renderBufferedHome(")
        renderer = s[start:s.index("void homeScreen(", start)]
        self.assertIn("advanceScene=(pass==0);", renderer)
        self.assertIn("DrawBand::setEnabled(true);", renderer)
        self.assertIn("DrawBand::set(bandFrameTop,bandFrameTop+bandH);", renderer)
        self.assertIn("DrawBand::setEnabled(false);", renderer)

    def test_original_squachy_is_not_gated_by_custom_sprite(self):
        s = self.ui
        section = s[s.index("void bigSquachy("):s.index("void rightStatus(")]
        self.assertLess(section.index("drawSquachyFrame(emitting,suite,frameNow());"),
                        section.index("if(squachSpriteReady)drawSquachyFrame"))

    def test_portrait_original_character_is_outside_background_else(self):
        s = self.ui
        portrait = s[s.index("void portraitPage(const char* active,uint32_t remaining,bool suite){"):]
        split = portrait.index("#endif\n#if defined(SQE_ORIGINAL_SQUACHY)\n")
        self.assertIn("drawOriginalScene(60,215);", portrait[:split])
        self.assertIn("Squachy::tick(lcd,120,68,138,frameNow()", portrait[split:])
        self.assertIn("Squachy::holdBubble(true);", portrait[split:])


if __name__ == "__main__":
    unittest.main()
