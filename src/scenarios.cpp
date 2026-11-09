#include "scenarios.h"
#include <string.h>

// Source of expected labels: SquachWatch-CYD include/state.h, signatures.cpp
// and detection.cpp (master as inspected 2026-11-08).
const Scenario kScenarios[] = {
  {"BLE-FLOCK-MFG", Kind::BleManufacturer, "FLOCK", "Flock supplier company ID 0x09C8", 0x09C8, nullptr},
  {"BLE-FLOCK-NAME", Kind::BleName, "FLOCK", "Synthetic Penguin battery name", 0, "Penguin-0123456789"},
  {"BLE-META-UUID", Kind::BleUuid, "META", "Ray-Ban Meta service UUID", 0xFD5F, nullptr},
  {"BLE-META-MFG", Kind::BleManufacturer, "META", "Meta company ID", 0x01AB, nullptr},
  {"BLE-SKIMMER", Kind::BleName, "SKIMMER", "HC-05-like BLE name; NOT Bluetooth Classic", 0, "HC-05"},
  {"BLE-RAVEN", Kind::BleUuid, "RAVEN", "Raven service UUID", 0x3100, nullptr},
  {"BLE-AIRTAG", Kind::BleManufacturer, "AIRTAG", "Synthetic Apple Find My pattern", 0x004C, nullptr},
  {"BLE-DRONE", Kind::BleRemoteId, "DRONE", "Three synthetic OpenDroneID Basic/Location/System frames", 0xFFFA, nullptr},
  {"BLE-SAMSUNG", Kind::BleUuid, "SAMSUNG_TAG", "Samsung SmartTag service UUID", 0xFD5A, nullptr},
  {"BLE-GOOGLE", Kind::BleUuid, "GOOGLE_TAG", "Google Eddystone/FMDN shared UUID", 0xFEAA, nullptr},
  {"BLE-TILE", Kind::BleUuid, "TILE", "Tile service UUID", 0xFEED, nullptr},
  {"BLE-HACKER-UUID", Kind::BleUuid, "HACKER", "Flipper signature UUID", 0x3081, nullptr},
  {"BLE-HACKER-MFG", Kind::BleManufacturer, "HACKER", "Flipper company ID", 0x0E29, nullptr},
  {"BLE-IBEACON", Kind::BleManufacturer, "IBEACON", "Synthetic iBeacon; enable IBEACON in SquachWatch first", 0x004C, nullptr},
  {"BLE-NEG-APPLE", Kind::BleAppleNegative, "NONE", "Apple company ID without Find My/iBeacon structure", 0x004C, nullptr},
  {"BLE-NEG-UUID", Kind::BleUuid, "NONE", "Unknown service UUID", 0xA7A7, nullptr},
  {"BLE-NEG-NAME", Kind::BleName, "NONE", "Ordinary BLE device name", 0, "SQE-BENIGN"},
  {"WIFI-AXON", Kind::WifiAp, "AXON", "Body-camera pairing SSID prefix", 0, "AB2-SQE-TEST"},
  {"WIFI-FLOCK", Kind::WifiAp, "FLOCK", "Flock test SSID prefix", 0, "FLOCK-SQE-TEST"},
  {"WIFI-HACKER-PINE", Kind::WifiAp, "HACKER", "Wi-Fi Pineapple SSID pattern", 0, "Pineapple_SQE"},
  {"WIFI-HACKER-PWN", Kind::WifiAp, "HACKER", "Deauther management SSID pattern; no attacks", 0, "pwned-SQE-TEST"},
  {"WIFI-NEG", Kind::WifiAp, "NONE", "Ordinary WPA2 SoftAP; negative control", 0, "SQE-BENIGN"},
};
const size_t kScenarioCount = sizeof(kScenarios) / sizeof(kScenarios[0]);
const Scenario* findScenario(const char* id) {
  if (!id) return nullptr;
  for (size_t i = 0; i < kScenarioCount; ++i)
    if (strcmp(kScenarios[i].id, id) == 0) return &kScenarios[i];
  return nullptr;
}
