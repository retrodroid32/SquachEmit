"""Exact manufacturer-prefix on-air tests: 24, 28, and 36-bit allocation boundaries."""
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]

class ExtendedVendorOuiTests(unittest.TestCase):
 def test_catalog_and_oui_only_semantics(self):
  entries=json.loads((ROOT/"tools/uav_vendor_oui_reference.json").read_text())["entries"]
  catalog=json.loads((ROOT/"tests/cases.json").read_text())
  by_id={x["id"]:x for x in catalog}
  self.assertEqual(len(entries),27)
  self.assertEqual(len(catalog),172)
  self.assertEqual(len(by_id),172)
  self.assertEqual(len({x["prefixHex"] for x in entries}),27)
  for item in entries:
   self.assertEqual(len(item["prefixHex"])*4,item["bits"])
   self.assertIn(item["bits"],(24,28,36))
   c=by_id[item["id"]]
   self.assertEqual((c["carrier"],c["kind"],c["code"],c["expected"]),
                    ("WIFI","WifiAp",6,item["expected"]))
   self.assertIn("OUI ONLY; NO RID",c["description"])
   self.assertTrue(c["text"].startswith("SQE-LAB-"))
   self.assertLessEqual(len(c["text"]),32)
  main=(ROOT/"src/main.cpp").read_text()
  self.assertIn("SquachUavVendorLab::find(s->id)",main)
  self.assertIn("SquachUavVendorLab::apply(s->id,out)",main)
  self.assertIn("isOuiOnly(s)",main)
  ui=(ROOT/"src/cyd32_ui.cpp").read_text()
  self.assertIn('scenarioDeviceName(*s,name,sizeof(name))',ui)
  names=(ROOT/"include/scenario_display.h").read_text()
  self.assertIn('sqeStarts(id,"OUI-")',names)
  self.assertIn('NO REMOTE ID',names)

 def test_cpp_prefix_preserves_nibble_and_host_bits(self):
  cc=shutil.which("g++") or shutil.which("clang++")
  if cc is None: self.skipTest("No native C++ compiler")
  cpp=r'''
#include "uav_vendor_oui_lab.h"
#include "identity.h"
#include <cassert>
#include <cstdint>
#include <cstring>
int main() {
 using namespace SquachUavVendorLab;
 static_assert(kCount==27,"27 verified extensions required");
 for(size_t i=0;i<kCount;++i) {
  uint8_t mac[6]={};
  SquachIdentity::derive(0x112233445566ULL,145+i,SquachIdentity::Radio::Wifi,mac);
  uint8_t original[6]; memcpy(original,mac,6);
  assert((mac[0]&3)==2);
  assert(apply(kCases[i].caseId,mac));
  const size_t full=kCases[i].bits/8;
  assert(memcmp(mac,kCases[i].prefix,full)==0);
  if(kCases[i].bits%8) {
    assert((mac[full]&0xF0)==(kCases[i].prefix[full]&0xF0));
    assert((mac[full]&0x0F)==(original[full]&0x0F));
  }
  for(size_t j=full+(kCases[i].bits%8?1:0);j<6;++j)
    assert(mac[j]==original[j]);
  assert((mac[0]&3)==0);
 }
 uint8_t neighbor[6]={2,0,0,0,0,0};
 assert(apply("OUI-INSITU-70B3D57AD",neighbor));
 assert(neighbor[0]==0x70 && neighbor[1]==0xB3 && neighbor[2]==0xD5 &&
        neighbor[3]==0x7A && (neighbor[4]&0xF0)==0xD0);
 uint8_t autel[6]={2,0,0,0x0B,0,0};
 assert(apply("OUI-AUTEL-EC5BCDE",autel));
 assert(autel[0]==0xEC && autel[1]==0x5B && autel[2]==0xCD &&
        autel[3]==0xEB);
 uint8_t unchanged[6]={2,1,2,3,4,5};
 assert(!apply("RID-WIFI-DJI-AIR2S",unchanged));
 assert(unchanged[0]==2);
 assert(!find("OUI-AUTEL-18D7930"));
 return 0;
}'''
  with tempfile.TemporaryDirectory() as d:
   s=Path(d)/"test.cpp";binary=Path(d)/"check";s.write_text(cpp)
   subprocess.run([cc,"-std=c++17","-Wall","-Wextra","-Werror",
                   "-I",str(ROOT/"include"),str(s),"-o",str(binary)],
                  check=True,capture_output=True,text=True)
   subprocess.run([str(binary)],check=True,capture_output=True,text=True)

if __name__=="__main__": unittest.main()
