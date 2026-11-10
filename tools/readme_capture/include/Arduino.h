#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstdlib>
inline uint32_t mock_time_ms = 1000;
inline uint32_t millis(){ return mock_time_ms; }
inline void pinMode(int,int){}
inline void digitalWrite(int,int){}
constexpr int OUTPUT=1,LOW=0,HIGH=1;
constexpr int TFT_BL=27;
struct SerialMock {
  void println(const char* s){ fprintf(stderr,"%s\n",s); }
  template<typename... Ts> void printf(const char* fmt, Ts... a){ fprintf(stderr,fmt,a...); }
};
inline SerialMock Serial;