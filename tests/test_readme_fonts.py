"""Regression checks for authentic TFT_eSPI font rendering in README GIFs."""
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
CAPTURE=ROOT/"tools/readme_capture"

class FontCaptureTests(unittest.TestCase):
    def test_no_dejavu_font_workaround(self):
        source=(CAPTURE/"generate.py").read_text()
        renderer=(CAPTURE/"include/TFT_eSPI.h").read_text()
        self.assertNotIn("DejaVuSansMono",source)
        self.assertNotIn("font5x7.generated.h",renderer)
        self.assertIn("FontPixels::font2_width",renderer)
        self.assertIn("FontPixels::glcd",renderer)
        self.assertIn("FontPixels::font2_bitmap",renderer)

    def test_actual_text_width_and_glyphs(self):
        program=r"""
#include <cassert>
#include <cstdio>
#include "TFT_eSPI.h"
int main(){
 TFT_eSPI screen;
 screen.setTextFont(1);
 assert(screen.textWidth("READY TO TEST.") == 14*6);
 screen.setTextFont(2);
 assert(screen.textWidth("IIII") == 16);
 assert(screen.textWidth("WWWW") == 40);
 assert(screen.textWidth("READY TO TEST. PICK A PROFILE.") <= 285);
 const int width=screen.textWidth("READY TO TEST. PICK A PROFILE.");
 assert(width > 170);
 screen.setTextColor(TFT_WHITE);
 screen.setCursor(10,10);
 screen.print("READY TO TEST. PICK A PROFILE.");
 size_t lit=0;
 for(auto p:screen.pixel)if(p==TFT_WHITE)lit++;
 assert(lit>300 && lit<320*240/2);
 printf("FONT1/2 TEXT WIDTH OK: %d px, %zu lit pixels\n",width,lit);
}
"""
        with tempfile.TemporaryDirectory() as tmp:
            src=Path(tmp)/"test.cpp"; exe=Path(tmp)/"test"
            src.write_text(program)
            subprocess.run(["g++","-std=c++17","-O2","-I",str(CAPTURE/"include"),
                            str(src),"-o",str(exe)],check=True,capture_output=True,text=True)
            subprocess.run([str(exe)],check=True,capture_output=True,text=True)

if __name__=="__main__": unittest.main()
