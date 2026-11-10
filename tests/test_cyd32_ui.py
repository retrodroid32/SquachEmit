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
                     '"ALL TEST CASES"', '"DRONE / REMOTE ID"',
                     '"BLUETOOTH / BLE"', '"WIFI / BEACONS"'):
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
        self.assertIn('lcd.fillRoundRect(x-27,y-2,54,21,5,kInk)', self.s)
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
        self.assertIn('Kind::WifiRemoteId',self.s)
        for name in ("Filter::All","Filter::Drone","Filter::Ble","Filter::Wifi"):
            self.assertIn(name,self.s)
if __name__=="__main__":
    unittest.main()
