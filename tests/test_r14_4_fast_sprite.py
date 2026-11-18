"""R14.4: match SquachWatch's proven 8-bit band primitives and pacing.

These guards prove wiring, not actual physical FPS or LCD tearing.  The
on-device CYD32,FRAME,PERF lines are the performance measurements.
"""
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

class Cyd32FastSpriteGuards(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ui=(ROOT/"src/cyd32_ui.cpp").read_text()
        cls.header=(ROOT/"src/cyd32_fast_sprite.h").read_text()
        cls.impl=(ROOT/"src/cyd32_fast_sprite.cpp").read_text()
        cls.setup=(ROOT/"include/cyd32_st7798_user_setup.h").read_text()

    def test_native_fast_sprite_on_band_only(self):
        self.assertIn('class FastSprite : public TFT_eSprite',self.header)
        self.assertIn('FastSprite halfFrame(&panel);',self.ui)
        self.assertIn('TFT_eSprite squachSprite(&panel);',self.ui)
        self.assertIn('paint=&halfFrame;',self.ui)
        self.assertIn('halfFrame.pushSprite(0,bandFrameTop);',self.ui)
        self.assertIn('halfFrame.setViewport(0,-bandFrameTop,w,h,true);',self.ui)
        self.assertIn('void setWindow(int32_t x0',self.header)
        self.assertIn('#if defined(SQE_CYD32)',self.header)
        self.assertIn('#if defined(SQE_CYD32)',self.impl)

    def test_original_squachwatch_optimized_primitives_present(self):
        for name in ('drawPixel','drawFastVLine','drawLine','drawChar'):
            self.assertIn('FastSprite::'+name+'(',self.impl)
        self.assertIn('if (!s_fast || _bpp != 8)',self.impl)
        self.assertIn('TFT_eSprite::drawPixel(x, y, color)',self.impl)

    def test_native_frame_pacing_without_unsafe_spi_overclock(self):
        self.assertIn('frameCadence=animationLevel==0?100u:animationLevel==1?33u:25u;',self.ui)
        self.assertIn('#define SPI_FREQUENCY 40000000',self.setup)
        self.assertIn('CYD32,FRAME,PERF,',self.ui)
        self.assertIn('advanceScene=(pass==0);',self.ui)
        self.assertIn('Theme::drawActiveBackground(lcd,frameNow(),top,bottom,noReceiver,advanceScene)',self.ui)

if __name__=='__main__':
    unittest.main()
