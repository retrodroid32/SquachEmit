"""CYD32 UI R11 hardware-geometry regression: true portrait coordinates,
upstream background cycle, current-page preservation and no RF action."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MOCK=ROOT/"tools/readme_capture/include"
class RotationBackgroundTests(unittest.TestCase):
 def test_landscape_portrait_landscape_and_edge_background_cycle(self):
  cc=shutil.which("g++") or shutil.which("clang++")
  if not cc: self.skipTest("C++ compiler unavailable")
  c=r'''
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "Preferences.h"
#include "cyd32_ui.h"
#include <cassert>
void idle(){
 mock_time_ms+=85;
 assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
}
void touch(unsigned x,unsigned y){
 mock_time_ms+=350;
 MockDisplay::x=x;MockDisplay::y=y;MockDisplay::touch=true;
 assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
 mock_time_ms+=90;
 MockDisplay::touch=false;
 idle();
}
int main(){
 cyd32UiBegin();idle();
 auto* t=MockDisplay::screen;
 assert(t&&t->width()==320&&t->height()==240);
 const Scenario* first=cyd32UiSelection();
 // Default Synthwave (SquachWatch selectable last), left -> Gibson.
 touch(8,100);
 assert(mock_background_saved&&mock_background_value==8);
 touch(316,100);
 assert(mock_background_value==9);
 // R13 restore animation via completed 8-bit band pushes, never
 // direct physical LCD fillScreen in the animation loop.
 const unsigned pushesLandscape=MockDisplay::completedBandPushes;
 const unsigned clearsLandscape=t->fillScreenCalls;
 for(int k=0;k<7;++k)idle();
 assert(MockDisplay::completedBandPushes>pushesLandscape);
 assert(t->fillScreenCalls==clearsLandscape);
 // Header icon rotates the CURRENT Home, without navigating and without RF.
 touch(300,8);idle();
 assert(t->width()==240&&t->height()==320);
 assert(cyd32UiSelection()==first);
 // Portrait left/right edges match the same upstream order.
 touch(8,110);
 assert(mock_background_value==8);
 touch(236,110);
 assert(mock_background_value==9);
 const unsigned pushesPortrait=MockDisplay::completedBandPushes;
 const unsigned clearsPortrait=t->fillScreenCalls;
 for(int k=0;k<7;++k)idle();
 assert(MockDisplay::completedBandPushes>pushesPortrait);
 assert(t->fillScreenCalls==clearsPortrait);
 // Return to landscape, preserving selection and still transmitting nothing.
 touch(221,8);idle();
 assert(t->width()==320&&t->height()==240);
 assert(cyd32UiSelection()==first);
 return 0;
}'''
  with tempfile.TemporaryDirectory() as d:
   source=Path(d)/"test.cpp"; exe=Path(d)/"test"
   source.write_text(c)
   subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
    "-I",str(MOCK),"-I",str(ROOT/"include"),
    str(source),str(ROOT/"src/cyd32_ui.cpp"),
    str(ROOT/"src/cyd32_squachy_port.cpp"),
    str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],check=True)
   subprocess.run([str(exe)],check=True,capture_output=True,text=True)
if __name__=="__main__":unittest.main()
