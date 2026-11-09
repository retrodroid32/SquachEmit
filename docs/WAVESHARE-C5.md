# Waveshare ESP32-C5-Touch-LCD-2.8 — SquachEmit experimental board

This hardware target is **NOT YET BOARD-TESTED**. The supplied Amazon ASIN
B0H8SMQYZT appears consistent with Waveshare's capacitive-touch product family,
but the Amazon listing itself could not be directly verified. **Before flashing,
confirm the PCB is labelled Waveshare ESP32-C5 Touch LCD 2.8**, not an LCDWiki,
RockBase, NiceMCU, or ZLX XPT2046 resistive-touch version.

## Wiring verified against manufacturer documentation
- ST7789 240 x 320 LCD: GPIO6 SCK, GPIO7 MOSI, GPIO9 DC, GPIO10 CS
- CST3530 capacitive touch: I2C address 0x58, SDA GPIO0, SCL GPIO1, INT GPIO5
- CH32V003 IO expander: I2C address 0x24; EXIO1 LCD reset,
  EXIO0 touch reset, backlight via CH32 PWM
- MicroSD: GPIO6/7/8 (SPI clock/MOSI/MISO), GPIO23 CS — not used
- USB native: D- GPIO13, D+ GPIO14; UART0 TX11/RX12
- Vendor examples use a 16MB flash partition layout. The module may
  physically contain more flash; this conservative build uses 16MB.

Sources:
- https://docs.waveshare.com/ESP32-C5-Touch-LCD-2.8
- https://github.com/waveshareteam/ESP32-C5-Touch-LCD-2.8
- https://github.com/platformio/platform-espressif32/releases/tag/v8.0.0

## Build and upload
PlatformIO 8.x ships Arduino-ESP32 3.3.12 (compatible with the vendor's
documented version). A **custom board manifest** is checked into `boards/`.

```powershell
pio run -e waveshare-esp32c5-touch-28
pio run -e waveshare-esp32c5-touch-28 -t upload
pio device monitor -b 115200
```

For a new board, upload with the project (bootloader + partitions + app)
rather than flashing a lone application binary at an arbitrary offset.

## Operation
- No BLE/Wi-Fi transmissions are started on boot.
- Tap a scenario row, then **RUN** for the default 12-second test.
- **PREV/NEXT** navigate among all test cases.
- **STOP** cancels and stops the current RF test.
- **SUITE** runs the complete bounded sequence with quiet gaps.
- USB serial commands from the S3 target are unchanged (115200 baud).
- The screen requires PSRAM for the 240x320 16-bit framebuffer. If PSRAM
  allocation fails, serial mode remains available.

## Physical validation — mandatory before release
1. Confirm actual PCB model and the presence of CH32V003 + CST3530.
2. Boot with USB, confirm boot banner, backlight, LCD and touch coordinates.
3. Run BLE-META-UUID, BLE-DRONE and WIFI-AXON; independently inspect both
   SquachWatch 2.8-inch and 3.2-inch detections.
4. Press STOP during each mode and verify RF emission stops.
5. Validate BLE-NEG-APPLE and WIFI-NEG produce no target alert.
6. Validate page navigation, SUITE quiet gaps, 30-second timeout, reboot.
7. Do not label RF cases verified until actual detector observations exist.

## Scope and limits
The C5 adds 5GHz radio capability and 802.15.4 silicon but this phase
**does not** provide 5GHz or Zigbee/Thread test scenarios. It emits ordinary
2.4GHz Wi-Fi SoftAP and bounded BLE adverts. No Bluetooth Classic, arbitrary
802.11 frame injection, deauth flooding, or jamming support.
The radio remains timeshared. Synthetic RF signals do not certify that
a real camera, tracker, bodycam or drone would be detected.
