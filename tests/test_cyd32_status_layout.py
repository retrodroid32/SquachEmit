"""Native TFT pixel regression: right status never overlaps active countdown."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
MOCK=ROOT/"tools/readme_capture/include"

class Cyd32StatusLayoutTests(unittest.TestCase):
    def test_exactly_one_text_baseline_in_right_slot(self):
        cxx=shutil.which("g++") or shutil.which("clang++")
        if not cxx:
            self.skipTest("Native compiler unavailable")
        cpp=r'''
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "cyd32_ui.h"
#include "scenarios.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

static void expectRightStatus(const char* text, uint16_t color){
    TFT_eSPI expected(320,240,false);
    expected.fillScreen(0x0801);
    expected.setTextFont(1);
    expected.setTextSize(1);
    expected.setTextColor(color);
    expected.setCursor(320-12-expected.textWidth(text),186);
    expected.print(text);
    TFT_eSPI* actual=MockDisplay::screen;
    if(!actual){std::fprintf(stderr,"No rendered display\n");std::abort();}
    for(int y=183;y<198;y++){
      for(int x=236;x<320;x++){
        const size_t i=(size_t)y*320+x;
        if(actual->pixel[i]!=expected.pixel[i]){
          std::fprintf(stderr,
            "Right status mismatch at (%d,%d) got %04x expected %04x in '%s'\n",
            x,y,(unsigned)actual->pixel[i],(unsigned)expected.pixel[i],text);
          std::abort();
        }
      }
    }
}
int main(){
  cyd32UiBegin();
  mock_time_ms += 200;
  cyd32UiTick(nullptr,0,false);
  char total[24]; std::snprintf(total,sizeof(total),"%u CASES",(unsigned)kScenarioCount);
  expectRightStatus(total,0xFB99);
  mock_time_ms += 200;
  cyd32UiTick("RID-DJI-MAVIC2PRO",7000,false);
  expectRightStatus("7s LEFT",0xF96F);
  mock_time_ms += 1000;
  cyd32UiTick("RID-DJI-MAVIC2PRO",6000,false);
  expectRightStatus("6s LEFT",0xF96F);
  mock_time_ms += 1000;
  cyd32UiTick(nullptr,0,false);
  expectRightStatus(total,0xFB99);
  return 0;
}'''
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/"check.cpp"
            exe=Path(tmp)/"test"
            source.write_text(cpp)
            subprocess.run([cxx,"-std=c++17","-O2","-DSQE_CYD32=1",
                            "-I",str(MOCK),"-I",str(ROOT/"include"),
                            str(source),str(ROOT/"src/cyd32_ui.cpp"),
                            str(ROOT/"src/cyd32_squachy_port.cpp"),
                            str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],
                            check=True,capture_output=True,text=True)
            subprocess.run([str(exe)],check=True,capture_output=True,text=True)

    def test_old_overlapping_baseline_is_gone(self):
        source=(ROOT/"src/cyd32_ui.cpp").read_text()
        self.assertIn("void rightStatus(bool emitting,uint32_t count)",source)
        self.assertIn("if(!(active&&*active))rightStatus(false,0);",source)
        self.assertIn("rightStatus(true,count);",source)
        self.assertNotIn('label(247,193,buf,kPink)',source)
        self.assertNotIn('label(244,186,totalText,kVaporPink)',source)

if __name__=="__main__":
    unittest.main()
