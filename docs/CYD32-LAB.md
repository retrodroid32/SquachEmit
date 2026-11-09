
# SquachEmit CYD 3.2 — ESP32-2432S032R ST7798 LAB

This is a third SquachEmit transmitter target, independent of Waveshare C5
and the ESP32-S3. It uses the tested SquachWatch-CYD 3.2-inch pin mapping.
It is NOT a SquachWatch upgrade or a production release.

Hardware: ESP32-2432S032R / E32R32P, ESP32-WROOM-32, 4MB flash,
240x320 ST7798/ST7789-compatible TFT with resistive XPT2046.

| Bus | Pins |
|---|---|
| HSPI shared TFT/touch | MISO12, MOSI13, SCLK14 |
| TFT | CS15, DC2, reset -1, GPIO27 backlight |
| XPT2046 | TOUCH_CS33, SPI touch frequency 2.5 MHz |
| TFT configuration | TFT_eSPI ST7789, BGR, inversion off, 40 MHz |
| RF | 2.4 GHz Wi-Fi + BLE4 legacy only |
| Serial console | 115200 baud (SquachWatch uses 2,000,000) |
| Flash | 4MB, bootloader offset 0x1000, application offset 0x10000 |

## Touch and UI

First boot offers TAP SCREEN TO START, then interactive five-point TFT_eSPI
touch calibration. Calibration persists under the SquachEmit-only NVS
namespace 'sqemit32', key 'touch', leaving SquachWatch app namespaces alone.
To reset alignment, connect at 115200 baud and send CALTOUCH; tap the panel.

Menu pages show 49 on-air test cases with PREV/NEXT, RUN, STOP and SUITE.
No large framebuffer/PSRAM is required. RF stays off on boot.

## WARNING: this replaces your SquachWatch application

Flashing SquachEmit overwrites the SquachWatch firmware on that 3.2-inch
CYD. One board cannot emit and scan with both applications simultaneously.
Use another physical board (such as your 2.8-inch CYD) as the receiver.
Backup existing settings/firmware; erasing flash clears saved settings.

## Required hardware validation before merge

1. Confirm physical PCB ESP32-2432S032R, NOT C5.
2. Flash only the CYD32 LAB installer entry, chipFamily ESP32.
3. Confirm boot, TFT colors/backlight, stable redraw and 49 entries.
4. Calibrate touch, test scrolling, RUN/STOP and SUITE.
5. Confirm reboot retains touch fit, CALTOUCH recovers calibration.
6. Run RID-DJI-MAVIC, RID-WIFI-BEACON, RID-NEG-APP and STOP
   with an independent SquachWatch board observing RF.
7. Confirm timed stop, silent boot, no resets and no receiver anomalies.
8. Record hardware results; compiled firmware is not hardware verified.
