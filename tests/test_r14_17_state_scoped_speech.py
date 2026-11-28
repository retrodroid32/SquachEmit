"""R14.17: speech bubbles may use only IDLE off air, and live file
category or TRANSMITTING on air. No unrelated categories or event quips."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]
CPP=r"""
#include "sqe_category_quips.h"
#include "scenario_library.h"
#include <cassert>
#include <cstddef>
int main(){
  using namespace SqeQuips;
  // Categories are the real scenario library's ten families, not carrier
  // types or whichever menu entry is currently highlighted.
  const Topic families[kDeviceCategoryCount]={
    Topic::General, Topic::Axon, Topic::General, Topic::Drones,
    Topic::Flock, Topic::General, Topic::Hacker, Topic::IBeacon,
    Topic::General, Topic::Raven, Topic::General, Topic::SerialModules,
    Topic::SmartGlasses, Topic::Trackers
  };
  size_t total=0;
  for(size_t i=0;i<kScenarioCount;++i){
    const Topic category=families[(size_t)deviceCategory(kScenarios[i])];
    ++total;
    for(unsigned roll=0;roll<100;roll++){
      // Even with a file selected, an inactive transmitter may NEVER
      // output a category, rare/Easter, suite, or transmitting message.
      assert(stateTopic(false,category,roll)==Topic::Idle);
      const Topic emitted=stateTopic(true,category,roll);
      assert(emitted==category||emitted==Topic::Transmitting);
      assert(emitted==(roll<60?category:Topic::Transmitting));
    }
  }
  assert(total==369);
  for(unsigned roll=0;roll<100;roll++){
    assert(stateTopic(false,Topic::Transmitting,roll)==Topic::Idle);
    assert(stateTopic(false,Topic::Ring,roll)==Topic::Idle);
    assert(stateTopic(true,Topic::Idle,roll)==Topic::Transmitting);
    assert(stateTopic(true,Topic::Rare,roll)==Topic::Transmitting);
    assert(stateTopic(true,Topic::Easter,roll)==Topic::Transmitting);
    assert(stateTopic(true,Topic::AirTag,roll)==Topic::Transmitting);
    assert(stateTopic(true,Topic::Suite,roll)==Topic::Transmitting);
  }
}
"""

class StateLockedSpeech(unittest.TestCase):
  @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),"native C++ compiler required")
  def test_whitelist_over_every_scenario_and_random_roll(self):
    cc=shutil.which("g++") or shutil.which("clang++")
    with tempfile.TemporaryDirectory() as tmp:
      source=Path(tmp)/"test.cpp"
      binary=Path(tmp)/"state_lock"
      source.write_text(CPP)
      subprocess.run([cc,"-std=c++17","-O2","-I",str(ROOT/"include"),
                      str(source),str(ROOT/"src/scenarios.cpp"),"-o",str(binary)],check=True)
      run=subprocess.run([str(binary)],capture_output=True,text=True)
      self.assertEqual(run.returncode,0,run.stderr)

  def test_speech_uses_live_case_not_selected_menu_file(self):
    code=(ROOT/"src/cyd32_ui.cpp").read_text()
    context=code.split("SqeBubble::Kind bubbleContext(",1)[1].split("long speechRandom(",1)[0]
    selector=code.split("void selectLandscapeSpeech(",1)[1].split("void advanceLandscapeBubble(",1)[0]
    self.assertIn("return active&&*active?SqeBubble::OnAir:SqeBubble::Idle;",context)
    self.assertIn("onAir?findScenario(active):nullptr",selector)
    self.assertIn("categoryTopic(transmitting)",selector)
    self.assertIn("SqeQuips::stateTopic(",selector)
    self.assertIn("SqeQuips::choose(",selector)
    self.assertNotIn("selected<kScenarioCount",selector)
    self.assertNotIn("selectedTopic(",selector)
    self.assertNotIn("weightedTopic(",selector)
    self.assertNotIn("kLongIdle",selector)
    self.assertNotIn("kRepeatRun",selector)
    self.assertNotIn("SqeQuips::Topic::Rare",selector)
    self.assertNotIn("SqeQuips::Topic::Easter",selector)
    self.assertIn("landscapeBubble.context=255;",code)
    self.assertIn("speechActiveCase",code)

if __name__=="__main__": unittest.main()
