"""R14.14 tests — centered text-hugging landscape speech, radio beacon icon."""
import shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CXX=r'''
#include "sqe_bubble_layout.h"
#include <cassert>
int main(){
  auto shortLine=SqeBubbleLayout::bounds(86,0,false,1.f);
  assert(shortLine.w==112);
  assert(shortLine.h==30);
  assert(shortLine.x==104);
  assert(shortLine.y==27);
  assert(SqeBubbleLayout::centeredTextX(shortLine,86)==117);
  auto longLine=SqeBubbleLayout::bounds(276,0,false,1.f);
  assert(longLine.w==302);
  assert(longLine.x==9);
  assert(SqeBubbleLayout::centeredTextX(longLine,276)==22);
  // A two-line error grows only vertically and to the wider of its lines.
  auto error=SqeBubbleLayout::bounds(92,148,true,1.f);
  assert(error.w==174&&error.h==44&&error.y==20);
  assert(SqeBubbleLayout::centeredTextX(error,148)==86);
  auto max=SqeBubbleLayout::bounds(282,282,false,1.03f);
  assert(max.x>=0 && max.x+max.w<=320);
  assert(max.y>=0 && max.y+max.h<=64);
  auto start=SqeBubbleLayout::bounds(150,0,false,0.f);
  assert(start.w==5&&start.h==4);
  assert(start.x>=0&&start.y>=0);
  return 0;
}
'''
class R1414SpeechAndIcon(unittest.TestCase):
 @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),"C++ compiler required")
 def test_measured_layout_center_and_padding(self):
  cc=shutil.which("g++") or shutil.which("clang++")
  with tempfile.TemporaryDirectory() as d:
   s=Path(d)/"bubble.cpp";out=Path(d)/"bubble"
   s.write_text(CXX)
   subprocess.run([cc,"-std=c++17","-O2","-I",str(ROOT/"include"),
     str(s),"-o",str(out)],check=True)
   p=subprocess.run([str(out)],capture_output=True,text=True)
   self.assertEqual(p.returncode,0,p.stderr)
 def test_centered_render_and_new_tx_beacon(self):
  ui=(ROOT/"src/cyd32_ui.cpp").read_text()
  bubble=ui[ui.index("void speechBubble(const char* active"):ui.index("void statusStrip(",ui.index("void speechBubble("))]
  self.assertIn('bubbleLine(speechLine(),primary,sizeof(primary),2,282);',bubble)
  self.assertIn('const int primaryW=lcd.textWidth(primary);',bubble)
  self.assertIn('SqeBubbleLayout::bounds(primaryW,detailW,failed,scale)',bubble)
  self.assertIn('SqeBubbleLayout::centeredTextX(box,primaryW)',bubble)
  self.assertIn('SqeBubbleLayout::centeredTextX(box,detailW)',bubble)
  self.assertNotIn('fit(16,failed?25:35',bubble)
  self.assertNotIn('int w=(int)(306.f*scale)',bubble)
  self.assertNotIn('scenarioDeviceName',bubble)
  self.assertIn('fit(66,200,chosen?friendly:"NONE"',ui)
  self.assertIn('void drawTxBeacon(int cx,int cy,bool transmitting)',ui)
  self.assertIn('drawTxBeacon(298,7,onAirIndicator)',ui)
  self.assertIn('drawTxBeacon(221,13,active&&*active)',ui)
  self.assertIn('lcd.drawFastVLine(cx-11,cy-2,5,col);',ui)
  self.assertIn('lcd.drawFastVLine(cx+11,cy-2,5,col);',ui)
  self.assertNotIn('lcd.drawLine(292,11,298,3,txColor);',ui)
  self.assertNotIn('lcd.drawLine(215,14,221,4,tx);',ui)
 def test_preserve_r1412_and_framepush(self):
  ui=(ROOT/"src/cyd32_ui.cpp").read_text()
  self.assertIn("SqeTouchWizard::runInteractive",ui)
  self.assertIn("TouchFit::toScreen(portraitFit,a,b,0,sx,sy)",ui)
  self.assertIn("Cyd32FramePush::push(panel,halfFrame.buf()",ui)
  self.assertIn("if(portraitMode)portraitPage(active,remaining,suite);",ui)
  self.assertIn("SqeBubble::visible(landscapeBubble,now)",ui)
if __name__=="__main__":unittest.main()
