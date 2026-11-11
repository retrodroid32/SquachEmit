"""Host regression of the actual AP MAC initialization sequence."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

class ApMacSequenceTests(unittest.TestCase):
    def test_ap_mac_order_and_failure_paths(self):
        compiler=shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            self.skipTest("No C++ compiler")
        code=r'''
#include "ap_mac_lifecycle.h"
#include <cassert>
#include <cstring>
#include <string>
#include <vector>
using namespace SquachApMac;
struct Fake {
 std::vector<std::string> calls;
 std::string fail;
 uint8_t stored[6]={};
 bool startSta(){calls.push_back("STA");return fail!="STA";}
 int32_t stop(){calls.push_back("STOP");return fail=="STOP"?3:0;}
 int32_t setApMode(){calls.push_back("AP_MODE");return fail=="AP_MODE"?4:0;}
 int32_t setMac(const uint8_t mac[6]){
  calls.push_back("MAC");if(fail=="MAC")return 5;
  memcpy(stored,mac,6);return 0;
 }
 int32_t restart(){calls.push_back("START");return fail=="START"?6:0;}
 int32_t getMac(uint8_t actual[6]){
  calls.push_back("READ");if(fail=="READ")return 7;
  memcpy(actual,stored,6);
  if(fail=="MISMATCH")actual[5]^=0x02;
  return 0;
 }
};
int main() {
 const uint8_t mac[6]={0x02,0x12,0x34,0x56,0x78,0x9A};
 Fake success;
 assert(configure(success,mac).ok());
 assert((success.calls==std::vector<std::string>{
   "STA","STOP","AP_MODE","MAC","START","READ"}));
 Fake noSta;noSta.fail="STA";
 assert(configure(noSta,mac).stage==Stage::StaMode);
 assert((noSta.calls==std::vector<std::string>{"STA"}));
 Fake noStop;noStop.fail="STOP";
 assert(configure(noStop,mac).stage==Stage::StopForMac);
 assert((noStop.calls==std::vector<std::string>{"STA","STOP"}));
 Fake noMode;noMode.fail="AP_MODE";
 assert(configure(noMode,mac).stage==Stage::ApMode);
 // Mode failure still restarts driver; MAC setting is not attempted.
 assert((noMode.calls==std::vector<std::string>{
   "STA","STOP","AP_MODE","START"}));
 Fake noMac;noMac.fail="MAC";
 assert(configure(noMac,mac).stage==Stage::SetMac);
 assert((noMac.calls==std::vector<std::string>{
   "STA","STOP","AP_MODE","MAC","START"}));
 Fake noRestart;noRestart.fail="START";
 assert(configure(noRestart,mac).stage==Stage::Restart);
 Fake noRead;noRead.fail="READ";
 assert(configure(noRead,mac).stage==Stage::Readback);
 Fake mismatch;mismatch.fail="MISMATCH";
 assert(configure(mismatch,mac).stage==Stage::Readback);
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/"check.cpp"
            binary=Path(tmp)/"check"
            source.write_text(code)
            subprocess.run([compiler,"-std=c++11","-Wall","-Wextra","-Werror",
                            "-I",str(ROOT/"include"),str(source),"-o",str(binary)],
                           check=True,capture_output=True,text=True)
            subprocess.run([str(binary)],check=True,capture_output=True,text=True)

if __name__=="__main__":
    unittest.main()
