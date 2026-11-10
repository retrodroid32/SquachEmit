#pragma once
#include <stdint.h>
#include <stddef.h>

// Stable, locally administered lab identities. No real vendor MACs,
// no modification of the chip's permanent eFuse address.
// scenarioIndex is the index in the shared catalog (0..48).
// The last octet makes each case distinct even when payloads/SSIDs match.
// Device-specific eFuse bytes separate identical cases on different boards.
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
    static const char HEX[] = "0123456789ABCDEF";
    for (uint8_t i=0; i<6; ++i) {
        out[i*3]   = HEX[addr[i] >> 4];
        out[i*3+1] = HEX[addr[i] & 15];
        if (i != 5) out[i*3+2] = ':';
    }
    out[17] = '\0';
}
}
