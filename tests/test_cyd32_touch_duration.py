"""Native CYD 3.2 touch regression: no full-screen flash on idle taps or duration +/-."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
MOCK=ROOT/"tools/readme_capture/include"

class Cyd32TouchDurationTests(unittest.TestCase):
    def test_touch_no_flash_and_duration_boundaries(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        if not cc: self.skipTest("Native C++ compiler unavailable")
        cpp=r'''
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "cyd32_ui.h"
#include "Preferences.h"
#include <cassert>
#include <cstdint>
static void idleTick(){
 mock_time_ms+=70;
 cyd32UiTick(nullptr,0,false);
}
static void touchAt(uint16_t x,uint16_t y){
 mock_time_ms+=270;
 MockDisplay::x=x;MockDisplay::y=y;MockDisplay::touch=true;
 cyd32UiTick(nullptr,0,false);
 mock_time_ms+=70;
 MockDisplay::touch=false;
 cyd32UiTick(nullptr,0,false);
}
int main(){
 cyd32UiBegin();
 idleTick();
 assert(cyd32UiDurationMs()==12000);
 TFT_eSPI* lcd=MockDisplay::screen;
 assert(lcd!=nullptr);
 const unsigned initial=lcd->fillScreenCalls;
 // Squachy touch and unused home area must not clear the TFT.
 touchAt(160,100);
 touchAt(280,95);
 assert(lcd->fillScreenCalls==initial);
 // Enter the TX TIME editor through its explicit home chip.
 touchAt(145,189);
 idleTick();
 assert(lcd->fillScreenCalls==initial+1);
 const unsigned editorClears=lcd->fillScreenCalls;
 touchAt(267,107); // + 1
 assert(cyd32UiDurationMs()==13000);
 assert(mock_saved_tx_time==13);
 touchAt(49,108); // - 1
 assert(cyd32UiDurationMs()==12000);
 assert(lcd->fillScreenCalls==editorClears);
 // Clamp at the 3-second min, 30-second max with no full screen clears.
 for(int i=0;i<40;i++)touchAt(45,105);
 assert(cyd32UiDurationMs()==3000);
 for(int i=0;i<40;i++)touchAt(265,105);
 assert(cyd32UiDurationMs()==30000);
 assert(lcd->fillScreenCalls==editorClears);
 touchAt(160,226); // RESET 12
 assert(cyd32UiDurationMs()==12000);
 assert(lcd->fillScreenCalls==editorClears);
 touchAt(267,226); // DONE
 idleTick();
 assert(lcd->fillScreenCalls==editorClears+1);
 // Preference persists over begin() and home screen reflects selected time.
 touchAt(145,189);
 idleTick();
 touchAt(267,107);
 assert(cyd32UiDurationMs()==13000);
 assert(mock_saved_tx_time==13);
 return 0;
}'''
        with tempfile.TemporaryDirectory() as d:
            source=Path(d)/"check.cpp";binary=Path(d)/"check"
            source.write_text(cpp)
            subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
                            "-I",str(MOCK),"-I",str(ROOT/"include"),
                            str(source),str(ROOT/"src/cyd32_ui.cpp"),
                            str(ROOT/"src/cyd32_squachy_port.cpp"),
                            str(ROOT/"src/scenarios.cpp"),"-o",str(binary)],
                           check=True,text=True)
            subprocess.run([str(binary)],check=True,capture_output=True,text=True)
    def test_selected_duration_used_by_run_and_suite(self):
        source=(ROOT/"src/main.cpp").read_text()
        self.assertIn("startScenario(chosen,selectedDurationMs())",source)
        self.assertIn("startScenario(&kScenarios[g_suiteNext++],selectedDurationMs())",source)
        self.assertIn("uint32_t duration = selectedDurationMs();",source)

if __name__=="__main__": unittest.main()
