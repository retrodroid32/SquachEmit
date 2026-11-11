#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

// Explicit, bounded manufacturer-prefix Wi-Fi lab beacons only.
// NOT authentic drones, radio links, or standard Remote ID transmissions.
// Preserve the full IEEE assignment length (MA-L 24, MA-M 28, MA-S/IAB 36 bits).
namespace SquachUavVendorLab {
struct Entry { const char* caseId; uint8_t bits; uint8_t prefix[5]; };
static const Entry kCases[] = {
  {"OUI-AUTEL-18D7936", 28, {0x18, 0xD7, 0x93, 0x60, 0x00}},
  {"OUI-AUTEL-EC5BCDE", 28, {0xEC, 0x5B, 0xCD, 0xE0, 0x00}},
  {"OUI-PARROT-00121C", 24, {0x00, 0x12, 0x1C, 0x00, 0x00}},
  {"OUI-PARROT-00267E", 24, {0x00, 0x26, 0x7E, 0x00, 0x00}},
  {"OUI-PARROT-9003B7", 24, {0x90, 0x03, 0xB7, 0x00, 0x00}},
  {"OUI-PARROT-903AE6", 24, {0x90, 0x3A, 0xE6, 0x00, 0x00}},
  {"OUI-PARROT-A0143D", 24, {0xA0, 0x14, 0x3D, 0x00, 0x00}},
  {"OUI-SKYDIO-381D14", 24, {0x38, 0x1D, 0x14, 0x00, 0x00}},
  {"OUI-DJIB-9C5A8A", 24, {0x9C, 0x5A, 0x8A, 0x00, 0x00}},
  {"OUI-DJIB-EC72F7", 24, {0xEC, 0x72, 0xF7, 0x00, 0x00}},
  {"OUI-QUANTUM-AC86D17", 28, {0xAC, 0x86, 0xD1, 0x70, 0x00}},
  {"OUI-INSITU-0050C2C35", 36, {0x00, 0x50, 0xC2, 0xC3, 0x50}},
  {"OUI-INSITU-0050C2FD4", 36, {0x00, 0x50, 0xC2, 0xFD, 0x40}},
  {"OUI-INSITU-70B3D57AD", 36, {0x70, 0xB3, 0xD5, 0x7A, 0xD0}},
  {"OUI-INSITU-70B3D5865", 36, {0x70, 0xB3, 0xD5, 0x86, 0x50}},
  {"OUI-INSITU-70B3D5B3B", 36, {0x70, 0xB3, 0xD5, 0xB3, 0xB0}},
  {"OUI-INSITU-70B3D5D36", 36, {0x70, 0xB3, 0xD5, 0xD3, 0x60}},
  {"OUI-INSITU-70B3D5DD2", 36, {0x70, 0xB3, 0xD5, 0xDD, 0x20}},
  {"OUI-FLIR-00407F", 24, {0x00, 0x40, 0x7F, 0x00, 0x00}},
  {"OUI-XAG-A45129", 24, {0xA4, 0x51, 0x29, 0x00, 0x00}},
  {"OUI-UAVNAV-B44D43A", 28, {0xB4, 0x4D, 0x43, 0xA0, 0x00}},
  {"OUI-UAVIONIX-04714B1", 28, {0x04, 0x71, 0x4B, 0x10, 0x00}},
  {"OUI-UAVIONIX-546F71", 24, {0x54, 0x6F, 0x71, 0x00, 0x00}},
  {"OUI-MICROH-000F92", 24, {0x00, 0x0F, 0x92, 0x00, 0x00}},
  {"OUI-HUBSAN-98AAFC7", 28, {0x98, 0xAA, 0xFC, 0x70, 0x00}},
  {"OUI-YUNEEC-E0B6F58", 28, {0xE0, 0xB6, 0xF5, 0x80, 0x00}},
  {"OUI-FIMI-6CDFFBE", 28, {0x6C, 0xDF, 0xFB, 0xE0, 0x00}},
};
constexpr size_t kCount=sizeof(kCases)/sizeof(kCases[0]);
inline const Entry* find(const char* id) {
  if(!id)return nullptr;
  for(size_t i=0;i<kCount;++i)
    if(strcmp(kCases[i].caseId,id)==0)return &kCases[i];
  return nullptr;
}
inline bool apply(const char* id,uint8_t mac[6]) {
  const Entry* e=find(id);
  if(!e || !mac || (e->bits!=24 && e->bits!=28 && e->bits!=36))return false;
  const size_t complete=e->bits/8;
  memcpy(mac,e->prefix,complete);
  if(e->bits%8) {
    // Preserve generated suffix's lower nibble, not the next registrant's.
    mac[complete]=(uint8_t)((mac[complete]&0x0F)|(e->prefix[complete]&0xF0));
  }
  return (mac[0]&3)==0;
}
}
