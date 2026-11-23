// Fork-only SquachWatch OUI fixtures pinned to src/signatures.cpp blob 1f63a08ce3eba4148d557312bb0d1abe8b18545c.
// Temporary synthetic AP BSSIDs only; not genuine commercial devices.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace SquachForkOuiLab {
struct Entry {const char* id;uint8_t prefix[3];};
static const Entry kCases[]={
  {"FORK-OUI-FLOCK-3C71BF",{0x3C,0x71,0xBF}},
  {"FORK-OUI-FLOCK-4827EA",{0x48,0x27,0xEA}},
  {"FORK-OUI-FLOCK-A4CF12",{0xA4,0xCF,0x12}},
  {"FORK-OUI-DRONE-B030C8",{0xB0,0x30,0xC8}},
  {"FORK-OUI-DRONE-EC715E",{0xEC,0x71,0x5E}},
  {"FORK-OUI-DRONE-001AF9",{0x00,0x1A,0xF9}},
  {"FORK-OUI-DRONE-547D40",{0x54,0x7D,0x40}},
  {"FORK-OUI-DRONE-74B80F",{0x74,0xB8,0x0F}},
  {"FORK-OUI-SKIMMER-98D300",{0x98,0xD3,0x00}},
  {"FORK-OUI-HACKER-0CFA22",{0x0C,0xFA,0x22}},
};
static constexpr size_t kCount=sizeof(kCases)/sizeof(kCases[0]);
inline const Entry* find(const char* id){if(!id)return nullptr;for(size_t i=0;i<kCount;++i)if(strcmp(kCases[i].id,id)==0)return &kCases[i];return nullptr;}
inline bool apply(const char* id,uint8_t mac[6]){const Entry* e=find(id);if(!e||!mac)return false;memcpy(mac,e->prefix,3);return true;}
} // namespace SquachForkOuiLab
