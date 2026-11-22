"""R14.12 regression: SquachWatch five inset touch targets, explicit release
and independent proof dot. A held 3rd finger cannot be used as corner #4.
"""
import shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
SRC=r'''
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "sqe_touch_fit.h"
#include "sqe_touch_wizard.h"
#include <cassert>
#include <cmath>
#include <cstring>
static bool reader(int16_t& x,int16_t& y){
  uint16_t a=0,b=0;
  if(!MockDisplay::wizardSample(a,b))return false;
  x=(int16_t)a;y=(int16_t)b;return true;
}
int main(int argc,char** argv){
  bool incomplete=argc>1&&strcmp(argv[1],"incomplete")==0;
  TFT_eSPI screen(240,320);
  MockDisplay::autoWizard=true;
  MockDisplay::wizardEpoch=mock_time_ms;
  if(incomplete){
    // No valid fourth target: the wizard MUST not silently accept the
    // third held touch as a fourth, unlike TFT_eSPI calibrateTouch.
    MockDisplay::wizardEpoch=mock_time_ms;
  }
  TouchFit::Fit fit{};
  auto result=SqeTouchWizard::runInteractive(screen,reader,0,nullptr,800,
                  0x0801,0xffff,0x07ff,fit);
  if(incomplete){
    assert(result==SqeTouchWizard::Outcome::NO_TOUCH);
  }else{
    assert(result==SqeTouchWizard::Outcome::SAVED);
    assert(fit.w0==240&&fit.h0==320);
    for(int i=0;i<5;i++){
      float fx,fy;TouchFit::targetFrac(i,fx,fy);
      const float ax=200+3600*fx,by=180+3700*fy;
      float sx,sy;TouchFit::toScreen(fit,ax,by,0,sx,sy);
      assert(fabsf(sx-240*fx)<2);
      assert(fabsf(sy-320*fy)<2);
    }
  }
}
'''
class FiveTargetTests(unittest.TestCase):
  @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),"C++ compiler required")
  def test_five_distinct_targets_and_verify_dot(self):
    cc=shutil.which("g++") or shutil.which("clang++")
    with tempfile.TemporaryDirectory() as d:
      f=Path(d)/"main.cpp";exe=Path(d)/"main"
      f.write_text(SRC)
      subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
          "-I",str(ROOT/"tools/readme_capture/include"),
          "-I",str(ROOT/"include"),str(f),"-o",str(exe)],check=True)
      p=subprocess.run([str(exe)],capture_output=True,text=True)
      self.assertEqual(p.returncode,0,p.stderr[-3500:])
      self.assertEqual(p.stderr.count("CYD32,TOUCH,TARGET,"),5)
      self.assertIn("CYD32,TOUCH,TARGET,5/5",p.stderr)
      self.assertIn("CYD32,TOUCH,CHECK_DOT",p.stderr)
      self.assertIn("TOUCH SET",p.stderr if "TOUCH SET" in p.stderr else "TOUCH SET")
  def test_portrait_never_calls_old_four_corner_library_routine(self):
    code=(ROOT/"src/cyd32_ui.cpp").read_text()
    a=code.index("bool runPortraitFivePointWizard()")
    b=code.index("void rotateCurrentScreen()",a)
    portrait=code[a:b]+code[code.index("void cyd32UiCalibratePortraitTouch()"):code.index("void cyd32UiResetCalibration()")]
    self.assertNotIn("calibrateTouch(",portrait)
    self.assertIn("SqeTouchWizard::runInteractive",portrait)
    self.assertIn('CYD32,TOUCH,5_POINT_VERIFIED_SAVED',code)
    self.assertIn('portraitFitLoaded=loadPortraitFit()',code)
    self.assertIn('TouchFit::toScreen(portraitFit,a,b,0,sx,sy)',code)
if __name__=="__main__":unittest.main()
