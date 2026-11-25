// SquachWatch v1.50.0 source-pinned 24-bit Wi-Fi OUI test fixtures.
// Synthetic temporary AP MACs; an OUI proves NEITHER an actual manufacturer
// nor a camera, drone, tracking device or active surveillance.
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>
namespace SquachV150OuiLab {
struct Entry {const char* id; uint8_t prefix[3];};
static const Entry kCases[]={
  {"V150-OUI-ALPR-00047D",{0x00,0x04,0x7D}},
  {"V150-OUI-ALPR-001885",{0x00,0x18,0x85}},
  {"V150-OUI-ALPR-00BF15",{0x00,0xBF,0x15}},
  {"V150-OUI-CAMERA-002542",{0x00,0x25,0x42}},
  {"V150-OUI-CAMERA-4427F3",{0x44,0x27,0xF3}},
  {"V150-OUI-CAMERA-001F4F",{0x00,0x1F,0x4F}},
  {"V150-OUI-CAMERA-F0C88B",{0xF0,0xC8,0x8B}},
  {"V150-OUI-CAMERA-80482C",{0x80,0x48,0x2C}},
  {"V150-OUI-CAMERA-F85128",{0xF8,0x51,0x28}},
  {"V150-OUI-CAMERA-FCDB21",{0xFC,0xDB,0x21}},
  {"V150-OUI-CAMERA-0016B2",{0x00,0x16,0xB2}},
  {"V150-OUI-RING-AC9FC3",{0xAC,0x9F,0xC3}},
  {"V150-OUI-RING-187F88",{0x18,0x7F,0x88}},
  {"V150-OUI-RING-FC65DE",{0xFC,0x65,0xDE}},
  {"V150-OUI-RING-242BD6",{0x24,0x2B,0xD6}},
  {"V150-OUI-LISTENER-000E58",{0x00,0x0E,0x58}},
  {"V150-OUI-LISTENER-347E5C",{0x34,0x7E,0x5C}},
  {"V150-OUI-LISTENER-38420B",{0x38,0x42,0x0B}},
  {"V150-OUI-META-28E888",{0x28,0xE8,0x88}},
  {"V150-OUI-META-98DA92",{0x98,0xDA,0x92}},
  {"V150-OUI-FLOCK-B41E52",{0xB4,0x1E,0x52}},
  {"V150-OUI-AXON-0025DF",{0x00,0x25,0xDF}},
};
static constexpr size_t kCount=sizeof(kCases)/sizeof(kCases[0]);
inline const Entry* find(const char* id){
  if(!id)return nullptr;
  for(size_t i=0;i<kCount;++i)
    if(strcmp(kCases[i].id,id)==0)return &kCases[i];
  return nullptr;
}
inline bool apply(const char* id,uint8_t mac[6]){
  const Entry* e=find(id);
  if(!e||!mac)return false;
  memcpy(mac,e->prefix,3); // per-board/per-case synthetic suffix retained
  return true;
}
} // namespace SquachV150OuiLab
