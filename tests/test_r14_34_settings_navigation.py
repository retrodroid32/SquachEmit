"""R14.34 navigation: Settings submenu BACK must restore Settings page one.

Runs the real CYD3.2 renderer with the native TFT/XPT2046 mock. It checks
touch sequences and the serial state model, not only source-code snippets.
"""
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
MOCK=ROOT/"tools/readme_capture/include"


class SettingsNavigationTests(unittest.TestCase):
    def test_settings_back_from_identity_help_recovery_battery(self):
        cc=shutil.which("g++") or shutil.which("clang++")
        if not cc:self.skipTest("Host C++ compiler not installed")
        code=r"""
#include "Arduino.h"
#include "TFT_eSPI.h"
#include "cyd32_ui.h"
#include <assert.h>
#include <stdio.h>
static void idle(){
  mock_time_ms+=80;
  assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
}
static void touch(unsigned x,unsigned y){
  mock_time_ms+=300;
  MockDisplay::touch=true;MockDisplay::x=x;MockDisplay::y=y;
  assert(cyd32UiTick(nullptr,0,false)==Cyd32UiAction::None);
  mock_time_ms+=75;MockDisplay::touch=false;idle();
}
static void mark(const char* name){
  fprintf(stderr,"CHECKPOINT %s\n",name);
  cyd32UiStreamState(nullptr,0,false,true);
}
static void openSettings(){
  touch(20,8);       // open Main Menu from header
  touch(268,226);   // main menu page 2, items 5-8
  touch(150,92);    // SETTINGS (main menu item 6)
}
int main(){
  cyd32UiBegin();idle();
  openSettings();mark("settings0");
  touch(158,226);mark("settings1"); // MORE > -> DISPLAY page
  touch(155,132);mark("identity");  // identity on page 2
  touch(50,226);mark("back_identity");

  touch(158,226);mark("settings1_again");
  touch(268,226);mark("help"); // HELP footer from SETTINGS / DISPLAY
  touch(268,226);mark("recovery"); // Help -> Recovery
  touch(50,226);mark("back_recovery"); // -> first Settings page

  touch(158,226);mark("settings1_third");
  touch(268,226);mark("help_again");
  touch(50,226);mark("back_help");

  touch(270,226);mark("battery");
  touch(50,226);mark("back_battery");

  // Cycling Settings pages still works both ways without opening a submenu.
  touch(150,226);mark("settings2_cycle");
  touch(150,226);mark("settings1_cycle");
  touch(150,226);mark("settings_second_for_back");
  touch(50,226);mark("back_second_to_first");

  // Direct Help from global Main Menu must go back to Main Menu, NOT Settings.
  touch(50,226);mark("main_menu_from_settings");
  touch(268,226); // last page in 12-entry Main Menu
  touch(150,87);mark("direct_help"); // item 10
  touch(50,226);mark("back_direct_help");
  return 0;
}
"""
        with tempfile.TemporaryDirectory() as td:
            folder=pathlib.Path(td)
            src=folder/"nav.cpp";bin=folder/"nav"
            src.write_text(code,encoding="utf-8")
            subprocess.run([cc,"-std=c++17","-O2","-DSQE_CYD32=1",
                            "-I",str(MOCK),"-I",str(ROOT/"include"),
                            str(src),str(ROOT/"src"/"cyd32_ui.cpp"),
                            str(ROOT/"src"/"cyd32_squachy_port.cpp"),
                            str(ROOT/"src"/"scenarios.cpp"),"-o",str(bin)],
                           capture_output=True,text=True,check=True)
            result=subprocess.run([str(bin)],capture_output=True,text=True,check=True)
        chunks=re.split(r"^CHECKPOINT ([^\n]+)\n",result.stderr,flags=re.M)
        states={}
        for i in range(1,len(chunks),2):
            name=chunks[i]
            payload=chunks[i+1]
            state=re.search(r"^UI STATE 1 (\w+) ",payload,re.M)
            self.assertIsNotNone(state,(name,payload[:400]))
            device=re.search(r"^UI DEVICE 1 ([^\n]+)",payload,re.M)
            more=int(device.group(1).split()[-1]) if device else None
            states[name]=(state.group(1),more)
        wanted={
            "settings0":("SETTINGS",0),
            "settings1":("SETTINGS",1),
            "identity":("IDENTITY",None),
            "back_identity":("SETTINGS",0),
            "settings1_again":("SETTINGS",1),
            "help":("HELP",0),
            "recovery":("RECOVERY",0),
            "back_recovery":("SETTINGS",0),
            "settings1_third":("SETTINGS",1),
            "help_again":("HELP",0),
            "back_help":("SETTINGS",0),
            "battery":("BATCAL",None),
            "back_battery":("SETTINGS",0),
            "settings2_cycle":("SETTINGS",1),
            "settings1_cycle":("SETTINGS",0),
            "settings_second_for_back":("SETTINGS",1),
            "back_second_to_first":("SETTINGS",0),
            "main_menu_from_settings":("MENU",None),
            "direct_help":("HELP",0),
            "back_direct_help":("MENU",None),
        }
        self.assertEqual(states,wanted)

    def test_all_settings_origin_back_routes_share_single_reset(self):
        src=(ROOT/"src"/"cyd32_ui.cpp").read_text(encoding="utf-8")
        self.assertIn("void backFromSettingsSubmenu()",src)
        self.assertIn("if(settingsMore){settingsMore=false;}",src)
        self.assertIn("else {screen=Screen::MainMenu;settingsSubmenuFlow=false;}",src)
        self.assertIn("settingsMore=false;",src)
        self.assertIn("settingsSubmenuFlow=false;",src)
        self.assertEqual(src.count("if(x<108)backFromSettingsSubmenu();"),3)
        self.assertIn("if(x<108){screen=Screen::Settings;settingsMore=false;redraw=true;}",src)
        self.assertIn("return touchIdentity(50,218);",src)
        self.assertIn("return touchHelp(50,218);",src)
        self.assertIn("return touchRecovery(50,218);",src)


if __name__=="__main__":
    unittest.main()
