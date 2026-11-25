"""R14.39 fork-only emission matrix, in addition to upstream v1.50 defaults.

Assertions are source-level and native C++ checks, NOT physical receiver evidence.
"""
import json
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
ADDED={
    "OUI-INSPFLIGHT-34B5F32": ("WifiAp", "NONE", "DRONE", "34B5F32"),
    "OUI-ZEROZERO-C863144": ("WifiAp", "NONE", "DRONE", "C863144"),
    "FORK-BLE-SKIM-BT04": ("BleName", "NONE", "SKIMMER", "BT04"),
    "FORK-BLE-SKIM-JDY09": ("BleName", "NONE", "SKIMMER", "JDY-09"),
    "FORK-BLE-SKIM-JDY10": ("BleName", "NONE", "SKIMMER", "JDY-10"),
    "FORK-BLE-SKIM-JDY17": ("BleName", "NONE", "SKIMMER", "JDY-17"),
    "FORK-BLE-SKIM-SPPC": ("BleName", "NONE", "SKIMMER", "SPP-C"),
}
FORK_MAM={
    "OUI-AUTEL-EC5BCDE","OUI-HUBSAN-98AAFC7","OUI-YUNEEC-E0B6F58",
    "OUI-QUANTUM-AC86D17","OUI-FIMI-6CDFFBE",
    "OUI-INSPFLIGHT-34B5F32","OUI-ZEROZERO-C863144"
}
FORK_NAMES={
    "BLE-SKIM-BT05","BLE-SKIM-JDY08","BLE-SKIM-JDY16","BLE-SKIM-MLTBT",
    "BLE-SKIM-LINVOR","FORK-BLE-SKIM-BT04","FORK-BLE-SKIM-JDY09",
    "FORK-BLE-SKIM-JDY10","FORK-BLE-SKIM-JDY17","FORK-BLE-SKIM-SPPC",
}

class ForkExtraEmissionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.cases=json.loads((ROOT/"tests/cases.json").read_text())
        cls.byid={x["id"]:x for x in cls.cases}
        cls.profile=json.loads((ROOT/"tests/squachwatch_fork_expectations.json").read_text())
    def test_complete_catalog_and_fork_profile(self):
        self.assertEqual(len(self.cases),369)
        self.assertEqual(len(self.byid),369)
        self.assertEqual(self.profile["signature_blob_sha"],
                         "1f63a08ce3eba4148d557312bb0d1abe8b18545c")
        self.assertEqual(set(self.profile["overrides"]),
                         FORK_MAM|FORK_NAMES|{"BLE-META-TECH","BLE-HACKER-MFG"})
        for id,want in self.profile["overrides"].items():
            self.assertIn(id,self.byid)
            self.assertEqual(self.byid[id]["expected"],"NONE",id)
            self.assertIn(want,{"DRONE","SKIMMER","META","HACKER"})
        for id,(kind,upstream,fork,value) in ADDED.items():
            case=self.byid[id]
            self.assertEqual(case["kind"],kind)
            self.assertEqual(case["expected"],upstream)
            self.assertEqual(self.profile["overrides"][id],fork)
            if kind=="BleName":self.assertEqual(case["text"],value)
            else:self.assertIn(value,case["description"])
    def test_added_cases_group_under_correct_device_category(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        if not cc:self.skipTest("No C++ compiler")
        program=r"""
#include "scenario_library.h"
#include <assert.h>
int main(){
  assert(deviceCategory(*findScenario("OUI-INSPFLIGHT-34B5F32"))==DeviceCategory::Drones);
  assert(deviceCategory(*findScenario("OUI-ZEROZERO-C863144"))==DeviceCategory::Drones);
  assert(deviceCategory(*findScenario("FORK-BLE-SKIM-BT04"))==DeviceCategory::SerialModules);
  assert(deviceCategory(*findScenario("FORK-BLE-SKIM-JDY09"))==DeviceCategory::SerialModules);
  assert(deviceCategory(*findScenario("FORK-BLE-SKIM-JDY10"))==DeviceCategory::SerialModules);
  assert(deviceCategory(*findScenario("FORK-BLE-SKIM-JDY17"))==DeviceCategory::SerialModules);
  assert(deviceCategory(*findScenario("FORK-BLE-SKIM-SPPC"))==DeviceCategory::SerialModules);
}
"""
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)
            (path/"test.cpp").write_text(program)
            subprocess.run([cc,"-std=c++11","-Wall","-Wextra","-Werror",
                "-I",str(ROOT/"include"),str(path/"test.cpp"),str(ROOT/"src/scenarios.cpp"),
                "-o",str(path/"test")],check=True)
            subprocess.run([str(path/"test")],check=True)

    def test_exact_28_bit_prefixes_are_emitted(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        if not cc:self.skipTest("No C++ compiler")
        src=r"""
#include "uav_vendor_oui_lab.h"
#include <assert.h>
#include <stdint.h>
int main(){
  uint8_t a[6]={0x02,0x11,0x22,0x33,0x44,0x55};
  assert(SquachUavVendorLab::apply("OUI-INSPFLIGHT-34B5F32",a));
  assert(a[0]==0x34&&a[1]==0xB5&&a[2]==0xF3);
  assert((a[3]&0xF0)==0x20 && (a[3]&0x0F)==0x03);
  assert(SquachUavVendorLab::apply("OUI-ZEROZERO-C863144",a));
  assert(a[0]==0xC8&&a[1]==0x63&&a[2]==0x14);
  assert((a[3]&0xF0)==0x40 && (a[3]&0x0F)==0x03);
  assert(!SquachUavVendorLab::apply("UNKNOWN-MAM",a));
}
"""
        with tempfile.TemporaryDirectory() as folder:
            path=pathlib.Path(folder)
            (path/"test.cpp").write_text(src)
            subprocess.run([cc,"-std=c++11","-Wall","-Wextra","-Werror",
                "-I",str(ROOT/"include"),str(path/"test.cpp"),"-o",str(path/"test")],
                check=True)
            subprocess.run([str(path/"test")],check=True)

if __name__=="__main__":unittest.main()
