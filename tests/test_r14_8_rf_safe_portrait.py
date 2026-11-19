"""R14.8: protect RF timeout/STOP from blocking first portrait calibration.

Native harness verifies first portrait rotate is refused while TX or a
suite is active, the display remains in landscape, idle calibration works,
and once calibration is saved a transmitter may rotate without re-running
the blocking wizard. It does not prove real TFT SPI or RF behavior.
"""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MOCK = ROOT / "tools/readme_capture/include"

CPP = r'''
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "Preferences.h"
#include "cyd32_ui.h"
#include <cassert>
#include <cstring>
static void tick(const char* id=nullptr,bool suite=false){
  mock_time_ms+=350;
  assert(cyd32UiTick(id,id?2500:0,suite)==Cyd32UiAction::None);
}
static void touch(unsigned x,unsigned y,const char* id=nullptr,bool suite=false){
  mock_time_ms+=350;
  MockDisplay::x=x;MockDisplay::y=y;MockDisplay::touch=true;
  assert(cyd32UiTick(id,id?2500:0,suite)==Cyd32UiAction::None);
  mock_time_ms+=100;MockDisplay::touch=false;
  assert(cyd32UiTick(id,id?2500:0,suite)==Cyd32UiAction::None);
}
int main(int argc,char** argv){
  const bool useSuite=(argc>1&&std::strcmp(argv[1],"suite")==0);
  cyd32UiBegin();tick();
  auto* screen=MockDisplay::screen;
  assert(screen && screen->width()==320 && screen->height()==240);
  // Start an RF case or suite, then attempt uncalibrated portrait rotation.
  tick(useSuite?nullptr:"BLE-AIRTAG",useSuite);
  touch(300,8,useSuite?nullptr:"BLE-AIRTAG",useSuite);
  assert(screen->width()==320 && screen->height()==240);
  // The rejected gesture must not damage the radio UI state.
  tick(useSuite?nullptr:"BLE-AIRTAG",useSuite);
  assert(screen->width()==320 && screen->height()==240);
  // Finish RF, complete normal first-time portrait touch setup.
  tick();
  touch(300,8);
  assert(screen->width()==240 && screen->height()==320);
  // Return while a case is active: no touch wizard is needed when cached.
  tick("BLE-AIRTAG");
  touch(221,8,"BLE-AIRTAG");
  assert(screen->width()==320 && screen->height()==240);
  tick("BLE-AIRTAG");
  touch(300,8,"BLE-AIRTAG");
  assert(screen->width()==240 && screen->height()==320);
  return 0;
}
'''

class RfSafePortraitTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "C++ compiler required")
    def test_native_active_tx_suite_and_saved_portrait_calibration(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/"check.cpp"
            exe=Path(d)/"check"
            source.write_text(CPP)
            subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
                 "-I",str(MOCK),"-I",str(ROOT/"include"),
                 str(source),str(ROOT/"src/cyd32_ui.cpp"),
                 str(ROOT/"src/cyd32_squachy_port.cpp"),
                 str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],check=True)
            for mode in ("active","suite"):
                with self.subTest(mode=mode):
                    completed=subprocess.run([str(exe),mode],text=True,
                                              capture_output=True,check=True)
                    self.assertIn("CYD32,ROTATE,DEFER_TOUCH_CAL_RF_ACTIVE",
                                  completed.stderr)
                    self.assertIn("CYD32,ROTATE,PORTRAIT",completed.stderr)
                    self.assertIn("CYD32,ROTATE,LANDSCAPE",completed.stderr)

    def test_calibration_and_color_bars_reject_any_rf_work(self):
        main=(ROOT/"src/main.cpp").read_text()
        rotate=(ROOT/"src/cyd32_ui.cpp").read_text()
        self.assertIn('ERROR,CALTOUCH_REQUIRES_IDLE',main)
        self.assertIn('if (g_active || g_suite || g_playlist)',main)
        self.assertIn('ERROR,COLORS_REQUIRES_IDLE',main)
        self.assertIn('(onAirIndicator||previousSuite||playlistRunning||txDriverActive)',rotate)
        self.assertLess(rotate.index('CYD32,ROTATE,DEFER_TOUCH_CAL_RF_ACTIVE'),
                        rotate.index('panel.setRotation(makePortrait?0:1)'))
        self.assertIn('Cyd32FramePush::invalidate();',rotate)
        self.assertIn('allocateSceneFrame();',rotate)

    def test_radio_still_uses_bounded_start_stop_and_suite_gaps(self):
        source=(ROOT/"src/main.cpp").read_text()
        self.assertIn('static constexpr uint32_t kSuiteGapMs = 2400;',source)
        self.assertIn('if (g_active && now - g_started >= g_duration)',source)
        self.assertIn('static void cancelAll()',source)
        self.assertIn('finish("stopped");',source)
        self.assertIn('stopTx();',source)
        self.assertIn('startScenario(&kScenarios[g_suiteNext++],selectedDurationMs())',source)
        self.assertIn('startScenario(chosen,selectedDurationMs())',source)

if __name__ == "__main__":
    unittest.main()
