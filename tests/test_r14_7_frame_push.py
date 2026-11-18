"""R14.7: native row-span / RGB332 checks plus FramePush integration gates.

Verifies source wiring and the upstream pixel/row arithmetic. Actual FPS,
image correctness and SPI interactions require physical CYD 3.2 ST7798.
"""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "src"

class OptimizedFramePushTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.ui = (SRC / "cyd32_ui.cpp").read_text()
        cls.main = (SRC / "main.cpp").read_text()
        cls.header = (SRC / "cyd32_frame_push.h").read_text()
        cls.push = (SRC / "cyd32_frame_push.cpp").read_text()

    def test_classic_cyd_only_and_s3_c5_untouched(self):
        self.assertIn("#if defined(SQE_CYD32)", self.push)
        self.assertIn("defined(SQE_CYD32) && defined(ARDUINO_ARCH_ESP32)", self.header)
        self.assertIn("namespace Cyd32FramePush", self.header)
        self.assertIn("namespace Cyd32FramePush", self.push)

    def test_full8_only_and_fallback(self):
        ui = self.ui
        self.assertIn('const bool useFast=(bands==1)&&fastPushDesired&&Cyd32FramePush::enabled();',ui)
        self.assertIn('Cyd32FramePush::push(panel,halfFrame.buf(),w,bandH,0,bandFrameTop)',ui)
        self.assertIn("Cyd32FramePush::invalidate();",ui)
        self.assertIn("halfFrame.pushSprite(0,bandFrameTop);",ui)
        self.assertIn("fastPushDesired=false;",ui)
        self.assertIn("sceneFrameBands=1;",ui)
        self.assertIn("halfFrame.createSprite(w,h/2)",ui)

    def test_help_and_ab_commands(self):
        for command in ("PUSH ON", "PUSH OFF", "PUSH STATUS"):
            self.assertIn(command, self.main)
        self.assertIn("cyd32UiSetFramePush(true)",self.main)
        self.assertIn("cyd32UiSetFramePush(false)",self.main)
        self.assertIn("cyd32UiPrintFramePushStatus()",self.main)
        self.assertIn("push_mode=%s,rows=%lu,hash_us=%lu,wire_us=%lu",self.ui)
        self.assertIn('Cyd32FramePush::newFrame();',self.ui)
        self.assertIn('Cyd32FramePush::invalidate();',self.ui)

    def test_upstream_burst_and_changed_row_safeguards(self):
        self.assertIn("PX_PER_BURST = 32",self.push)
        self.assertIn("static uint32_t s_rowHash[MAX_ROWS]",self.push)
        self.assertIn("(s_frameNo % 64) == 0",self.push)
        self.assertIn("frameSpans(changed, h, align, spans, 96)",self.push)
        self.assertIn("SQW_PUSH_CHARGE(s_wireUs, tWire)",self.push)
        self.assertIn("WRITE_PERI_REG",self.push)
        self.assertIn("tft.setAddrWindow(x, y + r0, w, r1 - r0)",self.push)

    @unittest.skipUnless(shutil.which("g++"),"g++ needed for native C++ checks")
    def test_native_color_conversion_and_spans(self):
        code=r"""
#include "cyd32_frame_push.h"
#include <cstdint>
int main(){
  using namespace Cyd32FramePush;
  if(rgb332Wire(0x00)!=0x0000u || rgb332Wire(0xff)!=0xffffu) return 1;
  if(rgb332Wire(0xe0)!=0x00f8u || rgb332Wire(0x03)!=0x1f00u) return 2;
  bool rows[8]={false,true,false,false,false,true,false,false};
  RowSpan out[8]{};
  int n=frameSpans(rows,8,2,out,8);
  if(n!=2 || out[0].r0!=0 || out[0].r1!=2 ||
     out[1].r0!=4 || out[1].r1!=6) return 3;
  bool all[8]={true,true,true,true,true,true,true,true};
  n=frameSpans(all,8,2,out,8);
  if(n!=1 || out[0].r0!=0 || out[0].r1!=8) return 4;
  return 0;
}
"""
        with tempfile.TemporaryDirectory() as d:
            file=Path(d)/"frame.cpp"
            file.write_text(code)
            exe=Path(d)/"frame"
            subprocess.run(["g++","-std=c++17","-O2",
                "-I",str(SRC),"-I",str(ROOT/"tools/readme_capture/include"),
                str(file),"-o",str(exe)],check=True)
            subprocess.run([str(exe)],check=True)

if __name__=="__main__":
    unittest.main()
