// SquachEmit ESP32-2432S032R / E32R32P 3.2" CYD.
// Wiring and TFT_eSPI 2.5.43 init path checked against the existing,
// hardware-used SquachWatch-CYD cyd32-st7798_user_setup.h.
// ST7798-branded module uses the ST7789-compatible commands.
// XPT2046 shares the LCD's HSPI bus; touch CS=GPIO33.
#pragma once
#define USER_SETUP_INFO "SquachEmit / CYD 3.2 ST7798 / ESP32-2432S032R"
#define ST7789_DRIVER
#define TFT_WIDTH 240
#define TFT_HEIGHT 320
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST -1
#define TFT_BL 27
#define TOUCH_CS 33
#define TFT_BACKLIGHT_ON HIGH
#define USE_HSPI_PORT
#define SPI_FREQUENCY 40000000
#define SPI_READ_FREQUENCY 20000000
#define SPI_TOUCH_FREQUENCY 2500000
#define TFT_RGB_ORDER TFT_BGR
#define TFT_INVERSION_OFF
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
