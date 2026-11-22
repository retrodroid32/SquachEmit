#pragma once
#include <cstring>
#include <cstddef>
#include <cstdint>
inline uint8_t mock_saved_tx_time=12;
inline int32_t mock_battery_offset_mv=0;
inline bool mock_battery_display_volts=false;
// Default remains calibrated for existing host fixtures. New RF-safe
// portrait tests can set false to emulate a freshly erased portrait key.
inline bool mock_portrait_touch_saved=true;
inline bool mock_portrait_fit_saved=true;
inline bool mock_background_saved=false;
inline uint8_t mock_background_value=9;
class Preferences{
public:
 int32_t getInt(const char* name,int32_t fallback) {
  return std::strcmp(name,"batoff_mv")==0?mock_battery_offset_mv:fallback;
 }
 size_t putInt(const char* name,int32_t value) {
  if(std::strcmp(name,"batoff_mv")==0)mock_battery_offset_mv=value;
  return sizeof(int32_t);
 }
 uint8_t getUChar(const char* name,uint8_t fallback){
  if(std::strcmp(name,"txtime")==0)return mock_saved_tx_time?mock_saved_tx_time:fallback;
  if(std::strcmp(name,"v")==0)return mock_portrait_fit_saved?1:fallback;
  if(std::strcmp(name,"bg_sqwatch")==0&&mock_background_saved)return mock_background_value;
  return fallback;
 }
 size_t putUChar(const char* name,uint8_t value){
  if(std::strcmp(name,"txtime")==0)mock_saved_tx_time=value;
  if(std::strcmp(name,"bg_sqwatch")==0){mock_background_saved=true;mock_background_value=value;}
  return 1;
 }
 bool begin(const char*,bool){return true;}
 void end(){}
 bool getBool(const char* name,bool defaultVal){
  if(std::strcmp(name,"batshow_v")==0)return mock_battery_display_volts;
  return defaultVal;
 }
 bool isKey(const char* name){return std::strcmp(name,"bg_sqwatch")==0&&mock_background_saved;}
 size_t getBytesLength(const char* name){
  if(std::strcmp(name,"fit")==0)return mock_portrait_fit_saved?28:0;
  if(std::strcmp(name,"touchport")==0&&!mock_portrait_touch_saved)return 0;
  return 10;
 } // simulated previously calibrated touch unless intentionally absent
 size_t getBytes(const char* name,void* output,size_t length){
   // Realistic stored portrait coefficients allow the native edge-rescue
   // test to exercise TFT_eSPI's unsigned outside-panel rejection path.
   std::memset(output,0,length);
   if(std::strcmp(name,"fit")==0&&length==28){
     struct CalFit{float xa,xb,xc,ya,yb,yc;int16_t w0,h0;};
     const CalFit fit{240.f/3600.f,0.f,-200.f*240.f/3600.f,
       0.f,320.f/3700.f,-180.f*320.f/3700.f,240,320};
     std::memcpy(output,&fit,sizeof(fit));
   }
   if(std::strcmp(name,"touchport")==0&&length==10){
     const uint16_t cal[5]={200,3600,180,3700,0};
     std::memcpy(output,cal,sizeof(cal));
   }
   return length;
 }
 size_t putBytes(const char* name,const void*,size_t length){
  if(std::strcmp(name,"touchport")==0)mock_portrait_touch_saved=true;
  if(std::strcmp(name,"fit")==0)mock_portrait_fit_saved=true;
  return length;
 }
 void putBool(const char* name,bool value){
  if(std::strcmp(name,"batshow_v")==0)mock_battery_display_volts=value;
 }
 void remove(const char*){}
};