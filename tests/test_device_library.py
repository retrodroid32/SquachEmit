"""Native device-type library regression: every case belongs exactly once."""
import json
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

class DeviceLibraryTests(unittest.TestCase):
    def test_exclusive_categories_and_alphabetical_order(self):
        compiler = shutil.which("g++") or shutil.which("clang++")
        if compiler is None:
            self.skipTest("Host C++ compiler unavailable")
        program = r'''
#include "scenario_library.h"
#include <stdio.h>
int main() {
  for (size_t cat = 0; cat < kDeviceCategoryCount; ++cat) {
    DeviceCategory c = static_cast<DeviceCategory>(cat);
    size_t count = deviceCategoryCaseCount(c);
    printf("CATEGORY\t%zu\t%s\t%zu\n", cat, deviceCategoryName(c), count);
    for (size_t rank = 0; rank < count; ++rank) {
      size_t index = kScenarioCount;
      if (!deviceCategoryCaseAtRank(c, rank, &index)) return 2;
      printf("CASE\t%zu\t%s\n", cat, kScenarios[index].id);
    }
    size_t unused = kScenarioCount;
    if (deviceCategoryCaseAtRank(c, count, &unused)) return 3;
  }
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            source=Path(tmp)/"test.cpp"
            binary=Path(tmp)/"test"
            source.write_text(program)
            subprocess.run([compiler,"-std=c++17","-I",str(ROOT/"include"),
                            str(ROOT/"src"/"scenarios.cpp"),str(source),"-o",str(binary)],
                           capture_output=True,text=True,check=True)
            result=subprocess.run([str(binary)],capture_output=True,text=True,check=True)
        catalog=json.loads((ROOT/"tests"/"cases.json").read_text())
        known={x["id"]:x for x in catalog}
        category_names=[]
        grouped={}
        for line in result.stdout.splitlines():
            parts=line.split("\t")
            if parts[0]=="CATEGORY":
                _,idx,name,count=parts
                category_names.append(name)
                grouped[int(idx)]=[]
            else:
                _,idx,case_id=parts
                grouped[int(idx)].append(case_id)
        self.assertEqual(category_names, sorted(category_names))
        self.assertEqual(len(grouped), 10)
        all_ids=[case for values in grouped.values() for case in values]
        self.assertEqual(len(all_ids),145)
        self.assertEqual(len(set(all_ids)),145)
        self.assertEqual(set(all_ids),set(known))
        for cases in grouped.values():
            self.assertEqual(cases,sorted(cases))
            self.assertGreater(len(cases),0)
        by_name=dict(zip(category_names,grouped.values()))
        drones=by_name["DRONES / REMOTE ID"]
        self.assertIn("BLE-DRONE",drones)
        self.assertIn("RID-WIFI-CH1",drones)
        self.assertIn("RID-DJI-MINI",drones)
        self.assertIn("OUI-DJI-04A85A",drones)
        self.assertEqual(len([s for s in drones if s.startswith("OUI-DJI-")]),10)
        self.assertTrue(all(s.startswith(("RID-","OUI-DJI-")) or s in ("BLE-DRONE","BLE-NEG-DRONE-UUID") for s in drones))
        self.assertNotIn("BLE-DRONE",by_name["GENERAL / CONTROLS"])
        self.assertIn("BLE-META-SNAP",by_name["SMART GLASSES"])
        self.assertIn("BLE-NEG-JDY16-EXTRA",by_name["SERIAL MODULES"])
        self.assertIn("WIFI-NEG-PAGER-OFFICE",by_name["HACKER DEVICES"])

if __name__=="__main__":
    unittest.main()
