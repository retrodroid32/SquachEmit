"""Static behavioral guards for 3.2-inch emitter UI; not hardware proof."""
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


class Cyd32UiGuards(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = (ROOT / "src" / "cyd32_ui.cpp").read_text()

    def test_two_distinct_screens(self):
        for pattern in ('enum class Screen', 'Screen::Home', 'Screen::Library',
                        'homeScreen(', 'libraryScreen(', 'touchHome(', 'touchLibrary('):
            self.assertIn(pattern, self.source)
        self.assertIn('label(9,5,"SQUACHEMIT"', self.source)
        self.assertIn('bigSquachy(emitting,suite)', self.source)
        for label in ('"TESTS"', '"RUN"', '"STOP"', '"SUITE"', '"HOME"', '"< PREV"', '"NEXT >"'):
            self.assertIn(label, self.source)

    def test_firmware_only_controls_rf_on_user_action(self):
        main = (ROOT / "src" / "main.cpp").read_text()
        for pattern in ('Cyd32UiAction::Run', 'Cyd32UiAction::Stop',
                        'Cyd32UiAction::Suite'):
            self.assertIn(pattern, main)
        self.assertIn('if((active&&*active)||suite)return Cyd32UiAction::None;', self.source)

    def test_flicker_and_calibration_guards(self):
        self.assertIn('void countdownOnly(', self.source)
        self.assertIn('countdownOnly(count);', self.source)
        self.assertIn('"touchland"', self.source)
        self.assertIn('lcd.setRotation(1)', self.source)
        self.assertNotIn('fillScreen(kBg);\n  char buf[16]', self.source)
        self.assertNotIn('LIST_H=', self.source)
        self.assertNotIn('FILTERS=', self.source)

    def test_drone_ble_wifi_filters(self):
        for label in ('"ALL"', '"DRONE"', '"BLE"', '"WIFI"'):
            self.assertIn(label, self.source)
        self.assertIn('Kind::WifiRemoteId', self.source)


if __name__ == "__main__":
    unittest.main()
