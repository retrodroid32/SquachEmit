"""Compile DJI OUI fixture mapping and validate all 10 prefixes."""
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
import json

ROOT=Path(__file__).resolve().parents[1]
EXPECTED=[
    ("04A85A","04:A8:5A"),
    ("0C9AE6","0C:9A:E6"),
    ("34D262","34:D2:62"),
    ("481CB9","48:1C:B9"),
    ("4C43F6","4C:43:F6"),
    ("58B858","58:B8:58"),
    ("60601F","60:60:1F"),
    ("882985","88:29:85"),
    ("8C5823","8C:58:23"),
    ("E47A2C","E4:7A:2C"),
]

class DjiOuiOnAirTests(unittest.TestCase):
    def test_ten_ui_transmitter_cases_and_no_remoteid_data(self):
        cases=json.loads((ROOT/"tests/cases.json").read_text())
        by_id={x["id"]:x for x in cases}
        self.assertEqual(len(cases),172)
        for key,prefix in EXPECTED:
            case=by_id["OUI-DJI-"+key]
            self.assertEqual((case["carrier"],case["kind"],case["expected"],case["code"]),
                             ("WIFI","WifiAp","DRONE",6))
            self.assertIn(prefix,case["description"])
            self.assertIn("NO RID",case["description"])
            self.assertTrue(case["text"].startswith("SQE-LAB-DJI-"))
        src=(ROOT/"src/main.cpp").read_text()
        self.assertIn("SquachDjiOuiLab::find(s->id)",src)
        self.assertIn("SquachDjiOuiLab::apply(s->id,out)",src)
        self.assertIn("TEST,OUI_ONLY",src)
        self.assertIn("WiFi.softAP(s->text",src)
        self.assertIn("beginWifiIdentity(s)",src)
        ui=(ROOT/"src/cyd32_ui.cpp").read_text()
        self.assertIn('startsWith(s->id,"OUI-DJI-")',ui)
        self.assertIn("NO REMOTE ID",ui)

    def test_cpp_fixed_prefixes_unique_suffix_and_non_oui_original(self):
        cxx=shutil.which("g++") or shutil.which("clang++")
        if not cxx:self.skipTest("C++ compiler unavailable")
        program=r'''
#include "dji_oui_lab.h"
#include "identity.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main(){
  using namespace SquachDjiOuiLab;
  static_assert(kCount==10, "DJI OUI LAB catalog must contain 10 entries");
  uint8_t suffixes[kCount][3]={};
  for(size_t i=0;i<kCount;++i){
    uint8_t mac[6]={};
    SquachIdentity::derive(0xAABBCCDDEEFFULL,i+135,SquachIdentity::Radio::Wifi,mac);
    assert(mac[0]==0x02);
    assert(apply(kCases[i].caseId,mac));
    assert(memcmp(mac,kCases[i].prefix,3)==0);
    assert((mac[0]&3)==0);
    memcpy(suffixes[i],mac+3,3);
    for(size_t j=0;j<i;++j)
      assert(memcmp(suffixes[j],suffixes[i],3)!=0);
    uint8_t mismatch[6]={0x02,1,2,3,4,5};
    assert(!apply("RID-WIFI-DJI-AIR2S",mismatch));
    assert(mismatch[0]==0x02);
  }
  assert(!find("OUI-DJI-FFFFFFFF"));
  assert(!find(nullptr));
  return 0;
}'''
        with tempfile.TemporaryDirectory() as d:
            src=Path(d)/"test.cpp"; exe=Path(d)/"test"
            src.write_text(program)
            subprocess.run([cxx,"-std=c++17","-Wall","-Wextra","-Werror",
                            "-I",str(ROOT/"include"),str(src),"-o",str(exe)],
                           capture_output=True,text=True,check=True)
            subprocess.run([str(exe)],capture_output=True,text=True,check=True)
if __name__=="__main__":unittest.main()
