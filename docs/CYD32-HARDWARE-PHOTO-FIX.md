# CYD 3.2 — ST7798 hardware-photo correction build

This LAB revision is motivated by the owner's **actual photo** of the
3.2-inch ST7798 SquachEmit beside a working SquachWatch CYD, on
2026-11-10. The emitter showed a **white background and cyan/light-blue
Squachy** while SquachWatch showed a dark neon/saturated character.

## Corrections

- ST7798 polarity defaults to `invertDisplay(true)` on this LAB model.
  The previous build defaulted to false even though the actual photo
  demonstrated the wrong polarity. This intentionally uses the new
  NVS key `sqemit32/invert2` so an old saved `invert=false` does not
  make the bad default persist. Use `COLORS` and `INVERT ON/OFF`
  at 115200 baud to confirm physical batches. The setup remains BGR,
  40 MHz, GPIO 27 backlight, GPIO33 shared-bus XPT2046.
- The emitter UI now draws a **dark pixel-art synthwave skyline**,
  gradient title strip, readable purple-bordered speech bubble, Squachy
  with shaggy hair/sunglasses/pink cheeks/bright mouth, compact counters
  and three neon-outlined controls. Test category screens retain full
  width/settings-style rows. No white cards, giant pale belly or split
  poster layout. This is an independent TFT renderer inspired by actual
  SquachWatch 3.2 source, not a direct GPL code copy.
- Each emitted BLE/Wi-Fi scenario has stable separate synthetic MACs
  derived from board eFuse, scenario index and radio type. The
  `IDENTITY` serial command and TRUTH field help correlate cases.
  The BLE controller and SoftAP addresses are checked before a case
  is declared started; errors fail closed.

## Hardware gate

1. Flash **only** ESP32-2432S032R/CYD 3.2. This overwrites SquachWatch
   on that physical board, so keep a second CYD for detections.
2. After boot, the UI must be near-black with purple/cyan/pink accents
   and *brown* Squachy. If not, run `COLORS` to observe named pure
   R/G/B and use `INVERT ON` or `INVERT OFF` and reboot.
3. Compare to working SquachWatch screen for contrast, button positions,
   speech bubble, skyline, and recognizable sunglasses/pink cheeks.
4. Test TESTS -> categories -> case -> home; RUN, STOP and SUITE.
   Countdown must not repaint the entire screen.
5. With an independent SquachWatch 2.8 or 3.2 receiver, run:
   `RUN BLE-FLOCK-NAME 12`, `RUN BLE-FLOCK-MFG 12`,
   `RUN RID-DJI-MINI 12`, `RUN RID-DJI-AIR 12`.
   Confirm distinct MACs with `IDENTITY` or TRUTH and actual receiver
   logs. Repeat Wi-Fi fixtures `RUN WIFI-FLOCK-CH6 12` and
   `RUN RID-WIFI-BEACON 12`.
6. Confirm a 12-second test stops automatically, RF remains silent at
   boot, no stuck Wi-Fi, and no reset/crash loops.
7. No automatic merge or production tag until the owner reports
   hardware PASS. CI validates build only, not visual success.

## Note on architecture

SquachWatch is GPLv3; SquachEmit is MIT. This code uses independent
palette, geometry and original TFT primitives rather than importing
SquachWatch's GPL character assets/implementation. Exact animated
costumes and graphics would require separate licensing/integration.
