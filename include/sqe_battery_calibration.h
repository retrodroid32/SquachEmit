#pragma once
#include <stdint.h>

// E32R32P BAT_ADC correction, in battery-side millivolts. Keep it separate
// from the ADC/noise filter and the uncalibrated state-of-charge estimate.
namespace SqeBatteryCalibration {
static constexpr int16_t kMinOffsetMv=-500;
static constexpr int16_t kMaxOffsetMv=500;
inline int16_t clampOffset(int32_t value){
  return (int16_t)(value<kMinOffsetMv?kMinOffsetMv:
    value>kMaxOffsetMv?kMaxOffsetMv:value);
}
inline uint16_t correctedMv(uint16_t rawMv,int16_t offsetMv){
  const int32_t corrected=(int32_t)rawMv+(int32_t)offsetMv;
  return (uint16_t)(corrected<0?0:corrected>5000?5000:corrected);
}
// User-defined linear battery display: 3.20V=0% and 4.20V=100%.
inline uint8_t linearPercent(uint16_t batteryMv) {
  if(batteryMv<=3200u)return 0;
  if(batteryMv>=4200u)return 100;
  return (uint8_t)(((unsigned)batteryMv-3200u+5u)/10u);
}
inline uint8_t roundedPercent5(uint16_t batteryMv) {
  const unsigned percent=linearPercent(batteryMv);
  const unsigned rounded=((percent+2u)/5u)*5u;
  return (uint8_t)(rounded>100u?100u:rounded);
}
inline unsigned displayStep50mV(uint16_t rawMv,int16_t offsetMv){
  return ((unsigned)correctedMv(rawMv,offsetMv)+25u)/50u;
}
} // namespace SqeBatteryCalibration
