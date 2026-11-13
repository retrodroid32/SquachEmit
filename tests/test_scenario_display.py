"""Every SquachEmit native UI fixture has a recognized device-first label."""
import pathlib
import subprocess
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]


class ScenarioDisplayTests(unittest.TestCase):
    def test_all_device_names_and_stable_case_ids(self):
        cpp = r'''
#include "scenario_display.h"
#include <cassert>
#include <cstdio>
#include <cstring>
int main() {
    assert(kScenarioCount == 172);
    for(size_t i=0;i<kScenarioCount;++i){
        char label[80]={0};
        scenarioDeviceName(kScenarios[i],label,sizeof label);
        if(!label[0] || std::strstr(label,"UNMAPPED") ||
           std::strstr(label,"UNKNOWN DEVICE")) {
            std::fprintf(stderr,"Missing native device name for %s\n",kScenarios[i].id);
            return 1;
        }
        if(!std::strcmp(kScenarios[i].id,label)){
            std::fprintf(stderr,"Raw ID used as a device name: %s\n",label);
            return 2;
        }
    }
    char name[80]={0};
    scenarioDeviceName(*findScenario("BLE-HACKER-UUID"),name,sizeof name);
    assert(!std::strcmp(name,"Flipper Zero"));
    scenarioDeviceName(*findScenario("BLE-HACKER-NAME"),name,sizeof name);
    assert(!std::strcmp(name,"Flipper Zero"));
    scenarioDeviceName(*findScenario("OUI-AUTEL-18D7936"),name,sizeof name);
    assert(std::strstr(name,"Autel"));
    scenarioDeviceName(*findScenario("RID-DJI-MINI3PRO"),name,sizeof name);
    assert(std::strstr(name,"DJI Mini 3 Pro"));
    scenarioDeviceName(*findScenario("BLE-NEG-FLIP-MFG"),name,sizeof name);
    assert(std::strstr(name,"Flipper Zero"));
    return 0;
}
'''
        with tempfile.TemporaryDirectory() as temp:
            src = pathlib.Path(temp) / "verify.cpp"
            out = pathlib.Path(temp) / "verify"
            src.write_text(cpp)
            subprocess.run([
                "g++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                "-I", str(ROOT / "include"),
                str(src), str(ROOT / "src/scenarios.cpp"), "-o", str(out)
            ], check=True, capture_output=True, text=True)
            subprocess.run([str(out)], check=True, text=True)

    def test_native_screen_name_is_primary(self):
        cyd = (ROOT / "src/cyd32_ui.cpp").read_text()
        c5 = (ROOT / "src/c5_ui.cpp").read_text()
        self.assertIn('scenarioDeviceName(*s,name,sizeof(name))', cyd)
        self.assertIn('scenarioDeviceName(*chosen,friendly,sizeof(friendly))', cyd)
        self.assertIn('deviceCaption(kScenarios[index],name,sizeof(name))', c5)
        self.assertIn('kScenarios[index].id', c5)


if __name__ == "__main__":
    unittest.main()
