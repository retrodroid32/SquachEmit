// SquachEmit → SquachWatch Theme setting adapter.
// The upstream renderer stays unchanged; only visual scene selection is
// supplied. This firmware does not initialize SquachWatch's receiver.
#if defined(SQE_CYD32) && defined(SQE_ORIGINAL_SQUACHY)
#include "settings.h"
#include "sqemit_visual_bridge.h"
#include "clock.h"
#include "security.h"
namespace {
  uint8_t chosenBackground=10; // SquachWatch enum SYNTHWAVE
}
namespace SquachWatchVisual {
void setBackground(uint8_t index){
  chosenBackground=index<9?index:10;
}
}
namespace Settings {
Background background(){return static_cast<Background>(chosenBackground);}
}
// SquachEmit has no SquachWatch GPS-backed trusted Clock or PIN lock.
// Explicitly report untrusted time; the original Squachy animation then
// skips time-sensitive greetings rather than inventing a clock.
namespace Clock {
bool trusted(){return false;}
uint8_t weekday(){return 0;}
bool weekend(){return false;}
uint8_t hour(){return 0;}
}
namespace Security {bool enabled(){return false;}}
#endif
