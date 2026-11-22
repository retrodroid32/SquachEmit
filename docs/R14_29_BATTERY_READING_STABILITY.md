# SquachEmit CYD 3.2 R14.29 — Battery voltage stability

## What changed

The charging-status indicator (question mark, plus and lightning bolt)
has been **removed** from the status bar. R14.29 displays only a battery
icon and an approximate voltage-derived charge percentage.

The E32R32P schematic uses two 100 kΩ resistors to measure the battery
through GPIO34, presenting roughly 50 kΩ source impedance to the ADC.
The ESP32's ADC is sensitive to electrical noise and source impedance;
Espressif recommends a 0.1 µF capacitor at the ADC input for accuracy.

The firmware waits 3 seconds after starting, rejects bad/noisy
individual samples, and requires **four separate 32-sample voltage
windows** that agree within 60 mV on the battery side before showing a
percentage. The first windows are spaced approximately 3 seconds apart;
subsequent stable windows use the prior ~15-second cadence. The voltage
shown in serial diagnostics is smoothed using four-window medians.
Disagreeing windows now show an unknown value instead of a confidently
wrong battery percentage.

The display shows approximate 5% steps (for example `~90%`) with
hysteresis, **not** a calibrated battery state-of-charge measurement.
The value may still differ after restarts if the actual battery
terminal voltage changes because of load, charging, temperature, cell
chemistry, or ADC calibration.

## Validation procedure

1. Use a consistent USB and battery connection (do not compare one
   USB-powered boot to a battery-only boot).
2. After a restart, expect `--` while four stable readings accumulate.
3. Compare readings after the settling period across three or more
   restarts under the **same electrical conditions**.
4. At 115200 baud, capture `CYD32,BATTERY` serial diagnostics:
   `raw_mV`, `filtered_mV`, `spread_mV`,
   `core_spread_mV`, `pct_est`, `valid`,
   `consensus_windows`, and `unstable_windows`.
5. If readings still disagree by over ~60 mV across windows, firmware
   intentionally reports an unknown percentage. Do not hide that state.

## Hardware limitation

Software cannot turn this board's battery divider and ESP32 ADC into a
coulomb-counting fuel gauge. If accuracy is still poor, a qualified
technician may examine whether a small 0.1 µF ceramic capacitor from
**BAT_ADC (GPIO34) to GND** is suitable on the exact assembled board.
Disconnect USB and the battery before any soldering. A calibrated
external battery fuel-gauge IC is the preferred approach for a true
state-of-charge percentage; otherwise use a multimeter comparison to
calibrate the voltage reading.

References:
- https://www.lcdwiki.com/res/E32R32P/3.2inch_ESP32-32_Display_Schematic.pdf
- https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html
