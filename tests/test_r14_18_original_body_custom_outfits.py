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

    def test_portrait_all_styles_use_original_body(self):
        ui=self.ui
        portrait=ui.split("void portraitPage(",1)[1].split("for(int i=0;i<4;++i)",1)[0]
        self.assertIn("Squachy::setOutfitPreview(outfitLevel<=2?0:(int8_t)(outfitLevel-2));",portrait)
        self.assertIn("Squachy::setSqemitCustomLook(outfitLevel==1?1:outfitLevel==2?2:0);",portrait)
        self.assertIn("Squachy::tick(lcd,120,68,138,frameNow(),advanceScene,0.3f,false,0,100);",portrait)
        self.assertIn("Squachy::setSqemitCustomLook(0);",portrait)
        self.assertIn("Squachy::setOutfitPreview(-1);",portrait)
        self.assertNotIn("outfitLevel!=1&&outfitLevel!=2",portrait)
        original=portrait.split("#if defined(SQE_ORIGINAL_SQUACHY)",1)[1].split("#else",1)[0]
        self.assertNotIn("SquachyPort::render(",original)
        self.assertNotIn("0xF81E",original)

    def test_custom_garments_follow_original_body_and_animated_shoulders(self):
        # R14.19 corrects the first original-body port's misplaced overlay:
        # drawOutfit receives hh (head anchor), not hy (body anchor).
        self.assertIn("void setSqemitCustomLook(uint8_t look)",self.source)
        self.assertIn("void setSqemitCustomLook(uint8_t look);",self.header)
        torso=self.source.split("static void drawBody(",1)[1]
        original_custom=self.source.split("static void drawOutfit(",1)[1].split(
            "static void drawBody(",1)[0]
        self.assertNotIn("if(s_sqemitCustomLook)",original_custom)
        self.assertIn("const int top = hy + S(23);",torso)
        self.assertIn("S(2 * th), S(18), S(5), fabric",torso)
        self.assertIn("s_armL0x,s_armL0y,s_armL1x,s_armL1y",torso)
        self.assertIn("s_armR0x,s_armR0y,s_armR1x,s_armR1y",torso)
        self.assertIn("const int hhLive = hy + s_headDrop + actHead;",torso)
        # First paint the chest, then moving sleeves, then original face.
        self.assertLess(torso.index("const int top = hy + S(23);"),
                        torso.index("auto dressArm ="))
        self.assertLess(torso.index("auto dressArm ="),
                        torso.index("// ---- head group"))
        self.assertLess(torso.index("// ---- head group"),
                        torso.index("drawOutfit(t, cx2, hh"))
        self.assertNotIn("hy+S(19),S(26),S(18)",original_custom)

    def test_native_hoodie_has_hood_strings_and_kangaroo_pocket(self):
        art=self.source.split("static void drawBody(",1)[1]
        self.assertIn("if (hoodie)",art)
        self.assertIn("t.fillRoundRect(cx2-S(10), hy+S(32), S(20), S(7)",art)
        self.assertIn("t.drawFastVLine(cx2-S(3), hy+S(25)",art)
        self.assertIn("t.drawFastVLine(cx2+S(3), hy+S(25)",art)
        self.assertIn("t.fillRoundRect(cx2-S(19),hhLive-S(2),S(38),S(29)",art)
        self.assertIn("t.drawRoundRect(cx2-S(17),hhLive,S(34),S(25)",art)
        # A pullover should not have the old central zipper.
        self.assertNotIn("const uint16_t zipper=s_sqemitCustomLook",art)

    def test_native_lab_jacket_has_true_collar_lapels_pockets_and_hem(self):
        art=self.source.split("static void drawBody(",1)[1]
        self.assertIn("const uint16_t undershirt",art)
        self.assertIn("t.fillTriangle(cx2-S(10),top",art)
        self.assertIn("t.fillTriangle(cx2+S(10),top",art)
        self.assertIn("t.fillRoundRect(cx2-S(11),hy+S(35)",art)
        self.assertIn("t.fillRoundRect(cx2+S(5),hy+S(35)",art)
        self.assertIn("t.drawFastHLine(cx2-S(th)+S(2),hy+S(43)",art)
        self.assertIn("t.drawFastHLine(cx2+S(2),hy+S(43)",art)

    def test_sleeves_match_native_rectangles_and_pose_widths(self):
        art=self.source.split("static void drawBody(",1)[1]
        native=art.split("auto limbTo =",1)[1].split("// Arms —",1)[0]
        sleeve=art.split("// R14.21: dress the EXACT silhouette",1)[1].split(
            "// Native layered hood",1)[0]
        # A resting original arm is S(8) wide x S(22) tall, and
        # posed arms explicitly record the native S(7)/S(8) stroke width.
        self.assertIn("int armWidthL = S(8), armWidthR = S(8);",art)
        self.assertIn("bool armRectL = true, armRectR = true;",art)
        self.assertIn("armWidthL = ww; armRectL = false;",native)
        self.assertIn("armWidthR = ww; armRectR = false;",native)
        self.assertIn("armWidthL = S(8); armWidthR = S(8);",native)
        self.assertIn("armRectL = true; armRectR = true;",native)
        self.assertIn("const int x = x0-S(4), y = y0-S(2);",sleeve)
        self.assertIn("const int w = S(8), h = S(22);",sleeve)
        self.assertIn("t.fillRoundRect(x,y,w,h,S(3),sleeve);",sleeve)
        self.assertIn("const int ex=x0+94*(x1-x0)/100;",sleeve)
        self.assertIn("const int w=nativeWidth>1?nativeWidth:2;",sleeve)
        self.assertNotIn("4*(x1-x0)/5",sleeve)
        self.assertNotIn("wideLine(t,x0,y0,ex,ey,w+2,outline)",sleeve)
        self.assertNotIn("fillCircle",sleeve)
        self.assertIn("dressArm(s_armL0x",sleeve)
        self.assertIn("dressArm(s_armR0x",sleeve)
        # Outfit colors and other garment details remain those of R14.20.
        self.assertIn("t.color565(43, 46, 88)",art)
        self.assertIn("t.color565(43,46,88)",art)

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
