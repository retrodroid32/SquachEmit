"""Regression gates for the CYD32 R14.26 status header."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SOURCE = (ROOT / "src" / "cyd32_ui.cpp").read_text(encoding="utf-8")


class R1426HeaderBatteryTests(unittest.TestCase):
    def test_title_width_is_not_taken_from_stale_sprite_font_metrics(self):
        start = SOURCE.index("void titleBar()")
        end = SOURCE.index("void applyTheme()", start)
        title = SOURCE[start:end]
        self.assertIn('static constexpr char kAppTitle[]="SQUACHEMIT";', title)
        self.assertIn('const int nameX=(kW-nameW)/2;', title)
        self.assertIn('const int nameW=(int)(sizeof(kAppTitle)-1)*kTitleGlyphW;', title)
        self.assertNotIn('textWidth("SQUACHEMIT")', title)
        self.assertIn("lcd.setTextSize(1);", title)

    def test_battery_voltage_uses_documented_voltage_indicator(self):
        start = SOURCE.index("void drawBatteryStatus(")
        end = SOURCE.index("void drawTxBeacon(", start)
        battery = SOURCE[start:end]
        self.assertIn('snprintf(value,sizeof(value),"%u.%02uV"', battery)
        self.assertNotIn('snprintf(value,sizeof(value),"~%d%%"', battery)
        self.assertIn("displayedMv/1000u",battery)
        self.assertIn("50u*50u",battery)
        self.assertIn('lcd.fillRect(x+1,y-5,15,10,kTaskbar);', battery)
        self.assertIn('lcd.fillRect(x+2,y-4,fill,8,chargeCol);', battery)
        self.assertIn('lcd.drawRect(x,y-6,17,12,kWhite);', battery)

    def test_battery_gauge_has_no_charging_symbols(self):
        start = SOURCE.index("void drawBatteryStatus(")
        end = SOURCE.index("void drawTxBeacon(", start)
        battery = SOURCE[start:end]
        self.assertNotIn("batteryCharging", battery)
        self.assertNotIn("charging", battery.lower())
        self.assertNotIn("kYellow", battery)
        self.assertNotIn('label(x+7,y-4,', battery)
        self.assertNotIn("SQE_BAT_CHARGE_PIN", SOURCE)

    def test_battery_reading_awaits_multiple_stable_windows(self):
        self.assertIn("batteryPercent<0?3000u:15000u", SOURCE)
        self.assertIn("batteryMeter.accept(batteryAdcSamples)", SOURCE)
        self.assertIn("consensus_windows=", SOURCE)


if __name__ == "__main__":
    unittest.main()
