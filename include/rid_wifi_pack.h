#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// A single **synthetic** ASTM/OpenDroneID-shaped Wi-Fi Beacon vendor IE.
// Mirrors OpenDroneID Core C's standard pack framing:
//   element ID 221, OUI FA:0B:BC, type 0D, counter,
//   0xF2 (message pack v2), one-message-size=25, count=5,
//   5 x 25-byte encoded messages.
// It does NOT by itself establish ASTM F3411-22a operational compliance.
namespace SquachRidWifi {
constexpr size_t kMsgBytes = 25;
constexpr size_t kMsgCount = 5;
constexpr size_t kPackBytes = 3 + kMsgBytes * kMsgCount;
constexpr size_t kVendorDataBytes = 4 + 1 + kPackBytes;
constexpr size_t kIeBytes = 2 + kVendorDataBytes;
static_assert(kIeBytes == 135, "5-message ODID Wi-Fi beacon IE should be 135 bytes");
static_assert(kVendorDataBytes <= 255, "One 802.11 vendor IE is limited to 255 data bytes");

inline bool encode(const uint8_t messages[kMsgCount][kMsgBytes],
                   uint8_t counter, uint8_t* ie, size_t capacity) {
  if (!messages || !ie || capacity < kIeBytes) return false;
  // Enforce the message types required by the receiver test matrix.
  const uint8_t type[kMsgCount] = {0x00, 0x10, 0x40, 0x30, 0x50};
  for(size_t i=0;i<kMsgCount;++i) {
    if ((messages[i][0] & 0xF0) != type[i] || (messages[i][0] & 0x0F) != 2u)
      return false;
  }
  ie[0] = 0xDD;
  ie[1] = (uint8_t)kVendorDataBytes;
  ie[2] = 0xFA; ie[3] = 0x0B; ie[4] = 0xBC;
  ie[5] = 0x0D; // ASTM/ASD-STAN OpenDroneID application type
  ie[6] = counter;
  ie[7] = 0xF2; // Message Pack type 0xF, protocol version 2
  ie[8] = (uint8_t)kMsgBytes;
  ie[9] = (uint8_t)kMsgCount;
  memcpy(ie+10, messages, kMsgCount*kMsgBytes);
  return true;
}
} // namespace SquachRidWifi
