"""SquachWatch v1.50.0 source-pinned SquachEmit R14.35 LAB regression.

Do not confuse source/host compatibility with hardware detection validation.
"""
import json
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
SQUACHWATCH_TAG="v1.50.0"
EXPECTED_SHA="d359f580048e4f8ea059dea9c95fdd53ee5afec7"

class V150SignatureCoverage(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.manifest=json.loads((ROOT/"tests/squachwatch_v150_manifest.json").read_text())
        cls.cases=json.loads((ROOT/"tests/cases.json").read_text())
        cls.byid={c["id"]:c for c in cls.cases}

    def test_source_pinned_complete_signature_tables(self):
        m=self.manifest
        self.assertEqual(m["tag"],SQUACHWATCH_TAG)
        self.assertEqual(m["signature_blob_sha"],EXPECTED_SHA)
        expected={"oui":151,"uuid":26,"mfg":9,"ssid":27,"bt_name":7}
        self.assertEqual({k:len(x) for k,x in m["tables"].items()},expected)
        self.assertEqual(sum(expected.values()),220)
        represented=sum(bool(x["fixture_ids"]) for table in m["tables"].values() for x in table)
        self.assertEqual(m["table_covered_count"],represented)
        self.assertGreaterEqual(represented,105)
        for table in m["tables"].values():
            for rule in table:
                for id in rule["fixture_ids"]:
                    self.assertIn(id,self.byid)
                    self.assertEqual(self.byid[id]["expected"],rule["category"])
        self.assertEqual(m["physical_receiver_validation"],"NOT PERFORMED")

    def test_catalog_is_bounded_and_includes_new_family_tests(self):
        self.assertEqual(len(self.cases),248)
        self.assertEqual(len(self.byid),248)
        self.assertEqual(len([x for x in self.cases if x["id"].startswith("V150-")]),66)
        self.assertLess(len(self.cases),254)  # per-case identity index limit
        positive={c["expected"] for c in self.cases if c["expected"]!="NONE"}
        self.assertEqual(positive,{
            "FLOCK","AXON","META","SKIMMER","RAVEN","AIRTAG","DRONE",
            "ALPR","CAMERA","SAMSUNG_TAG","GOOGLE_TAG","TILE","RING",
            "IBEACON","HACKER","LISTENER"
        })
        self.assertFalse(positive & {"LORAWAN","DEAUTH","EVILTWIN","MINE"})
        for id in ("V150-SSID-BLACKVUE","V150-OUI-RING-AC9FC3",
                   "V150-SSID-GOOGLEHOME","V150-BLE-HOMEPOD-SIRI",
                   "V150-BLE-GOOGLE-FMDN40","V150-BLE-AIRTAG-NEAR",
                   "V150-BLE-AIRPODS-NEG","V150-OUI-ALPR-00BF15"):
            self.assertIn(id,self.byid)

    def test_all_24_outdated_expectations_corrected(self):
        none={
            "BLE-HACKER-MFG","BLE-META-TECH","BLE-SKIM-BT05",
            "BLE-SKIM-JDY08","BLE-SKIM-JDY16","BLE-SKIM-MLTBT",
            "BLE-SKIM-LINVOR","WIFI-HACKER-PAGER1","WIFI-HACKER-PAGER2",
            "OUI-AUTEL-18D7936","OUI-AUTEL-EC5BCDE",
            "OUI-QUANTUM-AC86D17","OUI-INSITU-0050C2C35",
            "OUI-INSITU-0050C2FD4","OUI-INSITU-70B3D57AD",
            "OUI-INSITU-70B3D5865","OUI-INSITU-70B3D5B3B",
            "OUI-INSITU-70B3D5D36","OUI-INSITU-70B3D5DD2",
            "OUI-XAG-A45129","OUI-HUBSAN-98AAFC7",
            "OUI-YUNEEC-E0B6F58","OUI-FIMI-6CDFFBE"
        }
        self.assertEqual(len(none),23)
        for id in none:self.assertEqual(self.byid[id]["expected"],"NONE",id)
        self.assertEqual(self.byid["BLE-NEG-PENGUIN-9"]["expected"],"FLOCK")

    def test_ble_payload_cases_require_exact_v150_patterns(self):
        s=(ROOT/"src/main.cpp").read_text(encoding="utf-8")
        self.assertIn("V150-BLE-GOOGLE-FMDN40",s)
        self.assertIn("V150-BLE-GOOGLE-FMDN41",s)
        self.assertIn("setServiceData(NimBLEUUID((uint16_t)0xFEAA)",s)
        self.assertIn("V150-BLE-HOMEPOD-SIRI",s)
        self.assertIn("0x08,0x07,0x01,0x02,0x03",s)
        self.assertIn("V150-BLE-AIRTAG-NEAR",s)
        self.assertIn("0x4C,0x00,0x12,0x02",s)
        self.assertIn("V150-BLE-AIRTAG-UNPAIRED",s)
        self.assertIn("V150-BLE-AIRPODS-NEG",s)
        self.assertEqual(self.byid["V150-BLE-AIRPODS-NEG"]["expected"],"NONE")
        self.assertIn("SquachV150OuiLab::apply(s->id,out)",s)
        self.assertIn("SquachV150OuiLab::find(s->id)",s)

    def test_compiled_mac_prefixes_are_source_pinned_and_synthetic(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        if not cc:self.skipTest("Host compiler missing")
        prog=r"""
#include "squach_v150_oui_lab.h"
#include "scenarios.h"
#include <cassert>
#include <cstring>
#include <set>
#include <string>
int main(){
  using namespace SquachV150OuiLab;
  static_assert(kCount==22,"v1.50 new OUI test count");
  std::set<std::string> seen;
  for(size_t i=0;i<kCount;i++){
    const auto& c=kCases[i];
    const Scenario* sc=findScenario(c.id);
    assert(sc&&sc->kind==Kind::WifiAp);
    assert(sc->code==6);
    assert(sc->text&&sc->text[0]);
    uint8_t mac[6]={0x02,0xA1,0xB2,0x31,0x32,0x33};
    assert(apply(c.id,mac));
    assert(memcmp(mac,c.prefix,3)==0);
    assert(mac[3]==0x31&&mac[4]==0x32&&mac[5]==0x33);
    assert(seen.insert(std::string(c.id)).second);
  }
  assert(!apply("V150-NOT-REGISTERED",nullptr));
  return 0;
}
"""
        with tempfile.TemporaryDirectory() as td:
            p=pathlib.Path(td)
            cpp=p/"fixture.cpp";exe=p/"fixture";cpp.write_text(prog)
            subprocess.run([cc,"-std=c++17","-Wall","-Wextra","-Werror",
                            "-I",str(ROOT/"include"),str(cpp),
                            str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],
                           capture_output=True,text=True,check=True)
            subprocess.run([str(exe)],capture_output=True,text=True,check=True)

if __name__=="__main__":
    unittest.main()
