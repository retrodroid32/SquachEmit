"""R14.32 battery status header mode (% or V) and persistent Settings toggle."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[1]


class BatteryDisplayModeTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.src=(ROOT/"src"/"cyd32_ui.cpp").read_text(encoding="utf-8")
        cls.prefs=(ROOT/"tools"/"readme_capture"/"include"/"Preferences.h").read_text(encoding="utf-8")

    def test_header_has_explicit_percent_and_voltage_branches(self):
        block=self.src.split("void drawBatteryStatus(int x,int y){",1)[1].split(
            "// Small symmetric broadcasting radio glyph",1)[0]
        self.assertIn('if(!valid)snprintf(value,sizeof(value),batteryDisplayVolts?"--.--V":"--%%");',block)
        self.assertIn("else if(batteryDisplayVolts){",block)
        self.assertIn("SqeBatteryCalibration::centiVolts(calibrated)",block)
        self.assertIn('snprintf(value,sizeof(value),"%u.%02uV"',block)
        self.assertIn('snprintf(value,sizeof(value),"~%u%%"',block)
        self.assertIn("SqeBatteryCalibration::linearPercent(calibrated)",block)
        self.assertNotIn("batteryCharging",block)

    def test_mode_is_saved_and_loaded(self):
        self.assertIn("bool batteryDisplayVolts=false;",self.src)
        self.assertIn('prefs.putBool("batshow_v",batteryDisplayVolts);',self.src)
        self.assertIn('batteryDisplayVolts=prefs.getBool("batshow_v",false);',self.src)
        self.assertIn('setBatteryDisplayVolts(!batteryDisplayVolts);',self.src)
        self.assertIn("mock_battery_display_volts",self.prefs)
        self.assertIn('std::strcmp(name,"batshow_v")==0',self.prefs)
        self.assertIn("mock_battery_display_volts=value;",self.prefs)

    def test_settings_calibration_mode_touch_landscape_and_portrait(self):
        src=self.src
        self.assertIn('footer("BACK","RESET",batteryDisplayVolts?"SHOW V":"SHOW %");',src)
        self.assertIn('batteryDisplayVolts?"VOLTS":"PERCENT"',src)
        self.assertIn('button(163,287,74,30,batteryDisplayVolts?"SHOW V":"SHOW %");',src)
        self.assertIn('else if(x>=160)setBatteryDisplayVolts(!batteryDisplayVolts);',src)
        self.assertIn('else setBatteryDisplayVolts(!batteryDisplayVolts);',src)
        self.assertIn('if(x<108){screen=Screen::Settings;settingsMore=false;redraw=true;}',src)

    def test_selected_mode_is_not_confused_with_calibration_or_percent_scale(self):
        self.assertIn("SqeBatteryCalibration::linearPercent(adjusted)",self.src)
        self.assertIn("setBatteryOffsetMv(0);",self.src)
        self.assertIn("SqeBatteryCalibration::correctedMv(batteryMillivolts,batteryOffsetMv)",self.src)
        self.assertIn("3.20 V or below = 0%",(ROOT/"docs"/"R14_31_BATTERY_VOLTAGE_CALIBRATION.md").read_text())
        

if __name__=="__main__":
    unittest.main()
