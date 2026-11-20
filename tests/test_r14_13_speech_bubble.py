"""R14.13: SquachWatch-timed speech bubble: never repeat profile, pop
on a randomized cadence, disappear entirely, preserve full-frame rendering.
"""
import shutil,subprocess,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CPP=r"""
#include "sqe_bubble_timing.h"
#include <cassert>
#include <cmath>
static long zero(long a,long){return a;}
static long maxMinusOne(long a,long b){return b-1;}
int main(){
  SqeBubble::State s{};
  // The initial quip is random-delayed, never a permanent overlay.
  assert(SqeBubble::update(s,1000,SqeBubble::Idle,zero));
  assert(!SqeBubble::visible(s,1000));
  assert(s.next==16000);
  assert(!SqeBubble::update(s,15999,SqeBubble::Idle,zero));
  assert(SqeBubble::update(s,16000,SqeBubble::Idle,zero));
  assert(SqeBubble::visible(s,16000));
  assert(s.phrase<5);
  assert(SqeBubble::popScale(s,16000)==0.f);
  assert(SqeBubble::popScale(s,16220)==1.f);
  assert(!SqeBubble::update(s,20999,SqeBubble::Idle,zero));
  assert(SqeBubble::update(s,21000,SqeBubble::Idle,zero));
  assert(!SqeBubble::visible(s,21000));
  assert(s.next==30000);
  assert(SqeBubble::update(s,30000,SqeBubble::Idle,zero));
  assert(SqeBubble::visible(s,30000));
  // RF state changes announce promptly but do NOT remain up forever.
  assert(SqeBubble::update(s,31000,SqeBubble::OnAir,zero));
  assert(SqeBubble::visible(s,31000));
  assert(s.phrase<3);
  assert(SqeBubble::update(s,36000,SqeBubble::OnAir,zero));
  assert(!SqeBubble::visible(s,36000));
  assert(SqeBubble::update(s,37000,SqeBubble::Failed,zero));
  assert(SqeBubble::visible(s,37000));
  assert(SqeBubble::update(s,42000,SqeBubble::Failed,zero));
  assert(!SqeBubble::visible(s,42000));
  // SquachWatch random interval is 9s..22s AFTER bubble hides.
  assert(SqeBubble::update(s,90000,SqeBubble::Idle,maxMinusOne));
  assert(s.next==119999);
  assert(SqeBubble::update(s,119999,SqeBubble::Idle,maxMinusOne));
  assert(SqeBubble::update(s,124999,SqeBubble::Idle,maxMinusOne));
  assert(s.next==146998);
  // 220-ms pop never exceeds a safe 3% overshoot.
  SqeBubble::State pop{};
  SqeBubble::update(pop,1000,SqeBubble::OnAir,zero);
  assert(SqeBubble::popScale(pop,1165)>1.f);
  assert(SqeBubble::popScale(pop,1165)<1.031f);
  assert(SqeBubble::popScale(pop,1220)==1.f);
  return 0;
}
"""
class SpeechBubbleR1413Tests(unittest.TestCase):
  @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),"native C++ compiler required")
  def test_random_visibility_and_context_timing(self):
    cc=shutil.which("g++") or shutil.which("clang++")
    with tempfile.TemporaryDirectory() as d:
      f=Path(d)/"main.cpp"; out=Path(d)/"main"
      f.write_text(CPP)
      subprocess.run([cc,"-std=c++17","-O2","-I",str(ROOT/"include"),str(f),"-o",str(out)],check=True)
      proc=subprocess.run([str(out)],capture_output=True,text=True)
      self.assertEqual(proc.returncode,0,proc.stderr)
  def test_no_selected_profile_in_bubble_and_no_direct_frame_overlays(self):
    ui=(ROOT/"src/cyd32_ui.cpp").read_text()
    bubble=ui[ui.index("void speechBubble(const char* active"):ui.index("void statusStrip(",ui.index("void speechBubble("))]
    self.assertNotIn("scenarioDeviceName",bubble)
    self.assertNotIn("NO PROFILE",bubble)
    self.assertNotIn("showing?friendly",bubble)
    self.assertNotIn("lastBubbleAt",ui)
    self.assertIn('fit(66,200,chosen?friendly:"NONE"',ui)  # profile stays in status
    self.assertIn("if(!SqeBubble::visible(landscapeBubble,now))return;",bubble)
    self.assertIn("SqeBubble::popScale",bubble)
    self.assertIn("advanceLandscapeBubble(now,active,suite);",ui)
    self.assertIn("if(changed&&!portraitMode&&screen==Screen::Home&&!bandFrameReady)",ui)
    self.assertIn("Squachy::holdBubble(true);",ui)   # never double-overlay
    self.assertNotIn("invalidate();speechBubble(",ui)
    self.assertIn("if(portraitMode)portraitPage(active,remaining,suite);",ui)
    self.assertIn("else homeScreen(active,remaining,suite);",ui)
if __name__=="__main__":unittest.main()
