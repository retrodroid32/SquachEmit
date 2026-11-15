// SquachEmit → SquachWatch Theme setting adapter.
// The upstream renderer stays unchanged; only visual scene selection is
// supplied. This firmware does not initialize SquachWatch's receiver.
#if defined(SQE_CYD32) && defined(SQE_ORIGINAL_SQUACHY)
#include "settings.h"
#include "sqemit_visual_bridge.h"
#include "clock.h"
#include "security.h"
namespace SquachWatchVisual {
void setBackground(uint8_t index){
  // Use SquachWatch's actual Settings implementation, never redefine its
  // background accessor or shadow its original scene preferences.
  const uint8_t upstream=index<9?index:10;
  Settings::previewBackground(static_cast<Settings::Background>(upstream));
}
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
