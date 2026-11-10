#pragma once
// GPL-3.0-only character module adapted from SquachWatch-CYD.
#if defined(SQE_CYD32)
#include <stdint.h>
class TFT_eSprite;
namespace SquachyPort { void render(TFT_eSprite& t,uint32_t now,bool onAir,bool suite,bool pet); }
#endif
