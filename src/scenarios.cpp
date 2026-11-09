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
  // LAB channel test variants: Wi-Fi code 0 means legacy CH1; otherwise CH1..11.
  {"BLE-IBEACON-V2", Kind::BleManufacturer, "IBEACON", "Second synthetic iBeacon UUID/major/minor identity", 76, nullptr},
  {"BLE-NEG-IBEACON-SHORT", Kind::BleAppleNegative, "NONE", "Truncated Apple iBeacon structure; reject as incomplete", 76, nullptr},
  {"BLE-NEG-DRONE-UUID", Kind::BleUuid, "NONE", "OpenDroneID UUID as service list only; NOT Remote ID data", 65530, nullptr},
  {"BLE-NEG-META-ADJ", Kind::BleUuid, "NONE", "Adjacent non-Meta service UUID should not classify", 64862, nullptr},
  {"WIFI-AXON-CH6", Kind::WifiAp, "AXON", "Controlled Axon SSID on Wi-Fi channel 6", 6, "AB2-SQE-TEST"},
  {"WIFI-AXON-CH11", Kind::WifiAp, "AXON", "Controlled Axon SSID on Wi-Fi channel 11", 11, "AB2-SQE-TEST"},
  {"WIFI-FLOCK-CH6", Kind::WifiAp, "FLOCK", "Controlled Flock SSID on Wi-Fi channel 6", 6, "FLOCK-SQE-TEST"},
  {"WIFI-NEG-CH6", Kind::WifiAp, "NONE", "Benign WPA2 AP on Wi-Fi channel 6", 6, "SQE-BENIGN"},
  {"WIFI-NEG-CH11", Kind::WifiAp, "NONE", "Benign WPA2 AP on Wi-Fi channel 11", 11, "SQE-BENIGN"},
  // Synthetic Remote ID family/model profiles; not vendor radio impersonation.
  {"RID-DJI-MINI", Kind::BleRemoteId, "DRONE", "LAB DJI Mini Remote ID profile (synthetic, not real aircraft)", 0, "SQE-DJI-MINI-LAB01"},
  {"RID-DJI-AIR", Kind::BleRemoteId, "DRONE", "LAB DJI Air Remote ID profile (synthetic, not real aircraft)", 0, "SQE-DJI-AIR-LAB001"},
  {"RID-DJI-MAVIC", Kind::BleRemoteId, "DRONE", "LAB DJI Mavic Remote ID profile (synthetic, not real aircraft)", 0, "SQE-DJI-MAVIC-LAB1"},
  {"RID-DJI-PHANTOM", Kind::BleRemoteId, "DRONE", "LAB DJI Phantom Remote ID profile (synthetic, not real aircraft)", 0, "SQE-DJI-PHANTOM-L1"},
  {"RID-DJI-MATRICE", Kind::BleRemoteId, "DRONE", "LAB DJI Matrice Remote ID profile (synthetic, not real aircraft)", 0, "SQE-DJI-MATRICE-L1"},
  {"RID-AUTEL-EVO2", Kind::BleRemoteId, "DRONE", "LAB Autel EVO II Remote ID profile (synthetic, not real aircraft)", 0, "SQE-AUTEL-EVO2-L1"},
  {"RID-AUTEL-EVOMAX", Kind::BleRemoteId, "DRONE", "LAB Autel EVO Max Remote ID profile (synthetic, not real aircraft)", 0, "SQE-AUTEL-MAX-L01"},
  {"RID-AUTEL-EVOLITE", Kind::BleRemoteId, "DRONE", "LAB Autel EVO Lite Remote ID profile (synthetic, not real aircraft)", 0, "SQE-AUTEL-LITE-L1"},
  {"RID-PARROT-ANAFI", Kind::BleRemoteId, "DRONE", "LAB Parrot ANAFI Remote ID profile (synthetic, not real aircraft)", 0, "SQE-PARROT-ANAFI1"},
  {"RID-PARROT-ANAFIUSA", Kind::BleRemoteId, "DRONE", "LAB Parrot ANAFI USA Remote ID profile (synthetic, not real aircraft)", 0, "SQE-PARROT-USA-L01"},
  {"RID-GENERIC-BLE4", Kind::BleRemoteId, "DRONE", "Generic BLE4 Remote ID test profile with Self ID", 0, "SQE-GENERIC-BLE4-L1"},
  {"RID-WIFI-BEACON", Kind::WifiRemoteId, "DRONE", "2.4GHz Wi-Fi beacon with ASTM Remote ID vendor IE", 6, "SQE-RID-GENERIC"},
  {"RID-WIFI-PARROT", Kind::WifiRemoteId, "DRONE", "2.4GHz Wi-Fi beacon with Parrot Remote ID vendor IE", 6, "SQE-RID-PARROT"},
  {"RID-NEG-APP", Kind::BleRemoteNegative, "NONE", "BLE Remote ID UUID with wrong application code", 1, nullptr},
  {"RID-NEG-SHORT", Kind::BleRemoteNegative, "NONE", "BLE Remote ID service data one byte too short", 2, nullptr},
  {"RID-NO-OPERATOR", Kind::BleRemoteId, "DRONE", "Valid Basic+Location; omit System/operator data", 0, "SQE-NO-OP-LAB00001"},
  {"RID-STALE-LOC", Kind::BleRemoteId, "DRONE", "Valid Remote ID with old location time-of-hour (not a receiver clock test)", 0, "SQE-STALE-LOC-LAB1"},
  {"RID-BAD-SERIAL", Kind::BleRemoteId, "DRONE", "Valid Remote ID transport with invalid synthetic Basic ID text", 0, "SQE-BAD-SERIAL-LAB"},
};
const size_t kScenarioCount = sizeof(kScenarios) / sizeof(kScenarios[0]);
const Scenario* findScenario(const char* id) {
  if (!id) return nullptr;
  for (size_t i = 0; i < kScenarioCount; ++i)
    if (strcmp(kScenarios[i].id, id) == 0) return &kScenarios[i];
  return nullptr;
}
