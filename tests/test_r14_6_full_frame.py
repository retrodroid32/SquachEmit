"""R14.6 full RGB332 frame path -- source guards for buffer/viewport safety.

Full-sized framebuffer is a *hardware-dependent allocation*. CI verifies
that it renders once if allocated and that low-memory devices keep a safe
HALF8 two-band fallback. Only the physical ST7798 can confirm the seam.
"""
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

class FullSceneFramebufferTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.code=(ROOT/"src/cyd32_ui.cpp").read_text()

    def test_allocates_full_rgb332_first_and_preserves_low_memory_fallback(self):
        c=self.code
        f=c[c.index("void allocateSceneFrame(){"):c.index("void homeScreen(const char* active",c.index("void allocateSceneFrame(){"))]
        self.assertIn("halfFrame.setColorDepth(8)",f)
        self.assertLess(f.index("halfFrame.createSprite(w,h)"),f.index("halfFrame.createSprite(w,h/2)"))
        self.assertIn("sceneFrameBands=1;",f)
        self.assertIn("sceneFrameBands=2;",f)
        self.assertIn("afterFull>=64u*1024u",f)
        self.assertIn("largestFull>=24u*1024u",f)
        self.assertIn("if(fullCreated)halfFrame.deleteSprite();",f)
        self.assertIn("fullCandidate=%u",f)
        self.assertIn('"FULL8":"HALF8"',f)

    def test_full_scene_does_one_render_and_one_push_per_frame(self):
        c=self.code
        start=c.index("void renderBufferedHome(")
        stop=c.index("void allocateSceneFrame(){",start)
        renderer=c[start:stop]
        self.assertIn("const int bands=sceneFrameBands,bandH=h/bands;",renderer)
        self.assertIn("for(int pass=0;pass<bands;++pass)",renderer)
        self.assertIn("bandFrameTop=pass*bandH;",renderer)
        self.assertIn("advanceScene=(pass==0);",renderer)
        self.assertIn("DrawBand::set(bandFrameTop,bandFrameTop+bandH);",renderer)
        self.assertIn("halfFrame.setViewport(0,-bandFrameTop,w,h,true);",renderer)
        self.assertIn("halfFrame.pushSprite(0,bandFrameTop);",renderer)
        self.assertIn("logicalFrameMs=frameMs;",renderer)
        self.assertIn("DrawBand::setEnabled(false);",renderer)

    def test_init_and_rotation_reuse_exact_allocator(self):
        c=self.code
        rotate=c[c.index("void rotateCurrentScreen(){"):c.index("bool loadCal()",c.index("void rotateCurrentScreen(){"))] if False else c[c.index("void rotateCurrentScreen(){"):c.index("if(makePortrait){",c.index("void rotateCurrentScreen(){"))]
        self.assertIn("allocateSceneFrame();",rotate)
        self.assertIn("ROTATED_%s",rotate)
        boot=c[c.index("  // Allocate the animation band FIRST"):]
        self.assertLess(boot.index("allocateSceneFrame();"),boot.index("squachSprite.createSprite(112,111)"))
        self.assertIn("if(!bandFrameReady)return;",c)

if __name__=="__main__":
    unittest.main()
