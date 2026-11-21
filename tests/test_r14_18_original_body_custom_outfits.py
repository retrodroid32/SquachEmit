"""R14.18: custom SquachEmit costumes must use the actual upstream
SquachWatch Squachy body/pose/scene engine, never a solid RGB332 tile."""
from pathlib import Path
import re
import unittest

ROOT=Path(__file__).resolve().parents[1]
UI=ROOT/"src/cyd32_ui.cpp"
SOURCE=ROOT/"lib/SquachWatchOriginal/src/squachy.cpp"
HEADER=ROOT/"lib/SquachWatchOriginal/src/squachy.h"
PLATFORM=ROOT/"platformio.ini"

class OriginalBodyCustomCostumes(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ui=UI.read_text()
        cls.source=SOURCE.read_text()
        cls.header=HEADER.read_text()
        cls.platform=PLATFORM.read_text()

    def original_render_path(self):
        whole=self.ui.split("void drawSquachyFrame(",1)[1].split("void bigSquachy(",1)[0]
        return whole.split("#if defined(SQE_ORIGINAL_SQUACHY)",1)[1].split("#else",1)[0]

    def test_original_engine_enabled_for_cyd32(self):
        env=self.platform.split("[env:cyd32-st7798]",1)[1]
        self.assertIn("-DSQE_ORIGINAL_SQUACHY=1",env)

    def test_all_22_styles_share_one_original_body(self):
        body=self.original_render_path()
        self.assertIn("const int8_t original=(outfitLevel<=2)?0:(int8_t)(outfitLevel-2);",body)
        self.assertIn("Squachy::setOutfitPreview(original);",body)
        self.assertIn("Squachy::setSqemitCustomLook(outfitLevel==1?1:outfitLevel==2?2:0);",body)
        self.assertIn("Squachy::tick(lcd,160,65,116,now,advanceScene,0.3f,false,0,100);",body)
        self.assertIn("Squachy::setSqemitCustomLook(0);",body)
        self.assertIn("Squachy::setOutfitPreview(-1);",body)
        self.assertNotIn("outfitLevel!=1&&outfitLevel!=2",self.ui)
        # Original renderer can draw without a scratch allocation; these
        # markers only appear in an explicitly disabled legacy #else path.
        self.assertNotIn("SquachyPort::render(",body)
        self.assertNotIn("squachSprite",body)
        self.assertNotIn("0xF81E",body)
        self.assertNotIn("pushSprite(",body)
        self.assertNotIn("pushToSprite(",body)

    def test_custom_garment_from_approved_pr61_is_animated_overlay(self):
        self.assertIn("void setSqemitCustomLook(uint8_t look)",self.source)
        self.assertIn("void setSqemitCustomLook(uint8_t look);",self.header)
        custom=self.source.split("static void drawOutfit(",1)[1]
        self.assertLess(custom.index("if(s_sqemitCustomLook){"),
                        custom.index("if (outfit == OutfitId::NONE) return;"))
        self.assertIn("const uint16_t fabric=s_sqemitCustomLook==1?",custom)
        self.assertIn("if(s_sqemitCustomLook==2)",custom)
        self.assertIn("t.fillRoundRect(x-S(13),hy+S(19),S(26),S(18),S(3),fabric);",custom)

    def test_no_scratch_sprite_allocated_on_original_art_board(self):
        begin=self.ui.split("void cyd32UiBegin()",1)[1]
        alloc=begin.split("allocateSceneFrame();",1)[1]
        self.assertIn("#if !defined(SQE_ORIGINAL_SQUACHY)",alloc)
        legacy=alloc.split("#if !defined(SQE_ORIGINAL_SQUACHY)",1)[1].split("#endif",1)[0]
        self.assertIn("squachSprite.createSprite(112,111)",legacy)
        self.assertIn("drawSquachyFrame(emitting,suite,frameNow());",self.ui)

    def test_existing_speech_and_scene_features_unmodified(self):
        self.assertIn("Squachy::holdBubble(true);",self.original_render_path())
        self.assertIn("drawOriginalScene(15,181);",self.ui)
        self.assertIn("Cyd32FramePush::push(panel,halfFrame.buf()",self.ui)
        self.assertIn("SqeQuips::stateTopic(",self.ui)
        self.assertIn("landscapeSpeechText=SqeQuips::choose(",self.ui)

if __name__=="__main__":
    unittest.main()
