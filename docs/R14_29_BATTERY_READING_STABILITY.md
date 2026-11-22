# SquachEmit CYD 3.2 R14.29 — Battery voltage stability

## What changed

The charging-status indicator (question mark, plus and lightning bolt)
has been **removed** from the status bar. R14.29 displays only a battery
icon and an approximate voltage-derived charge percentage.

The E32R32P schematic uses two 100 kΩ resistors to measure the battery
through GPIO34, presenting roughly 50 kΩ source impedance to the ADC.
The official LCDWiki schematic and user manual show that **C1 and C2
(0.1 µF each) are already fitted** to the battery-voltage sensing circuit.
Adding another capacitor is **not** the first recommendation; verify the
hardware reading against a meter before considering any component changes.

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
coulomb-counting fuel gauge. **The board already has the manufacturer's
specified ADC bypass capacitor** (C1, 0.1 µF), so no extra capacitor
should be assumed necessary. The official manual specifies only battery
voltage (BAT_ADC × 2), **not a percent lookup**. Furthermore, when USB-C
is connected, Q3 disconnects the battery from the device power supply;
BAT_ADC then measures terminal voltage during charging, not resting SOC.
Starting in **R14.30** the header displays voltage (e.g. 4.05V), not
an uncalibrated percentage. Use a multimeter comparison to verify voltage
accuracy before considering board modifications. True percentage reporting
requires the particular cell's characterized resting-voltage curve or a
fuel-gauge IC.

References:
- https://www.lcdwiki.com/res/E32R32P/3.2inch_ESP32-32_Display_Schematic.pdf
- https://www.lcdwiki.com/res/E32R32P/3.2inch_ESP32-32E_E32R32P_E32N32P_User_Manual.pdf
- https://docs.espressif.com/projects/esp-hardware-design-guidelines/en/latest/esp32/schematic-checklist.html
