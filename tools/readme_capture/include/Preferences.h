#pragma once
#include <cstring>
#include <cstddef>
#include <cstdint>
class Preferences{
public:
 bool begin(const char*,bool){return true;}
 void end(){}
 bool getBool(const char*,bool defaultVal){return defaultVal;}
 size_t getBytesLength(const char*){return 10;} // simulated previously calibrated touch
 size_t getBytes(const char*,void* output,size_t length){
   std::memset(output,0,length);return length;
 }
 size_t putBytes(const char*,const void*,size_t length){return length;}
 void putBool(const char*,bool){}
 void remove(const char*){}
};