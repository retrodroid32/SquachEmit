"""Native C++ OpenDroneID Wi-Fi vendor IE regression and source guard."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class WifiRidPackTests(unittest.TestCase):
    def test_pack_5_message_envelope(self):
        cxx = shutil.which("g++") or shutil.which("clang++")
        if not cxx:
            self.skipTest("Native C++ compiler unavailable")
        cpp = r'''
#include "rid_wifi_pack.h"
#include <cassert>
#include <cstring>
int main(){
    using namespace SquachRidWifi;
    static_assert(kIeBytes==135, "incorrect length");
    uint8_t msgs[kMsgCount][kMsgBytes] = {};
    const uint8_t types[5] = {0x02,0x12,0x42,0x32,0x52};
    for(size_t i=0;i<kMsgCount;++i){
      msgs[i][0]=types[i];
      msgs[i][1]=(uint8_t)i;
      msgs[i][24]=(uint8_t)(0xA0+i);
    }
    uint8_t output[kIeBytes+2]; memset(output,0xCC,sizeof(output));
    assert(!encode(msgs, 7, output, kIeBytes-1));
    assert(encode(msgs,7,output,kIeBytes));
    assert(output[0]==0xDD);
    assert(output[1]==133);        // IE data length, NOT full IE byte length
    assert(output[2]==0xFA && output[3]==0x0B &&
           output[4]==0xBC && output[5]==0x0D);
    assert(output[6]==7);          // beacon service-info message counter
    assert(output[7]==0xF2 && output[8]==25 && output[9]==5);
    for(size_t i=0;i<5;i++){
      assert(output[10+i*25]==types[i]);
      assert(output[10+i*25+24]==(uint8_t)(0xA0+i));
    }
    assert(output[kIeBytes]==0xCC && output[kIeBytes+1]==0xCC);
    msgs[0][0]=0xF2;               // message pack cannot contain a nested pack
    assert(!encode(msgs,7,output,kIeBytes));
    msgs[0][0]=0x01;               // unsupported protocol version
    assert(!encode(msgs,7,output,kIeBytes));
    return 0;
}'''
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/"check.cpp"
            exe=Path(tmp)/"test"
            source.write_text(cpp)
            subprocess.run([cxx,"-std=c++17","-I",str(ROOT/"include"),
                            str(source),"-o",str(exe)],
                           check=True,capture_output=True,text=True)
            subprocess.run([str(exe)],check=True,capture_output=True,text=True)

    def test_firmware_uses_pack_not_basic_id_only(self):
        main=(ROOT/"src/main.cpp").read_text()
        self.assertIn('#include "rid_wifi_pack.h"',main)
        self.assertIn('SquachRidWifi::encode(msgs,1,ie,sizeof(ie))',main)
        self.assertIn('remoteFrame((uint8_t)i,frame,s)',main)
        self.assertIn('"SQE-LAB-OPERATOR"',main)
        self.assertNotIn('uint8_t ie[35] =',main)
        self.assertNotIn('memcpy(ie + 10, body + 2, 25)',main)

if __name__=="__main__":
    unittest.main()
