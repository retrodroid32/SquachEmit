#pragma once
#include <stdint.h>
#include <string.h>
// Configure a locally-administered SoftAP MAC before starting AP traffic.
// WiFi.mode(WIFI_STA) initializes ESP-IDF; stop the driver, set WIFI_MODE_AP,
// update WIFI_IF_AP MAC while disabled, then restart. Arduino-ESP32's private
// started flag stays consistent because every successful stop is followed by
// a restart (including after a failed mode or MAC operation).
namespace SquachApMac {
enum class Stage : uint8_t { Ok, StaMode, StopForMac, ApMode, SetMac, Restart, Readback };
struct Result {
  Stage stage;
  int32_t code;
  bool ok() const { return stage == Stage::Ok; }
};
inline const char* stageName(Stage step) {
  switch(step) {
    case Stage::Ok: return "OK";
    case Stage::StaMode: return "STA_MODE";
    case Stage::StopForMac: return "AP_STOP_FOR_MAC";
    case Stage::ApMode: return "AP_MODE";
    case Stage::SetMac: return "AP_MAC_SET";
    case Stage::Restart: return "AP_RESTART";
    case Stage::Readback: return "AP_MAC_READBACK";
  }
  return "AP_UNKNOWN";
}
template<typename Driver>
Result configure(Driver& driver, const uint8_t mac[6]) {
  if(!driver.startSta())return {Stage::StaMode,-1};
  const int32_t stopError=driver.stop();
  if(stopError)return {Stage::StopForMac,stopError};
  const int32_t modeError=driver.setApMode();
  const int32_t macError=modeError ? 0 : driver.setMac(mac);
  // Always restart after a successful stop: Arduino's started state is
  // tracked internally and must match ESP-IDF's state at return.
  const int32_t startError=driver.restart();
  if(modeError)return {Stage::ApMode,modeError};
  if(macError)return {Stage::SetMac,macError};
  if(startError)return {Stage::Restart,startError};
  uint8_t readback[6]={};
  const int32_t readError=driver.getMac(readback);
  if(readError || memcmp(mac,readback,6))
    return {Stage::Readback,readError ? readError : -1};
  return {Stage::Ok,0};
}
}
