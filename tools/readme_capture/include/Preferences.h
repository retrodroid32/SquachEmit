#pragma once
#include <cstring>
#include <cstddef>
#include <cstdint>
inline uint8_t mock_saved_tx_time=12;
// Default remains calibrated for existing host fixtures. New RF-safe
// portrait tests can set false to emulate a freshly erased portrait key.
inline bool mock_portrait_touch_saved=true;
inline bool mock_background_saved=false;
inline uint8_t mock_background_value=9;
class Preferences{
public:
 uint8_t getUChar(const char* name,uint8_t fallback){
  if(std::strcmp(name,"txtime")==0)return mock_saved_tx_time?mock_saved_tx_time:fallback;
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
 bool getBool(const char*,bool defaultVal){return defaultVal;}
 bool isKey(const char* name){return std::strcmp(name,"bg_sqwatch")==0&&mock_background_saved;}
 size_t getBytesLength(const char* name){
  if(std::strcmp(name,"touchport")==0&&!mock_portrait_touch_saved)return 0;
  return 10;
 } // simulated previously calibrated touch unless intentionally absent
 size_t getBytes(const char*,void* output,size_t length){
   std::memset(output,0,length);return length;
 }
 size_t putBytes(const char* name,const void*,size_t length){
  if(std::strcmp(name,"touchport")==0)mock_portrait_touch_saved=true;
  return length;
 }
 void putBool(const char*,bool){}
 void remove(const char*){}
};