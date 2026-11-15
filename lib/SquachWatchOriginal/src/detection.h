// SquachEmit visual-only adapter for SquachWatch Theme and Squachy.
// Preserve original Detection and DetectionType layouts from state.h.
// Deliberately provide ZERO receiver observations: this project transmits
// synthetic signals and must not run SquachWatch's sniffing radio engine.
#pragma once
#include "state.h"
class DetectionEngine {
public:
  uint8_t logCount() const {return 0;}
  const Detection* logAt(uint8_t) const {return nullptr;}
  uint8_t channelActivity(uint8_t) const {return 0;}
};
