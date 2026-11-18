// SquachEmit two-band animation clock correction.
// Kept in the SquachEmit fork of the original SquachWatch scene library;
// it does not modify SquachWatch firmware.
#pragma once
#include <stdint.h>

namespace SqEmitSceneClock {
// Each logical LCD frame is rendered twice with the SAME timestamp.
// An unchanged timestamp is *zero elapsed motion*, not a fallback 16ms
// tick. The original 16ms seed is used only on the first-ever frame.
// Unsigned subtraction also handles ordinary millis() wraparound.
inline uint32_t elapsedOnce(uint32_t now, uint32_t& previous,
                            uint32_t maxStepMs = 100u,
                            uint32_t initialStepMs = 16u) {
    if (now == previous) return 0u;
    const uint32_t dt = previous ? now - previous : initialStepMs;
    previous = now;
    return dt > maxStepMs ? maxStepMs : dt;
}
}  // namespace SqEmitSceneClock
