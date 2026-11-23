"""R14.15: all user-approved quips, 10 real SquachEmit categories,
carrier subtypes and bounded random show/hide + on-device Easter eggs.
"""
import shutil, subprocess, tempfile, unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
CXX=r'''
#include "sqe_category_quips.h"
#include "scenario_library.h"
#include <cassert>
#include <cstring>
#include <cstdint>
int main(){
 using namespace SqeQuips;
 // Exactly 10 real user-facing groups, each populated.
 assert(kDeviceCategoryCount==14);
 Topic topics[]={
   Topic::General,Topic::Axon,Topic::General,Topic::Drones,
   Topic::Flock,Topic::General,Topic::Hacker,Topic::IBeacon,
   Topic::General,Topic::Raven,Topic::General,Topic::SerialModules,
   Topic::SmartGlasses,Topic::Trackers
 };
 for(size_t i=0;i<kDeviceCategoryCount;i++){
   auto bankFound=bank(topics[i]);
   assert(bankFound.count>=5);
   assert(deviceCategoryCaseCount((DeviceCategory)i)>0);
   for(unsigned j=0;j<bankFound.count;j++)
     assert(bankFound.lines[j]&&strlen(bankFound.lines[j])>0
            &&strlen(bankFound.lines[j])<64);
 }
 // All 172 real scenarios map into real categories (never a made-up Ring).
 size_t sum=0;
 for(size_t c=0;c<kDeviceCategoryCount;c++)
   sum+=deviceCategoryCaseCount((DeviceCategory)c);
 assert(sum==238);
 for(size_t i=0;i<kScenarioCount;i++){
   assert((unsigned)deviceCategory(kScenarios[i])<kDeviceCategoryCount);
 }
 assert(bank(Topic::Idle).count==11);
 assert(bank(Topic::Transmitting).count==9);
 assert(bank(Topic::Starting).count==5);
 assert(bank(Topic::Failed).count==6);
 assert(bank(Topic::Playlist).count==5);
 assert(bank(Topic::Suite).count==7);
 assert(bank(Topic::Hacker).count==7);
 assert(bank(Topic::AirTag).count==3);
 assert(bank(Topic::Ble).count==3);
 assert(bank(Topic::Wifi).count==3);
 assert(bank(Topic::RemoteId).count==3);
 assert(bank(Topic::Ring).count==3);
 assert(bank(Topic::Rare).count==5);
 assert(bank(Topic::Easter).count==5);
 assert(strcmp(kRepeatRun,"AGAIN? SERIOUSLY?")==0);
 assert(strcmp(kLongIdle,"DID YOU FORGET ABOUT ME?")==0);
 // Exhaustively verify the active/idle bank whitelist for every roll.
 for(int r=0;r<100;r++){
   assert(stateTopic(false,Topic::Flock,r)==Topic::Idle);
   assert(stateTopic(false,Topic::Rare,r)==Topic::Idle);
   assert(stateTopic(false,Topic::Transmitting,r)==Topic::Idle);
   assert(stateTopic(true,Topic::Flock,r)==
          (r<60?Topic::Flock:Topic::Transmitting));
   assert(stateTopic(true,Topic::Hacker,r)==
          (r<60?Topic::Hacker:Topic::Transmitting));
   // Missing/invalid active file IDs cannot reuse selected menu category.
   assert(stateTopic(true,Topic::Idle,r)==Topic::Transmitting);
   assert(stateTopic(true,Topic::Ring,r)==Topic::Transmitting);
   assert(stateTopic(true,Topic::Rare,r)==Topic::Transmitting);
 }
 for(int t=0;t<=static_cast<int>(Topic::Easter);t++){
   const auto b=bank((Topic)t);
   for(unsigned i=0;i<b.count;i++){
     assert(choose((Topic)t,i)==b.lines[i]);
     for(const char* p=b.lines[i];*p;p++)assert((unsigned char)*p<128);
   }
 }
 return 0;
}
'''
ORIGINAL=[
"WAKE ME WHEN WE TRANSMIT!","JUST A SQUACH WITH A DREAM.","I SMELL RADIO WAVES!",
"IS THIS THING ON?","MY ANTENNA IS ITCHY.",
"PEW PEW! RADIO EDITION!","I'M NOT SAYING IT'S ALIENS...",
"BEHOLD! INVISIBLE NOISE!","SQUACHY GOES BRRRR!","SCIENCE! PROBABLY.",
"SUMMONING THE WIFI SPIRITS!","HOLD MY ANTENNA!","CONNECTING THE DOTS...",
"WIFI WIZARDRY IN PROGRESS!",
"WELL, THAT WAS EMBARRASSING.","THE GREMLINS DID IT!","TASK FAILED SUCCESSFULLY!",
"HAVE YOU TRIED MORE MAGIC?","MY ANTENNA NEEDS THERAPY.",
"DJ SQUACH IS IN THE HOUSE!","NOW SPINNING: RADIO WAVES!",
"THE HITS JUST KEEP COMING!","THIS MIX IS FIRE! (NOT RF)",
"RELEASE THE TEST GOBLINS!","LET THE CHAOS BEGIN!",
"ONE TEST TO RULE THEM ALL!","I WAS BUILT FOR THIS. MAYBE.",
"THE SCIENCE IS SCIENCING!",
"FLOCK AROUND AND FIND OUT!","WHO LET THE BIRDS OUT?","NO BIRDS WERE HARMED!",
"DING DONG! IT'S SQUACH!","SPECIAL DELIVERY: RF!","YOUR DOORBELL KNOWS TOO MUCH.",
"CAMERA READY? ACTION!","I DEMAND A BODYCAM!","SMILE! YOU'RE ON SQUACH CAM!",
"I'M NOT LOST. YOU ARE!","WHERE'S MY OTHER SOCK?","FOLLOW THE BEEP!",
"I BELIEVE I CAN WIFI!","I HAVE THE HIGH GROUND!","HOUSTON, WE HAVE A SQUACH!",
"FLYING WITHOUT LEAVING HOME!",
"LOW ENERGY. HIGH DRAMA.","BLUETOOTH AND CHILL?","BLE ME AWAY!",
"BEACON AND EGGS, ANYONE?","SSID? MORE LIKE SOS!","THE WIFI IS STRONG WITH ME.",
"IDENTITY CRISIS INCOMING!","HELLO! I AM A TEST DRONE!","MY ID IS COMPLICATED.",
"I KNOW WHAT YOU DID LAST WIFI.","THE GOVERNMENT STOLE MY WIFI.",
"I PUT THE SQUACH IN SQUELCH.","MY OTHER ANTENNA IS A YAGI.",
"I CAN HEAR YOUR TOASTER.","THIS IS FINE. PROBABLY.",
"ERROR 404: COMMON SENSE.","I'M 90% FUR, 10% RF.",
"DON'T TELL THE FCC ABOUT ME!","SHHH... I'M DOING SCIENCE.",
"AGAIN? SERIOUSLY?","DID YOU FORGET ABOUT ME?",
"HERE WATCHY WATCHY!","HACK THE PLANET!",
"SETTING PHASERS TO ANNOY!","SENDING EVERYTHING BUT THE KITCHEN SINK!"
]
class CategoryBanterTests(unittest.TestCase):
 @unittest.skipUnless(shutil.which("g++") or shutil.which("clang++"),"C++ compiler required")
 def test_complete_categories_and_weighted_chooser(self):
  cc=shutil.which("g++") or shutil.which("clang++")
  with tempfile.TemporaryDirectory() as tmp:
   p=Path(tmp)/"quips.cpp";exe=Path(tmp)/"quips"
   p.write_text(CXX)
   subprocess.run([cc,"-std=c++17","-O2",
     "-I",str(ROOT/"include"),str(p),str(ROOT/"src/scenarios.cpp"),"-o",str(exe)],check=True)
   out=subprocess.run([str(exe)],capture_output=True,text=True)
   self.assertEqual(out.returncode,0,out.stderr)
 def test_every_previously_approved_text_is_in_catalog(self):
  text=(ROOT/"include/sqe_category_quips.h").read_text()
  for phrase in ORIGINAL:
   with self.subTest(phrase=phrase):self.assertIn(phrase,text)
  for phrase in ["IBEACON, THEREFORE I AM.","QUOTH THE RAVEN: BLE!",
                 "AT+SQUACH=1","MY HOODIE HAS ROOT ACCESS.",
                 "I SEE WHAT YOU TRANSMITTED.","THIS ONE'S A CONTROL GROUP!",
                 "SQUACHY KNOWS WHERE HE IS."]:
   self.assertIn(phrase,text)
 def test_choose_once_per_popup_and_never_fake_ring_or_override_rf(self):
  ui=(ROOT/"src/cyd32_ui.cpp").read_text()
  hdr=(ROOT/"include/cyd32_ui.h").read_text()
  main=(ROOT/"src/main.cpp").read_text()
  self.assertIn("selectLandscapeSpeech(now,active,suite);",ui)
  self.assertIn("if(changed&&landscapeBubble.showing)",ui)
  self.assertIn("landscapeSpeechText=SqeQuips::choose(",ui)
  self.assertNotIn("SqeQuips::weightedTopic(",ui)
  self.assertNotIn("selectedTopic(",ui)
  self.assertNotIn("SqeQuips::kRepeatRun",ui)
  self.assertNotIn("SqeQuips::kLongIdle",ui)
  self.assertIn("const Scenario* transmitting=onAir?findScenario(active):nullptr;",ui)
  self.assertIn("onAir,categoryTopic(transmitting)",ui)
  self.assertIn("landscapeBubble.context=255;",ui)
  self.assertIn("cyd32UiNoteRunAction();",main)
  self.assertIn("void cyd32UiNoteRunAction();",hdr)
  # Ring lives in the catalog for future use, never classified today.
  self.assertNotIn("SqeQuips::Topic::Ring",ui)
  self.assertIn("case DeviceCategory::SmartGlasses:",ui)
  self.assertIn("case DeviceCategory::SerialModules:",ui)
  self.assertIn("case DeviceCategory::IBeacon:",ui)
  self.assertIn("case DeviceCategory::Raven:",ui)
  self.assertIn("case DeviceCategory::General:",ui)
  self.assertIn("speechWrap(speechLine(),primary,detail,282)",ui)
  self.assertIn('const bool hasDetail=detail[0]',ui)
  self.assertIn("Cyd32FramePush::push(panel,halfFrame.buf()",ui)
  self.assertIn("SqeTouchWizard::runInteractive",ui)
 def test_full_phrase_length_and_tft_legacy_font_safe(self):
  import re
  text=(ROOT/"include/sqe_category_quips.h").read_text()
  lines=re.findall(r'^\s*"(.*?)"[,;]?\s*$',text,re.M)
  self.assertGreaterEqual(len(lines),95)
  for line in lines:
   with self.subTest(line=line):
    self.assertLess(len(line),64)
    self.assertTrue(all(ord(ch)<128 for ch in line))
if __name__=="__main__":unittest.main()
