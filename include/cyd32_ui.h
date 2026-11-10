#pragma once
#include <stdint.h>
struct Scenario;
enum class Cyd32UiAction : uint8_t { None, Run, Stop, Suite };
// TFT_eSPI + on-board XPT2046, separate NVS touch calibration namespace.
// Does not access SquachWatch's own settings, except flashing a shared PCB
// will replace whichever application was installed.
void cyd32UiBegin();
Cyd32UiAction cyd32UiTick(const char* activeId, uint32_t remainingMs, bool suiteRunning);
const Scenario* cyd32UiSelection();
void cyd32UiResetCalibration();
void cyd32UiSetInversion(bool enabled);
bool cyd32UiInversion();
void cyd32UiColorBars();
