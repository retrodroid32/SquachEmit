#pragma once
#include <stdint.h>
struct Scenario;
enum class C5UiAction : uint8_t { None, Run, Stop, Suite };
void c5UiBegin();
C5UiAction c5UiTick(const char* activeId, uint32_t remainingMs, bool suiteRunning);
const Scenario* c5UiSelection();
