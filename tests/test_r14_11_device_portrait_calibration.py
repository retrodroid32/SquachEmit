"""R14.11 on-device portrait touchscreen recovery.

Explicitly tests landscape SETTINGS > MORE > CAL PORTRAIT TOUCH when
portrait NVS appears already saved (but may be wrong), visible Recovery
button and a raw-pressure 5s portrait hold independent of touch mapping.
Hardware touch accuracy still requires physical board testing.
"""
import shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MOCK=ROOT/"tools/readme_capture/include"
CPP=r"""
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "Preferences.h"
#include "cyd32_ui.h"
#include <cassert>
#include <cstring>
static void tick(const char* active=nullptr,bool suite=false,int ms=95){
 mock_time_ms+=ms;
 assert(cyd32UiTick(active,active?3000:0,suite)==Cyd32UiAction::None);
}
static void touch(int x,int y){
 mock_time_ms+=350;
 MockDisplay::x=x;MockDisplay::y=y;MockDisplay::touch=true;
 assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
 mock_time_ms+=110; MockDisplay::touch=false;
 assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
}
static void nextMenu(){
 touch(22,12);tick();       // MainMenu
 touch(260,223);tick();     // page 2: HEALTH, SETTINGS, OUTFITS, OTA
}
int main(int argc,char** argv){
 const bool hold=argc>1&&!std::strcmp(argv[1],"hold");
 const bool recovery=argc>1&&!std::strcmp(argv[1],"recovery");
 cyd32UiBegin();tick();
 assert(MockDisplay::screen->width()==320);
 assert(mock_portrait_touch_saved); // firmware believes bad old fit is valid
 assert(MockDisplay::calibrateTouchCalls==0);
 if(hold){
   touch(299,8);tick(); // portrait with old saved data, no wizard
   assert(MockDisplay::screen->width()==240);
   assert(MockDisplay::calibrateTouchCalls==0);
   // First ensure idle-only guard: hold for 6.5 seconds during Suite.
   MockDisplay::rawX=1850;MockDisplay::rawY=1900;
   MockDisplay::rawZ=420;MockDisplay::rawActive=true;
   for(int k=0;k<46;k++)tick(nullptr,true,155);
   assert(MockDisplay::calibrateTouchCalls==0);
   MockDisplay::rawActive=false;tick();
   // Raw touch is NOT accepted by calibrated getTouch(); the long-hold
   // timer must still work, without choosing any menu or RF action.
   MockDisplay::rawActive=true;
   for(int k=0;k<46;k++)tick(nullptr,false,155);
   MockDisplay::rawActive=false;tick();
   assert(MockDisplay::calibrateTouchCalls==1);
   assert(MockDisplay::screen->width()==240);
   assert(mock_portrait_touch_saved);
 }else if(recovery){
   nextMenu();
   touch(265,223);tick();    // page 3
   touch(145,143);tick();    // RECOVERY, third row
   cyd32UiStreamState(nullptr,0,false,true); // RECOVERY
   touch(135,170);tick();    // visible CAL PORTRAIT TOUCH
   assert(MockDisplay::screen->width()==240);
   assert(MockDisplay::calibrateTouchCalls==1);
   cyd32UiStreamState(nullptr,0,false,true); // HOME
 }else{
   nextMenu();
   touch(135,108);tick(); // SETTINGS, second row on page 2
   cyd32UiStreamState(nullptr,0,false,true); // SETTINGS
   touch(163,224);tick(); // MORE page
   touch(145,181);tick(); // CAL PORTRAIT TOUCH, fourth row
   assert(MockDisplay::screen->width()==240);
   assert(MockDisplay::calibrateTouchCalls==1);
   assert(mock_portrait_touch_saved);
   cyd32UiStreamState(nullptr,0,false,true); // HOME
 }
 return 0;
}
"""
class PortraitRecoveryTests(unittest.TestCase):
  @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),"C++ compiler needed")
  def test_landscape_settings_recovery_and_unmapped_hold(self):
    cc=shutil.which("g++") or shutil.which("clang++")
    with tempfile.TemporaryDirectory() as tmp:
      src=Path(tmp)/"test.cpp";exe=Path(tmp)/"test"
      src.write_text(CPP)
      subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
         "-I",str(MOCK),"-I",str(ROOT/"include"),
         str(src),str(ROOT/"src/cyd32_ui.cpp"),
         str(ROOT/"src/cyd32_squachy_port.cpp"),
         str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],check=True)
      for scenario in ("settings","recovery","hold"):
        with self.subTest(scenario=scenario):
          r=subprocess.run([str(exe),scenario],capture_output=True,text=True)
          self.assertEqual(r.returncode,0,
             f"{scenario} native test failed:\n{r.stderr[-2200:]}")
          if scenario=="hold":
            self.assertIn("CYD32,TOUCH,RAW_HOLD_CAL",r.stderr)
            self.assertIn("CYD32,TOUCH,PORTRAIT_CAL_DONE",r.stderr)
          else:
            self.assertIn("CYD32,TOUCH,PORTRAIT_CAL_START",r.stderr)
            self.assertIn("UI STATE 1 HOME",r.stderr)
  def test_source_does_not_trust_old_calibration_during_forced_setup(self):
    ui=(ROOT/"src/cyd32_ui.cpp").read_text()
    self.assertIn('menuRow(163,"CAL PORTRAIT TOUCH"',ui)
    self.assertIn('void calibratePortraitFromDevice()',ui)
    self.assertIn('forcePortraitCalibration=true;',ui)
    self.assertIn('if(!forcePortraitCalibration&&prefs.begin("sqemit32",true))',ui)
    self.assertIn('if(!portraitCalibrated||forcePortraitCalibration)',ui)
    self.assertIn('portraitRawHoldStarted>=5000u',ui)
    self.assertIn('HOLD SCREEN 5s: CAL TOUCH',ui)
    self.assertIn('CYD32,TOUCH,CAL_REQUIRES_IDLE',ui)
    self.assertIn('if(i==2)calibratePortraitFromDevice();',ui)
    self.assertIn('CAL PORTRAIT TOUCH',ui)
    self.assertIn('Cyd32FramePush::invalidate();',ui)
if __name__=="__main__":unittest.main()
