#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Fixed DJI MA-L prefixes for explicit, bounded manufacturer-OUI test cases.
// These are NOT genuine DJI devices, Remote ID broadcasts or OcuSync traffic.
// No arbitrary MAC input or spoofing API: only these 10 named lab fixtures.
namespace SquachDjiOuiLab {
struct Entry { const char* caseId; uint8_t prefix[3]; };
static const Entry kCases[] = {
  {"OUI-DJI-04A85A", {0x04, 0xA8, 0x5A}},
  {"OUI-DJI-0C9AE6", {0x0C, 0x9A, 0xE6}},
  {"OUI-DJI-34D262", {0x34, 0xD2, 0x62}},
  {"OUI-DJI-481CB9", {0x48, 0x1C, 0xB9}},
  {"OUI-DJI-4C43F6", {0x4C, 0x43, 0xF6}},
  {"OUI-DJI-58B858", {0x58, 0xB8, 0x58}},
  {"OUI-DJI-60601F", {0x60, 0x60, 0x1F}},
  {"OUI-DJI-882985", {0x88, 0x29, 0x85}},
  {"OUI-DJI-8C5823", {0x8C, 0x58, 0x23}},
  {"OUI-DJI-E47A2C", {0xE4, 0x7A, 0x2C}},
};
constexpr size_t kCount = sizeof(kCases)/sizeof(kCases[0]);

inline const Entry* find(const char* caseId) {
  if(!caseId)return nullptr;
  for(size_t i=0;i<kCount;++i)
    if(strcmp(kCases[i].caseId,caseId)==0)return &kCases[i];
  return nullptr;
}
inline bool apply(const char* caseId, uint8_t mac[6]) {
  const Entry* entry=find(caseId);
  if(!entry||!mac)return false;
  // The last 3 bytes come from SquachEmit's per-device, per-case identity.
  // No eFuse update or real aircraft serials.
  memcpy(mac,entry->prefix,3);
  return (mac[0]&0x03)==0; // IEEE global unicast MAC only
}
} // namespace SquachDjiOuiLab
