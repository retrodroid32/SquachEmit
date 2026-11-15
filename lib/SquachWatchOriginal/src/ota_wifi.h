// SquachEmit LAB: visual-only adapter for SquachWatch Theme's top-bar Wi-Fi
// glyph. SquachEmit is a signal transmitter; its original Theme renderer
// must NOT bring in SquachWatch OTA, HTTP, network passwords or radio tasks.
// All drawing routines in original theme.cpp are otherwise untouched.
#pragma once
#include <stdint.h>
namespace OtaWifi {
enum class SavedResult : uint8_t { UNTRIED=0, JOINED, BAD_PASSWORD, NOT_FOUND, FAILED };
enum class SavedTest : uint8_t { IDLE=0, CONNECTING, JOINED, BAD_PASSWORD, NOT_FOUND, FAILED };
inline uint8_t savedCount(){return 0;}
inline uint8_t savedUse(){return 0;}
inline SavedResult savedResult(uint8_t){return SavedResult::UNTRIED;}
inline SavedTest savedTestState(){return SavedTest::IDLE;}
} // namespace OtaWifi
