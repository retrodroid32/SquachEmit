#pragma once
#include "scenarios.h"
#include <string.h>

// One authoritative device-type assignment per scenario, independent of
// BLE/Wi-Fi carrier. Categories are alphabetized for the touch menu.
enum class DeviceCategory : uint8_t {
  Alpr, Axon, Cameras, Drones, Flock, General, Hacker, IBeacon,
  Listeners, Raven, Ring, SerialModules, SmartGlasses, Trackers, Count
};
constexpr size_t kDeviceCategoryCount = static_cast<size_t>(DeviceCategory::Count);
inline const char* deviceCategoryName(DeviceCategory c) {
  static const char* const names[] = {
    "ALPR", "AXON", "CAMERAS / DASHCAMS", "DRONES / REMOTE ID",
    "FLOCK", "GENERAL / CONTROLS", "HACKER DEVICES", "IBEACON",
    "LISTENERS", "RAVEN", "RING", "SERIAL MODULES",
    "SMART GLASSES", "TRACKERS"
  };
  const size_t i=static_cast<size_t>(c);
  return i<kDeviceCategoryCount?names[i]:"GENERAL / CONTROLS";
}
inline bool startsWith(const char* s,const char* p) {
  return strncmp(s,p,strlen(p))==0;
}
inline DeviceCategory deviceCategory(const Scenario& s) {
  const char* id=s.id;
  // New v1.50 category-specific fixtures preserve the exact expected
  // detector label, while negative controls stay with their signal family.
  if(startsWith(id,"V150-")||startsWith(id,"FORK-OUI-")){
    if(strcmp(s.expected,"ALPR")==0)return DeviceCategory::Alpr;
    if(strcmp(s.expected,"CAMERA")==0)return DeviceCategory::Cameras;
    if(strcmp(s.expected,"LISTENER")==0)return DeviceCategory::Listeners;
    if(strcmp(s.expected,"RING")==0)return DeviceCategory::Ring;
    if(strcmp(s.expected,"AXON")==0)return DeviceCategory::Axon;
    if(strcmp(s.expected,"FLOCK")==0)return DeviceCategory::Flock;
    if(strcmp(s.expected,"DRONE")==0)return DeviceCategory::Drones;
    if(strcmp(s.expected,"RAVEN")==0)return DeviceCategory::Raven;
    if(strcmp(s.expected,"META")==0)return DeviceCategory::SmartGlasses;
    if(strcmp(s.expected,"SKIMMER")==0)return DeviceCategory::SerialModules;
    if(strcmp(s.expected,"HACKER")==0)return DeviceCategory::Hacker;
    if(strcmp(s.expected,"GOOGLE_TAG")==0 ||
       strcmp(s.expected,"TILE")==0 ||
       strcmp(s.expected,"AIRTAG")==0)return DeviceCategory::Trackers;
    // Negative control remains beside the family it rules out.
    if(strstr(id,"AIRPODS")||strstr(id,"GOOGLE-"))return DeviceCategory::Trackers;
    if(strstr(id,"CAM-"))return DeviceCategory::Cameras;
    return DeviceCategory::General;
  }
  // Negative and malformed fixtures stay with the device family they test.
  if(startsWith(id,"RID-") || startsWith(id,"OUI-") ||
     strcmp(id,"BLE-DRONE")==0 ||
     strcmp(id,"BLE-NEG-DRONE-UUID")==0) return DeviceCategory::Drones;
  if(startsWith(id,"BLE-AXON") || startsWith(id,"WIFI-AXON") ||
     strcmp(id,"WIFI-NEG-AXON-NOHYPHEN")==0) return DeviceCategory::Axon;
  if(startsWith(id,"BLE-FLOCK") || startsWith(id,"WIFI-FLOCK") ||
     strcmp(id,"BLE-NEG-PENGUIN-9")==0 ||
     strcmp(id,"BLE-NEG-BARE9")==0 ||
     strcmp(id,"WIFI-NEG-FLOCKHART")==0) return DeviceCategory::Flock;
  if(startsWith(id,"BLE-META") ||
     strcmp(id,"BLE-NEG-META-ADJ")==0 ||
     strcmp(id,"BLE-NEG-SNAP-ADJ")==0) return DeviceCategory::SmartGlasses;
  if(startsWith(id,"BLE-RAVEN") ||
     strcmp(id,"BLE-NEG-RAVEN-3600")==0) return DeviceCategory::Raven;
  if(startsWith(id,"BLE-SKIM") || startsWith(id,"FORK-BLE-SKIM-") ||
     strcmp(id,"BLE-NEG-JDY16-EXTRA")==0) return DeviceCategory::SerialModules;
  if(startsWith(id,"BLE-HACKER") || startsWith(id,"WIFI-HACKER") ||
     strcmp(id,"BLE-NEG-FLIP-MFG")==0 ||
     startsWith(id,"WIFI-NEG-PAGER-")) return DeviceCategory::Hacker;
  if(startsWith(id,"BLE-IBEACON") ||
     strcmp(id,"BLE-NEG-IBEACON-SHORT")==0) return DeviceCategory::IBeacon;
  if(startsWith(id,"BLE-AIRTAG") || startsWith(id,"BLE-TILE") ||
     startsWith(id,"BLE-SAMSUNG") || startsWith(id,"BLE-GOOGLE") ||
     strcmp(id,"BLE-NEG-APPLE")==0) return DeviceCategory::Trackers;
  return DeviceCategory::General;  // only unassigned/generic control cases
}
// Alphabetical order by stable, user-visible test ID. No duplicate exposure.
inline bool deviceCategoryCaseAtRank(DeviceCategory category,size_t rank,
                                     size_t* rawIndex) {
  size_t previous=kScenarioCount;
  for(size_t position=0;position<=rank;++position){
    size_t smallest=kScenarioCount;
    for(size_t i=0;i<kScenarioCount;++i) {
      if(deviceCategory(kScenarios[i])!=category)continue;
      if(previous!=kScenarioCount &&
         strcmp(kScenarios[i].id,kScenarios[previous].id)<=0)continue;
      if(smallest==kScenarioCount ||
         strcmp(kScenarios[i].id,kScenarios[smallest].id)<0)smallest=i;
    }
    if(smallest==kScenarioCount)return false;
    previous=smallest;
  }
  if(rawIndex)*rawIndex=previous;
  return true;
}
inline size_t deviceCategoryCaseCount(DeviceCategory category) {
  size_t count=0;
  for(size_t i=0;i<kScenarioCount;++i)
    if(deviceCategory(kScenarios[i])==category)++count;
  return count;
}
