#pragma once
#include <stdint.h>
#include <stddef.h>

// Filter for a classic ESP32 ADC1 battery divider: robust readings and a
// deliberately coarse VOLTAGE-BASED charge estimate (not a fuel gauge).
namespace SqeBattery {
static constexpr size_t kSamples=32;

inline uint16_t trimmedAdcMillivolts(const uint16_t (&values)[kSamples],
                                    uint16_t& spread){
  uint16_t sorted[kSamples];
  for(size_t i=0;i<kSamples;++i)sorted[i]=values[i];
  // This runs only once per 15-second window, not once per frame.
  for(size_t i=1;i<kSamples;++i){
    const uint16_t v=sorted[i];
    size_t j=i;
    while(j&&sorted[j-1]>v){sorted[j]=sorted[j-1];--j;}
    sorted[j]=v;
  }
  spread=(uint16_t)(sorted[kSamples-1]-sorted[0]);
  uint32_t sum=0;
  for(size_t i=6;i<kSamples-6;++i)sum+=sorted[i];
  return (uint16_t)((sum+10u)/20u);
}
inline int voltagePercent(uint16_t mv){
  static constexpr uint16_t volts[]={3000,3300,3500,3600,3700,3750,3800,3850,3900,4000,4100,4200};
  static constexpr uint8_t percents[]={0,5,10,15,25,35,45,55,65,80,92,100};
  if(mv<=volts[0])return 0;
  for(size_t i=1;i<sizeof(volts)/sizeof(volts[0]);++i){
    if(mv<=volts[i]){
      return percents[i-1]+(int)((uint32_t)(mv-volts[i-1])*
             (percents[i]-percents[i-1])/(volts[i]-volts[i-1]));
    }
  }
  return 100;
}
struct Meter{
  uint16_t rawMillivolts=0;
  uint16_t filteredMillivolts=0;
  uint16_t sampleSpreadMillivolts=0;
  int8_t displayedPercent=-1;
  uint8_t badWindows=0;
  bool accept(const uint16_t (&adc)[kSamples]){
    uint16_t spread=0;
    const uint32_t mv=2u*(uint32_t)trimmedAdcMillivolts(adc,spread);
    sampleSpreadMillivolts=(uint16_t)(2u*spread);
    if(mv<2800u||mv>4350u){
      if(++badWindows>=3){
        displayedPercent=-1;filteredMillivolts=0;rawMillivolts=0;
      }
      return false;
    }
    badWindows=0;
    rawMillivolts=(uint16_t)mv;
    if(!filteredMillivolts)filteredMillivolts=(uint16_t)mv;
    else filteredMillivolts=(uint16_t)(((uint32_t)filteredMillivolts*3u+mv+2u)/4u);
    const int estimate=voltagePercent(filteredMillivolts);
    int coarse=(estimate+2)/5*5;
    if(coarse>100)coarse=100;
    // 5% display steps and 4% hysteresis to prevent bouncing around
    // threshold boundaries. Never claim one-percent fuel-gauge accuracy.
    if(displayedPercent<0||estimate>=displayedPercent+4||
       estimate<=displayedPercent-4)displayedPercent=(int8_t)coarse;
    return true;
  }
};
} // namespace SqeBattery
