// SquachEmit landscape speech timing.
// Match SquachWatch's cadence, not its receiver-specific dialogue:
// initial quip after 15-30s; subsequent gaps 9-22s, minimum 5s visible,
// 220ms pop-in. State changes get one immediate, time-limited announcement.
// No TFT or RF dependencies; host fixtures use a deterministic RNG.
#pragma once
#include <stdint.h>

namespace SqeBubble {
enum Kind : uint8_t { Idle, OnAir, Starting, Failed, Suite, Playlist };
struct State {
  uint8_t context=255;
  uint8_t phrase=0;
  bool showing=false;
  uint32_t started=0;
  uint32_t until=0;
  uint32_t next=0;
};
inline uint8_t phraseCount(Kind kind){
  switch(kind){
    case Idle: return 5;
    case OnAir: return 3;
    default: return 1;
  }
}
// randomRange signature matches Arduino random(min,max).
inline bool update(State& s, uint32_t now, Kind kind,
                   long (*randomRange)(long,long)){
  if(s.context!=(uint8_t)kind){
    s.context=(uint8_t)kind;
    s.showing=(kind!=Idle);
    s.started=now;
    s.until=now+5000u;
    s.next=now+15000u+(uint32_t)randomRange(0,15000);
    if(s.showing)s.phrase=(uint8_t)randomRange(0,phraseCount(kind));
    return true;
  }
  if(s.showing){
    if((int32_t)(now-s.until)>=0){
      s.showing=false;
      s.next=now+9000u+(uint32_t)randomRange(0,13000);
      return true;
    }
    return false;
  }
  if((int32_t)(now-s.next)>=0){
    s.showing=true;
    s.phrase=(uint8_t)randomRange(0,phraseCount(kind));
    s.started=now;
    s.until=now+5000u;
    return true;
  }
  return false;
}
inline bool visible(const State& s,uint32_t now){
  return s.showing&&(int32_t)(now-s.until)<0;
}
inline float popScale(const State& s,uint32_t now){
  if(!visible(s,now))return 0.f;
  const uint32_t elapsed=now>=s.started?now-s.started:0u;
  if(elapsed>=220u)return 1.f;
  const float t=(float)elapsed/220.f;
  // Small 3% overshoot, clamped to the 320px display margins.
  return t<0.75f ? (t/0.75f)*1.03f :
      1.03f-(t-0.75f)/0.25f*0.03f;
}
} // namespace SqeBubble
