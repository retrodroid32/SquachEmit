"""R14.9: pixel and touch regression for accessible portrait menu,
no unexplained HOME caption and one-source buffered TX countdown.
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

static void tick(const char* active=nullptr,uint32_t remaining=0,bool suite=false){
  mock_time_ms+=140;
  assert(cyd32UiTick(active,remaining,suite)==Cyd32UiAction::None);
}
static Cyd32UiAction touch(int x,int y,const char* active=nullptr,
                            uint32_t remaining=0,bool suite=false){
  mock_time_ms+=350;
  MockDisplay::x=x;MockDisplay::y=y;MockDisplay::touch=true;
  auto action=cyd32UiTick(active,remaining,suite);
  mock_time_ms+=120;MockDisplay::touch=false;
  assert(cyd32UiTick(active,remaining,suite)==Cyd32UiAction::None);
  return action;
}
static unsigned titlePixels(){
  auto* lcd=MockDisplay::screen;
  unsigned pixels=0;
  for(int y=37;y<55;++y)
    for(int x=12;x<70;++x)
      if(lcd->pixel[size_t(y)*240+x]!=0x0801)++pixels;
  return pixels;
}
int main(){
  cyd32UiBegin();tick();
  auto* lcd=MockDisplay::screen;
  assert(lcd && lcd->width()==320 && lcd->height()==240);
  touch(300,8);tick();
  assert(lcd->width()==240 && lcd->height()==320);
  // No "HOME" floating above the animated original scene.
  assert(titlePixels()==0);
  // Bottom-left of 65x42 hit area must open menu even if outside old 45x27.
  assert(touch(58,31)==Cyd32UiAction::None);
  tick();assert(titlePixels()>0);
  cyd32UiStreamState(nullptr,0,false,true); // UI STATE 1 MENU
  // Menu navigation to a real page remains functional.
  assert(touch(110,78)==Cyd32UiAction::None);
  tick();
  cyd32UiStreamState(nullptr,0,false,true); // UI STATE 1 CATEGORIES
  // Top-left menu and back work on submenu.
  touch(30,12);tick();
  assert(titlePixels()>0);
  touch(20,300);tick();
  assert(titlePixels()==0);
  // Simulate bounded TX while portrait HOME is buffered. The old bug
  // drew direct landscape countdown + portrait countdown over the sprite.
  const unsigned directBefore=MockDisplay::directPortraitCountdownWrites;
  const unsigned pushesBefore=MockDisplay::completedBandPushes;
  tick("BLE-AIRTAG",7000);
  tick("BLE-AIRTAG",6200);
  tick("BLE-AIRTAG",5100);
  tick("BLE-AIRTAG",4000);
  assert(MockDisplay::completedBandPushes>pushesBefore);
  assert(MockDisplay::directPortraitCountdownWrites==directBefore);
  // Users may open the menu while TX runs, and it MUST remain there.
  touch(58,31,"BLE-AIRTAG",4000);
  tick("BLE-AIRTAG",3900);
  assert(titlePixels()>0);
  cyd32UiStreamState("BLE-AIRTAG",3900,false,true); // UI STATE 1 MENU
  // Middle footer on all portrait pages must stay a live STOP action.
  assert(touch(120,299,"BLE-AIRTAG",3900)==Cyd32UiAction::Stop);
  tick();
  touch(20,299);tick(); // return from MENU to Home
  assert(titlePixels()==0);
  // While a suite is running, the same center STOP must stay reachable.
  tick(nullptr,0,true);
  touch(58,31,nullptr,0,true);
  tick(nullptr,0,true);
  assert(titlePixels()>0);
  assert(touch(120,299,nullptr,0,true)==Cyd32UiAction::Stop);
  tick();
  return 0;
}
'''
class PortraitR149Tests(unittest.TestCase):
    @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),
                         "C++ compiler required")
    def test_portrait_menu_touch_no_home_and_tx_countdown_single_source(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        with tempfile.TemporaryDirectory() as tmp:
            src=Path(tmp)/"check.cpp"
            exe=Path(tmp)/"check"
            src.write_text(CPP)
            subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
                "-I",str(MOCK),"-I",str(ROOT/"include"),
                str(src),str(ROOT/"src/cyd32_ui.cpp"),
                str(ROOT/"src/cyd32_squachy_port.cpp"),
                str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],check=True)
            completed=subprocess.run([str(exe)],text=True,capture_output=True)
            self.assertEqual(completed.returncode,0,
                f"native portrait regression failed: {completed.stderr[-2500:]}")
            self.assertGreaterEqual(completed.stderr.count("UI STATE 1 MENU"),2)
            self.assertIn("UI STATE 1 CATEGORIES",completed.stderr)
            self.assertIn("UI STATE 1 MENU",completed.stderr)

    def test_pixel_ownership_and_rf_stop_guards(self):
        src=(ROOT/"src/cyd32_ui.cpp").read_text()
        self.assertIn('if(screen!=Screen::Home)label(12,37,name,kVaporPink,2);',src)
        self.assertIn('if(y<70&&x<90)',src)
        self.assertIn('if(y>=287)',src)
        self.assertIn('return Cyd32UiAction::Stop;',src)
        self.assertIn('!bandFrameReady&&!portraitMode',src)
        self.assertIn('active&&*active&&!bandFrameReady',src)
        self.assertNotIn('lcd.fillRect(8,265,223,20,kBg);',src)
        self.assertIn('Cyd32FramePush::invalidate();',src)
        self.assertIn('const bool radioBusy=(active&&*active)||suite||playlistRunning;',src)
        self.assertIn('if(((active&&*active)||suite)&&screen!=Screen::Home&&!portraitMode)',src)
        self.assertIn('Cyd32FramePush::push(panel,halfFrame.buf(),w,bandH,0,bandFrameTop)',src)

if __name__=="__main__":
    unittest.main()
