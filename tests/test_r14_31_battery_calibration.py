"""R14.31 battery calibration / user-defined linear 3.2V-4.2V scale."""
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]


class BatteryCalibrationTests(unittest.TestCase):
    def test_real_cxx_voltage_offset_and_percent_mapping(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        if not cc:
            self.skipTest("C++ compiler unavailable")
        code=r"""
#include "sqe_battery_calibration.h"
#include <assert.h>
int main(){
  using namespace SqeBatteryCalibration;
  assert(clampOffset(-900)==-500);
  assert(clampOffset(900)==500);
  assert(clampOffset(150)==150);
  assert(correctedMv(3950,100)==4050);
  assert(correctedMv(3950,-100)==3850);
  assert(correctedMv(0,-500)==0);
  assert(correctedMv(4900,500)==5000);
  assert(linearPercent(3000)==0);
  assert(linearPercent(3200)==0);
  assert(linearPercent(3700)==50);
  assert(linearPercent(4000)==80);
  assert(linearPercent(4050)==85);
  assert(linearPercent(4200)==100);
  assert(linearPercent(4010)==81);
  assert(linearPercent(4040)==84);
  assert(centiVolts(4024)==402);  // 4.024V -> 4.02V
  assert(centiVolts(4025)==403);  // 4.025V -> 4.03V
  assert(centiVolts(4099)==410);  // 4.099V -> 4.10V
  assert(centiVolts(correctedMv(3950,100))==405);
  assert(linearPercent(4300)==100);
  assert(linearPercent(correctedMv(3950,100))==85);
  assert(linearPercent(3200)==0);
  assert(linearPercent(3700)==50);
  assert(linearPercent(4000)==80);
  assert(linearPercent(4200)==100);
  return 0;
}
"""
        with tempfile.TemporaryDirectory() as td:
            folder=pathlib.Path(td)
            cpp=folder/"battery_cal.cpp"
            exe=folder/"battery_cal"
            cpp.write_text(code)
            subprocess.run([cc,"-std=c++17","-Wall","-Wextra","-Werror",
                            "-I",str(ROOT/"include"),str(cpp),"-o",str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

    def test_ui_calibration_entry_and_persistence(self):
        src=(ROOT/"src"/"cyd32_ui.cpp").read_text(encoding="utf-8")
        fixture=(ROOT/"tools"/"readme_capture"/"include"/"Preferences.h").read_text(encoding="utf-8")
        self.assertIn("BatteryCalibration",src)
        self.assertIn('footer("BACK","MORE >","BATTERY");',src)
        self.assertIn("void batteryCalibrationScreen(){",src)
        self.assertIn("Cyd32UiAction touchBatteryCalibration(int x,int y){",src)
        self.assertIn('setBatteryOffsetMv(0);',src)
        self.assertIn('setBatteryOffsetMv((int32_t)batteryOffsetMv+(x<160?-10:10));',src)
        self.assertIn('setBatteryOffsetMv((int32_t)batteryOffsetMv+(x<160?-50:50));',src)
        self.assertIn('prefs.putInt("batoff_mv"',src)
        self.assertIn('prefs.getInt("batoff_mv",0)',src)
        self.assertIn('if(screen==Screen::BatteryCalibration||settingsSubmenuFlow)',src)
        self.assertIn('if(x<108){backFromSettingsSubmenu();}',src)
        self.assertIn("screen==Screen::BatteryCalibration)batteryCalibrationScreen()",src)
        self.assertIn("screen==Screen::BatteryCalibration)action=touchBatteryCalibration(x,y)",src)
        self.assertIn("screen==Screen::BatteryCalibration){",src)
        self.assertIn("mock_battery_offset_mv",fixture)
        self.assertIn("int32_t getInt(",fixture)
        self.assertIn("size_t putInt(",fixture)

    def test_header_uses_calibrated_voltage_for_percent_and_gauge(self):
        src=(ROOT/"src"/"cyd32_ui.cpp").read_text(encoding="utf-8")
        start=src.index("void drawBatteryStatus(")
        end=src.index("void drawTxBeacon(",start)
        battery=src[start:end]
        self.assertIn("const uint16_t calibrated=correctedBatteryMv();",battery)
        self.assertIn("SqeBatteryCalibration::linearPercent(calibrated)",battery)
        self.assertIn('snprintf(value,sizeof(value),"~%u%%"',battery)
        self.assertIn("const int fill=(int)((unsigned)percent*13u/100u);",battery)
        self.assertNotIn("batteryCharging",battery)


if __name__=="__main__":
    unittest.main()
