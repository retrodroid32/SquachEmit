"""Fork-only SquachWatch OUI test fixtures; host tests do not prove radio reception."""
import json
import pathlib
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
EXPECTED={
    "3C71BF":"FLOCK","4827EA":"FLOCK","A4CF12":"FLOCK",
    "B030C8":"DRONE","EC715E":"DRONE","001AF9":"DRONE",
    "547D40":"DRONE","74B80F":"DRONE","98D300":"SKIMMER",
    "0CFA22":"HACKER",
}

class ForkOuiCoverage(unittest.TestCase):
    def test_exact_catalog_and_fork_source_pin(self):
        cases=json.loads((ROOT/"tests/cases.json").read_text())
        self.assertEqual(len(cases),369)
        byid={x["id"]:x for x in cases}
        self.assertEqual(len(byid),len(cases))
        actual=[x for x in cases if x["id"].startswith("FORK-OUI-")]
        self.assertEqual(len(actual),len(EXPECTED))
        self.assertEqual({c["id"].split("-")[-1]:c["expected"] for c in actual},EXPECTED)
        for c in actual:
            self.assertEqual((c["carrier"],c["kind"],c["code"]),("WIFI","WifiAp",6))
            self.assertLessEqual(len(c["text"]),32)
            self.assertTrue(c["text"].startswith("SQE-FORK-"))
        main=(ROOT/"src/main.cpp").read_text()
        self.assertIn('SquachForkOuiLab::apply(s->id,out)',main)
        self.assertIn('SquachForkOuiLab::find(s->id)!=nullptr',main)

    def test_compiled_prefix_preserves_synthetic_suffix(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        if not cc:self.skipTest("No C++ host compiler")
        cpp=r"""
#include "squach_fork_oui_lab.h"
#include "scenarios.h"
#include <cassert>
#include <cstring>
#include <set>
#include <string>
int main(){
    using namespace SquachForkOuiLab;
    static_assert(kCount==10,"10 fork-only OUI entries");
    std::set<std::string> ids;
    for(size_t i=0;i<kCount;++i){
        auto& c=kCases[i];
        const Scenario* sc=findScenario(c.id);
        assert(sc&&sc->kind==Kind::WifiAp&&sc->code==6);
        assert(sc->expected&&strcmp(sc->expected,"NONE"));
        uint8_t mac[6]={2,1,2,3,4,5};
        assert(apply(c.id,mac));
        assert(memcmp(mac,c.prefix,3)==0);
        assert(mac[3]==3&&mac[4]==4&&mac[5]==5);
        assert(ids.insert(c.id).second);
    }
    uint8_t mac[6]={2,1,2,3,4,5};
    assert(!apply("FORK-OUI-NOT-REGISTERED",mac));
    assert(mac[0]==2);
    return 0;
}
"""
        with tempfile.TemporaryDirectory() as temp:
            src=pathlib.Path(temp)/"test.cpp"
            exe=pathlib.Path(temp)/"test"
            src.write_text(cpp)
            subprocess.run([cc,"-std=c++17","-Wall","-Wextra","-Werror",
                "-I",str(ROOT/"include"),str(src),str(ROOT/"src/scenarios.cpp"),
                "-o",str(exe)],check=True,capture_output=True,text=True)
            subprocess.run([str(exe)],check=True,capture_output=True,text=True)

if __name__=="__main__":
    unittest.main()
