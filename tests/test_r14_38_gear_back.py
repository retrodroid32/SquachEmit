"""R14.38 real touch navigation regression for gear menu and nested shortcuts."""
import pathlib
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT=pathlib.Path(__file__).resolve().parents[1]
MOCK=ROOT/"tools/readme_capture/include"


class SettingsNavigationTests(unittest.TestCase):
    def test_gear_menu_back_in_both_orientations(self):
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
  touch(20,8);mark("gear_page_one");
  touch(150,60);mark("gear_library");
  touch(50,226);mark("back_gear_library");
  touch(150,99);mark("gear_quick");
  touch(50,226);mark("back_gear_quick");
  touch(150,138);mark("gear_favorites");
  touch(50,226);mark("back_gear_favorites");
  touch(150,177);mark("gear_history");
  touch(50,226);mark("back_gear_history");
  touch(150,60);touch(150,60);mark("nested_library");
  touch(50,226);mark("back_nested_library");
  touch(250,34);mark("nested_quick");
  touch(50,226);mark("back_nested_quick");
  touch(50,226);mark("back_nested_categories");
  touch(50,226);touch(50,226);mark("home_library");
  touch(50,226);mark("back_home_library");
  touch(290,8);touch(20,8);mark("portrait_gear");
  touch(150,85);mark("portrait_library");
  touch(40,300);mark("portrait_back_library");
  touch(150,134);mark("portrait_quick");
  touch(40,300);mark("portrait_back_quick");
  touch(150,183);mark("portrait_favorites");
  touch(40,300);mark("portrait_back_favorites");
  touch(150,232);mark("portrait_history");
  touch(40,300);mark("portrait_back_history");
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
            states[name]=state.group(1)
        wanted={
          "gear_page_one": "MENU",
          "gear_library": "CATEGORIES",
          "back_gear_library": "MENU",
          "gear_quick": "QUICKFILTER",
          "back_gear_quick": "MENU",
          "gear_favorites": "FAVORITES",
          "back_gear_favorites": "MENU",
          "gear_history": "HISTORY",
          "back_gear_history": "MENU",
          "nested_library": "LIBRARY",
          "back_nested_library": "CATEGORIES",
          "nested_quick": "QUICKFILTER",
          "back_nested_quick": "CATEGORIES",
          "back_nested_categories": "MENU",
          "home_library": "CATEGORIES",
          "back_home_library": "HOME",
          "portrait_gear": "MENU",
          "portrait_library": "CATEGORIES",
          "portrait_back_library": "MENU",
          "portrait_quick": "QUICKFILTER",
          "portrait_back_quick": "MENU",
          "portrait_favorites": "FAVORITES",
          "portrait_back_favorites": "MENU",
          "portrait_history": "HISTORY",
          "portrait_back_history": "MENU"
        }
        self.assertEqual(states,wanted)


if __name__=="__main__":
    unittest.main()
