// Arduino-ESP32 3.3.12 entry point for the experimental Waveshare C5 build.
// Arduino CLI automatically compiles src/main.cpp, src/scenarios.cpp and
// src/c5_ui.cpp. PlatformIO ignores this root sketch for the S3 build.
#include <Arduino.h>
#if !defined(SQE_WAVESHARE_C5)
#error "Compile this sketch with -DSQE_WAVESHARE_C5=1 for the Waveshare ESP32-C5."
#endif
#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>
#include <Board_IO.h>
