"""CI guards for CYD32 clear/list UI; not a substitute for hardware."""
import unittest
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
class Cyd32UiGuards(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.s=(ROOT/"src"/"cyd32_ui.cpp").read_text()
        cls.main=(ROOT/"src"/"main.cpp").read_text()
    def test_native_three_screen_structure(self):
        for name in ("Screen::Home","Screen::Categories","Screen::Library",
                     "homeScreen(", "categoriesScreen(", "libraryScreen(",
                     "touchHome(","touchCategories(","touchLibrary(",
                     "pinnedBack(", "titleBar(", "bigSquachy(",
                     '"MENU"', '"RUN"', '"STOP"', '"SUITE"',
                     '"TEST LIBRARY / DEVICE TYPE"', 'deviceCategoryName(',
                     'deviceCategoryCaseAtRank(', '"< PREV"', '"NEXT >"'):
            self.assertIn(name,self.s)
    def test_squachwatch_panel_geometry_and_polarity(self):
        for name in ('lcd.setRotation(1)','lcd.invertDisplay(panelInverted)',
                     '"invert2",true','"touchland"','TFT_RGB_ORDER TFT_BGR'):
            if "TFT_RGB_ORDER" in name:
                self.assertIn(name,(ROOT/"include"/"cyd32_st7798_user_setup.h").read_text())
            else:
                self.assertIn(name,self.s)
        self.assertIn('COLORS',self.main)
        self.assertIn('INVERT ON',self.main)
        self.assertIn('INVERT OFF',self.main)
        self.assertIn('void rgbBars()',self.s)
        self.assertIn('void retroBackdrop()', self.s)
        self.assertIn('SquachyPort::render(', self.s)
        self.assertIn('void speechBubble(',self.s)
        self.assertIn('void statusStrip(',self.s)
        self.assertIn('panelInverted=true;', self.s)
    def test_rf_timeout_and_wizard(self):
        self.assertIn('if((active&&*active)||suite)return Cyd32UiAction::None;',self.s)
        self.assertIn('void countdownOnly(',self.s)
        self.assertIn('countdownOnly(count);',self.s)
        self.assertIn('Cyd32UiAction::Stop',self.s)
        self.assertNotIn('LIST_H=',self.s)
        self.assertNotIn('FILTERS=',self.s)
    def test_filters(self):
        self.assertIn('scenario_library.h',self.s)
        self.assertNotIn('Filter::',self.s)
        self.assertNotIn('"BLUETOOTH / BLE"',self.s)
        self.assertNotIn('"WIFI / BEACONS"',self.s)
        self.assertIn('categoryPage*kPageRows',self.s)
        self.assertIn('deviceCategoryCaseAtRank(',self.s)
    def test_r14_single_speech_bubble_with_original_squachy(self):
        # SquachWatch already provides a built-in speech suppression switch.
        # Its original body/pose/outfit engine must stay enabled; the user
        # should see SquachEmit's top text box, never two stacked bubbles.
        native = (ROOT/"lib"/"SquachWatchOriginal"/"src"/"squachy.cpp").read_text()
        self.assertIn("void holdBubble(bool held) { s_bubbleHeld = held; }", native)
        self.assertIn("if (s_bubbleHeld) return;", native)
        self.assertIn("void speechBubble(", self.s)
        self.assertIn("Squachy::tick(lcd,160,65,116", self.s)
        self.assertIn("Squachy::tick(lcd,120,68,138", self.s)
        self.assertGreaterEqual(self.s.count("Squachy::holdBubble(true);"), 3)

    def test_r9_handheld_navigation_and_touch(self):
        for name in ('Screen::MainMenu','Screen::Outfits','Screen::OtaInfo',
                     'mainMenuScreen()', 'outfitScreen()', 'otaInfoScreen()',
                     '"HEALTH / DIAGNOSTICS"', '"SETTINGS"',
                     '"BLACK HOODIE"', '"sqemitfit"', 'portraitFitLoaded',
                     'TouchFit::toScreen(portraitFit', 'loadCal();',
                     '"outfit"', 'outfitLevel'):
            self.assertIn(name, self.s)
        port=(ROOT/"src"/"cyd32_squachy_port.cpp").read_text()
        self.assertIn('uint8_t outfit',port)
        self.assertIn('BLACK HOODIE:',port)
        self.assertIn('const bool dance=',port)
        self.assertIn('NO SAFE SECOND APP SLOT',self.s)
    def test_r10_chrome_and_responsive_portrait(self):
        for term in ('footer("TESTS"', 'portraitMode=makePortrait',
                     'else if(portraitMode)portraitPage(',
                     'portraitTap(x,y,active&&*active,suite)',
                     'kBackgroundNames[]', 'kOutfitNames[]',
                     'backgroundPage', 'outfitPage',
                     'lcd.fillRoundRect(5,1,27,13,3,kWhite)',
                     'drawTxBeacon(298,7,onAirIndicator)'):
            with self.subTest(term=term):
                self.assertIn(term,self.s)
        self.assertIn('constexpr uint8_t kOutfitCount=22;',self.s)
        sprite=(ROOT/"src"/"cyd32_squachy_port.cpp").read_text()
        for outfit in range(3,22):
            self.assertIn(f'case {outfit}:',sprite)
    def test_source_derived_core(self):
        p=(ROOT/"src"/"cyd32_squachy_port.cpp").read_text()
        for s in ("void render(", "static void wedge(", "const int tuft[4][6]",
                  "slot=now/2600u", "squachSprite"):
            self.assertIn(s,p if s!="squachSprite" else self.s)
        self.assertNotIn("now-lastSceneFrame>=350u",self.s,
            "Physical LCD must not erase a scene every 350ms")
        self.assertIn("SquachWatchScenes::drawDigitalRain",self.s)
        # Only STATIC_FALLBACK may write portrait countdown pixels straight
        # onto the panel. FULL8/HALF8 already paints the status in the frame.
        self.assertIn("if(count!=portraitLastCount)",self.s)
        self.assertIn("active&&*active&&!bandFrameReady",self.s)
        self.assertIn("lcd.fillRect(10,265,225,19,kBg)",self.s)
        self.assertNotIn("lcd.fillRect(8,265,223,20,kBg)",self.s)
        self.assertIn("animationLevel==0?200u:animationLevel==1?120u:80u",self.s)
        self.assertNotIn("label(hx,148,heading",self.s)
if __name__=="__main__":
    unittest.main()
