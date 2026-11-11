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
// User-selected 3..30 second duration, persisted in CYD32 NVS.
uint32_t cyd32UiDurationMs();
void cyd32UiResetCalibration();
void cyd32UiSetInversion(bool enabled);
bool cyd32UiInversion();
void cyd32UiColorBars();

// Show/clear last radio startup stage on the CYD 3.2 home screen.
void cyd32UiSetRadioError(const char* stage);

// Acknowledge a Wi-Fi RUN touch before the radio driver blocks.
void cyd32UiSetRadioStarting(const char* scenarioId);
