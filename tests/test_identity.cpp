#include "../include/identity.h"
#include <assert.h>
#include <stdio.h>
#include <set>
#include <string>

static std::string mac(uint64_t chip, size_t index, SquachIdentity::Radio type) {
    uint8_t bytes[6];
    char output[18];
    SquachIdentity::derive(chip,index,type,bytes);
    SquachIdentity::text(bytes,output);
    if (type == SquachIdentity::Radio::Wifi) {
        assert(bytes[0] == 0x02);
        assert((bytes[0] & 1) == 0); // unicast
        assert((bytes[0] & 2) == 2); // locally administered
    } else {
        assert((bytes[0] & 0xC0) == 0xC0); // BLE static random
    }
    return output;
}
int main() {
    const uint64_t boards[]={0x102030405060ULL,0x102030415061ULL,
                             0xABCDEF125566ULL};
    std::set<std::string> unique;
    for (auto chip:boards) {
        for (size_t i=0;i<49;++i) {
            const auto ble=mac(chip,i,SquachIdentity::Radio::Ble);
            const auto wifi=mac(chip,i,SquachIdentity::Radio::Wifi);
            assert(ble==mac(chip,i,SquachIdentity::Radio::Ble));
            assert(wifi==mac(chip,i,SquachIdentity::Radio::Wifi));
            assert(ble!=wifi);
            assert(unique.insert(ble).second);
            assert(unique.insert(wifi).second);
        }
    }
    assert(unique.size()==3u*49u*2u);
    puts("Static BLE and LAA WiFi MACs: unique per board, scenario, radio; stable on rerun");
}
