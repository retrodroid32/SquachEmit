#include "Arduino.h"
#include "TFT_eSPI.h"
#include "cyd32_ui.h"
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
 tap(32,225);
 hold("categories",8);
 tap(245,225);
 hold("categories",8);
 tap(156,225);
 hold("categories",5);
 tap(25,225);
 hold("categories",5);
 tap(50,98);
 hold("categories",8);
 tap(45,60);
 hold("home",8);
 for(int sec=9;sec>=2;sec--){
   for(int i=0;i<3;i++){tick("BLE-DRONE",sec*1000,false,175);frameToPpm("emitting");}
 }
 hold("home",9);
 // Create an intentional ordered sequence for the hero: homepage, menus, selected profile,
 // then on-air. No radio is activated; this is a native TFT drawing capture only.
 printf("Captured %d native C++ UI frames\n",frame);
 return 0;
}
