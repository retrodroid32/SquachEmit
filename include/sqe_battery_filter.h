#pragma once
#include <stdint.h>
#include <stddef.h>

// E32R32P battery sense: GPIO34 through 100k/100k divider (50k Thevenin).
// Voltage is only an estimate of state of charge, not a fuel gauge.
namespace SqeBattery {
static constexpr size_t kSamples=32;
static constexpr size_t kConsensusWindows=4;
static constexpr uint16_t kMaxWindowSpanMv=60; // battery-side agreement

inline uint16_t trimmedAdcMillivolts(const uint16_t (&values)[kSamples],
                                    uint16_t& spread,uint16_t& coreSpread){
  uint16_t sorted[kSamples];
  for(size_t i=0;i<kSamples;++i)sorted[i]=values[i];
  for(size_t i=1;i<kSamples;++i){
    const uint16_t v=sorted[i];
    size_t j=i;
    while(j&&sorted[j-1]>v){sorted[j]=sorted[j-1];--j;}
    sorted[j]=v;
  }
  spread=(uint16_t)(sorted[kSamples-1]-sorted[0]);
  coreSpread=(uint16_t)(sorted[kSamples-7]-sorted[6]);
  uint32_t sum=0;
  for(size_t i=6;i<kSamples-6;++i)sum+=sorted[i];
  return (uint16_t)((sum+10u)/20u);
}
inline int voltagePercent(uint16_t mv){
  // Approximate *resting* single-cell Li-ion voltage to capacity table.
  // USB charging and active radio load make true SOC unobservable.
  static constexpr uint16_t volts[]={3000,3300,3500,3600,3700,3750,3800,3850,3900,4000,4100,4200};
  static constexpr uint8_t percents[]={0,5,10,15,25,35,45,55,65,80,92,100};
  if(mv<=volts[0])return 0;
  for(size_t i=1;i<sizeof(volts)/sizeof(volts[0]);++i){
    if(mv<=volts[i])
      return percents[i-1]+(int)((uint32_t)(mv-volts[i-1])*
             (percents[i]-percents[i-1])/(volts[i]-volts[i-1]));
  }
  return 100;
}
struct Meter {
  uint16_t rawMillivolts=0;
  uint16_t filteredMillivolts=0;
  uint16_t sampleSpreadMillivolts=0;
  uint16_t coreSpreadMillivolts=0;
  int8_t displayedPercent=-1;
  uint16_t windows[kConsensusWindows]={};
  uint8_t windowCount=0,windowHead=0,badWindows=0,unstableWindows=0;

  void clearEstimate(){
    displayedPercent=-1;
    filteredMillivolts=0;
    windowCount=0;
    windowHead=0;
    unstableWindows=0;
  }
  bool accept(const uint16_t (&adc)[kSamples]){
    uint16_t spread=0,core=0;
    const uint32_t mv=2u*(uint32_t)trimmedAdcMillivolts(adc,spread,core);
    sampleSpreadMillivolts=(uint16_t)(2u*spread);
    coreSpreadMillivolts=(uint16_t)(2u*core);
    // Reject noisy samples even if their average happens to be plausible.
    if(mv<2800u||mv>4350u||coreSpreadMillivolts>80u){
      if(++badWindows>=3){rawMillivolts=0;clearEstimate();}
      return false;
    }
    badWindows=0;
    rawMillivolts=(uint16_t)mv;
    windows[windowHead]=(uint16_t)mv;
    windowHead=(uint8_t)((windowHead+1u)%kConsensusWindows);
    if(windowCount<kConsensusWindows)++windowCount;
    // Never publish an instantaneous post-reboot reading as a true SOC.
    if(windowCount<kConsensusWindows)return true;
    uint16_t lo=windows[0],hi=windows[0];
    uint16_t vals[kConsensusWindows];
    for(size_t i=0;i<kConsensusWindows;++i){
      const uint16_t v=windows[i];
      vals[i]=v;
      if(v<lo)lo=v;
      if(v>hi)hi=v;
    }
    if((uint16_t)(hi-lo)>kMaxWindowSpanMv){
      // Report unknown rather than sticking to a confidently wrong 80-95%.
      if(++unstableWindows>=2) {
        displayedPercent=-1;
        filteredMillivolts=0;
      }
      return true;
    }
    unstableWindows=0;
    for(size_t i=1;i<kConsensusWindows;++i){
      const uint16_t v=vals[i];
      size_t j=i;
      while(j&&vals[j-1]>v){vals[j]=vals[j-1];--j;}
      vals[j]=v;
    }
    const uint16_t median=(uint16_t)(((uint32_t)vals[1]+vals[2]+1u)/2u);
    if(!filteredMillivolts)filteredMillivolts=median;
    else filteredMillivolts=(uint16_t)(((uint32_t)filteredMillivolts*3u+median+2u)/4u);
    const int estimate=voltagePercent(filteredMillivolts);
    int coarse=(estimate+2)/5*5;
    if(coarse>100)coarse=100;
    // 5% steps, 6-point hysteresis. Do not manufacture 1% precision.
    if(displayedPercent<0||estimate>=displayedPercent+6||
       estimate<=displayedPercent-6)displayedPercent=(int8_t)coarse;
    return true;
  }
};
} // namespace SqeBattery
