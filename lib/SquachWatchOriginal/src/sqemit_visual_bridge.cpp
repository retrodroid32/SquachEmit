// SquachEmit → SquachWatch Theme setting adapter.
// The upstream renderer stays unchanged; only visual scene selection is
// supplied. This firmware does not initialize SquachWatch's receiver.
#if defined(SQE_CYD32) && defined(SQE_ORIGINAL_SQUACHY)
#include "settings.h"
#include "sqemit_visual_bridge.h"
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
#endif
