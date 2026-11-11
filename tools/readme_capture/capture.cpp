#include "Arduino.h"
#include "TFT_eSPI.h"
#include "cyd32_ui.h"
#include "scenarios.h"
#include <string>
#include <fstream>
#include <filesystem>
#include <cstdio>
namespace fs=std::filesystem;
int frame=0;
std::string out;
void frameToPpm(const std::string& group){
  auto* screen=MockDisplay::screen;
  char name[64];snprintf(name,sizeof(name),"%04d.ppm",frame++);
  fs::create_directories(out+"/"+group);
  std::ofstream file(out+"/"+group+"/"+name,std::ios::binary);
  file << "P6\n320 240\n255\n";
  for(uint16_t p:screen->pixel){
    uint8_t rgb[]={uint8_t(((p>>11)&31)*255/31),
                   uint8_t(((p>>5)&63)*255/63),
                   uint8_t((p&31)*255/31)};
    file.write((const char*)rgb,3);
  }
}
void tick(const char* active="",uint32_t remaining=0,bool suite=false,int advance=160){
  mock_time_ms+=advance;
  cyd32UiTick(active,remaining,suite);
}
void hold(const std::string& group,int frames,const char* active="",uint32_t remain=0,bool suite=false){
  for(int i=0;i<frames;i++){tick(active,remain,suite);frameToPpm(group);}
}
void tap(int x,int y,const char* active="",uint32_t remain=0){
  MockDisplay::touch=false;tick(active,remain,false,160);
  MockDisplay::x=x;MockDisplay::y=y;MockDisplay::touch=true;
  tick(active,remain,false,160);
  MockDisplay::touch=false;tick(active,remain,false,160);
}
int main(int argc,char** argv){
 if(argc!=2){fprintf(stderr,"Usage: capture DIR\n");return 2;}
 out=argv[1];
 cyd32UiBegin();
 // The *real* UI is drawn by cyd32UiTick(), including the source Squachy animator.
 hold("home",19);
 // Show the *current* firmware's persisted TX TIME editor rather than
 // compositing a mock screen into the README graphics.
 if(cyd32UiDurationMs()!=12000UL){
   fprintf(stderr,"Expected default 12-second TX duration in clean native capture\n");
   return 4;
 }
 tap(175,189); // Open TRANSMIT TIME from the genuine 172-case home UI.
 hold("duration",3);
 for(int i=0;i<4;i++){tap(262,109);hold("duration",2);} // 12 -> 16 sec
 if(cyd32UiDurationMs()!=16000UL){fprintf(stderr,"Duration increment failed\n");return 5;}
 for(int i=0;i<2;i++){tap(55,109);hold("duration",2);} // 16 -> 14 sec
 if(cyd32UiDurationMs()!=14000UL){fprintf(stderr,"Duration decrement failed\n");return 6;}
 tap(160,225);hold("duration",2); // RESET 12
 if(cyd32UiDurationMs()!=12000UL){fprintf(stderr,"Duration reset failed\n");return 7;}
 tap(262,109);hold("duration",2); // Set 13 sec for the subsequent RUN preview.
 if(cyd32UiDurationMs()!=13000UL){fprintf(stderr,"Duration final setting failed\n");return 8;}
 tap(263,225); // DONE, return to the home display without erasing saved time.
 hold("home",5);
 tap(32,225);
 hold("categories",6);    // AXON, DRONES, FLOCK, GENERAL
 tap(245,225);
 hold("categories",6);    // HACKER, IBEACON, RAVEN, SERIAL
 tap(245,225);
 hold("categories",6);    // SMART GLASSES, TRACKERS
 tap(156,225);
 hold("categories",4);
 tap(156,225);
 hold("categories",4);
 // Select DRONES / REMOTE ID from the first alphabetized category page.
 tap(50,98);
 hold("categories",6);
 // Navigate through alphabetically sorted Drones tests.
 tap(250,225);
 hold("categories",5);
 tap(250,225);
 hold("categories",5);
 tap(150,225);
 hold("categories",4);
 tap(150,225);
 hold("categories",4);
 // Select the alphabetically first test inside DRONES.
 tap(45,60);
 if(!cyd32UiSelection() || std::string(cyd32UiSelection()->id)!="BLE-DRONE"){
   fprintf(stderr,"Demo did not select first Drones case\\n");
   return 3;
 }
 hold("home",8);
 for(int sec=13;sec>=2;sec--){
   for(int i=0;i<3;i++){tick("BLE-DRONE",sec*1000,false,175);frameToPpm("emitting");}
 }
 hold("home",9);
 // Create an intentional ordered sequence for the hero: homepage, menus, selected profile,
 // then on-air. No radio is activated; this is a native TFT drawing capture only.
 printf("Captured %d native C++ UI frames\n",frame);
 return 0;
}
