#pragma once
#include <stdint.h>
#include <stddef.h>

// The only on-air modes included are ordinary BLE advertisements and ordinary
// Wi-Fi SoftAP beacons. No forged 802.11 management traffic or deauth floods.
enum class Kind : uint8_t { BleUuid, BleManufacturer, BleName, BleRemoteId,
                            WifiAp, BleAppleNegative };
struct Scenario {
  const char* id;
  Kind kind;
  const char* expected; // SquachWatch DetectionType, or NONE
  const char* description;
  uint16_t code;         // 16-bit UUID or Bluetooth manufacturer company ID
  const char* text;      // BLE name or SoftAP SSID
};

extern const Scenario kScenarios[];
extern const size_t kScenarioCount;
const Scenario* findScenario(const char* id);
