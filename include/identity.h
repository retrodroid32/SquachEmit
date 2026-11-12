#pragma once
#include <stdint.h>
#include <stddef.h>

// Default identities are deterministic, locally administered (Wi-Fi) or
// static-random (BLE), with no modification of permanent eFuse addresses.
// Catalog indices 0..171 fit the nonzero final-octet scheme (max 253).
// Exception: 37 named OUI-only Wi-Fi cases in src/main.cpp replace their
// temporary SoftAP MAC's 24/28/36-bit prefix with a registered manufacturer
// allocation. An OUI match is only a spoofable vendor clue, NOT device proof.
namespace SquachIdentity {
enum class Radio : uint8_t { Ble, Wifi };
inline void derive(uint64_t efuse, size_t scenarioIndex, Radio radio, uint8_t out[6]) {
    out[0] = (radio == Radio::Ble) ? 0xC2 : 0x02;
    out[1] = (uint8_t)(efuse);
    out[2] = (uint8_t)(efuse >> 8);
    out[3] = (uint8_t)(efuse >> 16);
    out[4] = (uint8_t)(efuse >> 24);
    out[5] = (uint8_t)(scenarioIndex + 1);
}
inline void text(const uint8_t addr[6], char out[18]) {
    static const char kHexDigits[] = "0123456789ABCDEF";
    for (uint8_t i=0; i<6; ++i) {
        out[i*3]   = kHexDigits[addr[i] >> 4];
        out[i*3+1] = kHexDigits[addr[i] & 15];
        if (i != 5) out[i*3+2] = ':';
    }
    out[17] = '\0';
}
}
