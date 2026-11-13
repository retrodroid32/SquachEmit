#pragma once
#include "scenarios.h"
#include <cstdio>
#include <cstring>

// Human-facing synthetic fixture/device name. Stable case IDs remain unchanged
// for serial RUN, TRUTH, regression fixtures and CSV.
// Never interpret a descriptive name as proof of actual hardware identity.
inline bool sqeStarts(const char* s,const char* prefix){
  return s && prefix && std::strncmp(s,prefix,std::strlen(prefix))==0;
}
inline void sqeName(const char* s,char* out,size_t cap){
  if(cap)std::snprintf(out,cap,"%s",s?s:"");
}
inline void sqeDescriptionHead(const char* s,char* out,size_t cap){
  if(!cap)return;
  const char* end=s?std::strstr(s," | "):nullptr;
  const size_t n=end?static_cast<size_t>(end-s):(s?std::strlen(s):0);
  std::snprintf(out,cap,"%.*s",static_cast<int>(n),s?s:"");
}
inline void scenarioDeviceName(const Scenario& s,char* out,size_t cap){
  if(!out||!cap)return;
  const char* id=s.id?s.id:"";
  const char* name=nullptr;
  if(sqeStarts(id,"OUI-")||sqeStarts(id,"RID-DJI-")||
     sqeStarts(id,"RID-WIFI-DJI-")){
    sqeDescriptionHead(s.description,out,cap);return;
  }
  if(sqeStarts(id,"RID-AUTEL-")){
    if(std::strstr(id,"EVOMAX"))name="Autel EVO Max";
    else if(std::strstr(id,"EVOLITE"))name="Autel EVO Lite";
    else name="Autel EVO II";
  }else if(sqeStarts(id,"RID-PARROT-")||std::strcmp(id,"RID-WIFI-PARROT")==0){
    name=std::strstr(id,"ANAFIUSA")?"Parrot ANAFI USA":"Parrot ANAFI";
  }else if(sqeStarts(id,"RID-")||std::strcmp(id,"BLE-DRONE")==0){
    name="OpenDroneID";
  }else if(sqeStarts(id,"BLE-HACKER-")||std::strcmp(id,"BLE-NEG-FLIP-MFG")==0){
    name="Flipper Zero";
  }else if(sqeStarts(id,"WIFI-HACKER-PAGER")||sqeStarts(id,"WIFI-NEG-PAGER")){
    name="Wi-Fi Pineapple Pager";
  }else if(std::strcmp(id,"WIFI-HACKER-PINE")==0){
    name="Wi-Fi Pineapple";
  }else if(std::strcmp(id,"WIFI-HACKER-PWN")==0){
    name="ESP Deauther";
  }else if(sqeStarts(id,"BLE-FLOCK-")||sqeStarts(id,"WIFI-FLOCK")||
           std::strcmp(id,"BLE-NEG-BARE9")==0){
    name="Flock Safety";
  }else if(std::strcmp(id,"BLE-NEG-PENGUIN-9")==0){
    name="Flock Penguin";
  }else if(std::strcmp(id,"WIFI-NEG-FLOCKHART")==0){
    name="Flock Safety control";
  }else if(sqeStarts(id,"BLE-META-")){
    name=std::strstr(id,"LUXOTTICA")?"Ray-Ban / Luxottica":
         std::strstr(id,"SNAP")?"Snap Spectacles":
         std::strstr(id,"UUID")?"Ray-Ban Meta":"Meta";
  }else if(std::strcmp(id,"BLE-NEG-META-ADJ")==0){
    name="Ray-Ban Meta control";
  }else if(std::strcmp(id,"BLE-NEG-SNAP-ADJ")==0){
    name="Snap Spectacles control";
  }else if(sqeStarts(id,"BLE-RAVEN")||sqeStarts(id,"BLE-NEG-RAVEN")){
    name="Raven";
  }else if(sqeStarts(id,"BLE-AIRTAG")){
    name="Apple AirTag";
  }else if(sqeStarts(id,"BLE-SAMSUNG")){
    name="Samsung SmartTag";
  }else if(sqeStarts(id,"BLE-TILE")){
    name="Tile Tracker";
  }else if(std::strcmp(id,"BLE-GOOGLE")==0){
    name="Eddystone / Google control";
  }else if(sqeStarts(id,"BLE-IBEACON")||sqeStarts(id,"BLE-NEG-IBEACON")){
    name="Apple iBeacon";
  }else if(std::strcmp(id,"BLE-NEG-APPLE")==0){
    name="Apple Find My control";
  }else if(sqeStarts(id,"BLE-AXON")||sqeStarts(id,"WIFI-AXON")||
           sqeStarts(id,"WIFI-NEG-AXON")){
    name="Axon Body Camera";
  }else if(sqeStarts(id,"BLE-SKIM")||std::strcmp(id,"BLE-NEG-JDY16-EXTRA")==0){
    name=s.text?s.text:"Bluetooth Serial Module";
  }else if(sqeStarts(id,"BLE-NEG-UUID")||sqeStarts(id,"BLE-NEG-NAME")){
    name="Generic BLE device";
  }else if(sqeStarts(id,"WIFI-NEG")){
    name="Generic Wi-Fi AP";
  }
  // Strictly fail visible rather than disguising future uncatalogued cases
  // as a concrete brand; CI enumerates every production case.
  sqeName(name?name:"UNMAPPED DEVICE",out,cap);
}
