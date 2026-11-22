"""R14.10: raw XPT2046 portrait corner rescue, calibrate and serial trace.

The native mock simulates a physical bezel tap rejected by getTouch(), but
still reporting valid raw pressure. It verifies that only the menu responds,
that weak/noisy taps do not cause false presses, and that touch recalibration
can run without erasing settings. Actual pressure/geometry need physical CYD32.
"""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MOCK=ROOT/"tools/readme_capture/include"

CPP=r'''
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "Preferences.h"
#include "cyd32_ui.h"
#include <cassert>
#include <cstdio>
void idle(){
  mock_time_ms+=125;
  assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
}
void touch(int x,int y){
  mock_time_ms+=380;
  MockDisplay::x=x;MockDisplay::y=y;MockDisplay::touch=true;
  assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
  MockDisplay::touch=false; idle();
}
void raw(int x,int y,int pressure){
  mock_time_ms+=380;
  MockDisplay::rawX=x;MockDisplay::rawY=y;MockDisplay::rawZ=pressure;
  MockDisplay::rawActive=true;
  assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
  MockDisplay::rawActive=false;idle();
}
int main(){
  cyd32UiBegin();idle();
  touch(300,8);idle();
  assert(MockDisplay::screen->width()==240);
  cyd32UiSetTouchTrace(true);
  cyd32UiPrintTouchStatus();
  // At the outer top-left physical bezel, TFT_eSPI's unsigned calibrated
  // mapping can reject this entirely even though pressure is reliable.
  raw(80,140,350); // signed x ~-8 and y ~-3, rescue/clamp to (0,0)
  cyd32UiStreamState(nullptr,0,false,true);
  // Navigate BACK on portrait menu through a normal, calibrated press.
  touch(20,299);idle();
  cyd32UiStreamState(nullptr,0,false,true);
  // Low pressure below menu recovery threshold must not open the menu.
  raw(450,280,140);
  cyd32UiStreamState(nullptr,0,false,true);
  // Strong press in the middle of the screen must not be coerced to MENU.
  raw(2000,1800,350);
  cyd32UiStreamState(nullptr,0,false,true);
  // Strong stable raw header press should recover and open MENU again.
  raw(450,280,350);
  cyd32UiStreamState(nullptr,0,false,true);
  // Retrain via five separately held targets and a proof dot.
  MockDisplay::autoWizard=true;
  MockDisplay::wizardEpoch=mock_time_ms;
  cyd32UiCalibratePortraitTouch();
  MockDisplay::autoWizard=false;
  assert(MockDisplay::screen->width()==240);
  cyd32UiSetTouchTrace(false);
  cyd32UiPrintTouchStatus();
  return 0;
}
'''
class PortraitHamburgerTests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "C++ compiler required")
    def test_bezel_edge_raw_rescue_and_no_false_menu(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/"check.cpp"; exe=Path(d)/"check"
            source.write_text(CPP)
            subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
                "-I",str(MOCK),"-I",str(ROOT/"include"),
                str(source),str(ROOT/"src/cyd32_ui.cpp"),
                str(ROOT/"src/cyd32_squachy_port.cpp"),
                str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],check=True)
            done=subprocess.run([str(exe)],capture_output=True,text=True)
            self.assertEqual(done.returncode,0,done.stderr[-2800:])
            self.assertEqual(done.stderr.count("CYD32,TOUCH,MENU_EDGE_RECOVERED"),0)
            self.assertIn("CYD32,TOUCH,5_POINT_VERIFIED_SAVED",done.stderr)
            self.assertEqual(done.stderr.count("UI STATE 1 MENU"),2)
            self.assertEqual(done.stderr.count("UI STATE 1 HOME"),3)
            self.assertIn("CYD32,TOUCH,PORTRAIT_CAL_DONE",done.stderr)
            self.assertIn("CYD32,TOUCH,TRACE,ON",done.stderr)
            self.assertIn("CYD32,TOUCH,TRACE,OFF",done.stderr)

    def test_hardware_gates_and_serial_controls(self):
        ui=(ROOT/"src/cyd32_ui.cpp").read_text()
        main=(ROOT/"src/main.cpp").read_text()
        self.assertIn("recoverPortraitMenuTouch(x,y)",ui)
        self.assertIn("readPortraitRaw(a,b)",ui)
        self.assertIn("TouchFit::toScreen(portraitFit,a,b,0,sx,sy)",ui)
        self.assertIn("portraitTouchData[1]<200",ui)
        self.assertIn("if(px< -20||px>=90||py< -20||py>=70)return false;",ui)
        self.assertIn("if(y<70&&x<90)",ui)
        self.assertIn('CYD32,TOUCH,MENU_EDGE_RECOVERED',ui)
        self.assertIn('CYD32,TOUCH,STATUS',ui)
        self.assertIn('CYD32,TOUCH,TRACE',ui)
        for command in ('CALTOUCH PORTRAIT','TOUCH STATUS','TOUCH TRACE ON',
                        'TOUCH TRACE OFF'):
            self.assertIn(command,main)
        self.assertIn("if(g_active||g_suite||g_playlist)",main)
        self.assertIn("cyd32UiCalibratePortraitTouch()",main)
        self.assertIn("Cyd32FramePush::invalidate();",ui)

if __name__=="__main__":
    unittest.main()
