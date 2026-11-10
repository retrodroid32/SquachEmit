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
                     '"TESTS"', '"RUN"', '"STOP"', '"SUITE"',
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
    def test_source_derived_core(self):
        p=(ROOT/"src"/"cyd32_squachy_port.cpp").read_text()
        for s in ("void render(", "static void wedge(", "const int tuft[4][6]",
                  "slot=now/2600u", "squachSprite"):
            self.assertIn(s,p if s!="squachSprite" else self.s)
        self.assertIn("lastSquachFrame>=120u",self.s)
        self.assertNotIn("label(hx,148,heading",self.s)
if __name__=="__main__":
    unittest.main()
