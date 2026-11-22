# R14.27 CYD 3.2 battery and charging-state notes

## Board facts

On the E32R32P/ST7798 variant the published schematic shows a TP4054
charger (U6), a battery-voltage divider (100k/100k), and BAT_ADC routed to
the ESP32 GPIO34. It does **not** show USB VBUS or the TP4054 CHRG output
routed to an ESP32 input. See:
https://www.lcdwiki.com/res/E32R32P/3.2inch_ESP32-32_Display_Schematic.pdf

As a result, firmware can estimate battery voltage, but **cannot
automatically know** whether USB is plugged in, whether current is
actually flowing into the cell, or whether charging has completed.
A battery reading of 98% alone is not a charging signal.

## Display states (R14.27)

- A **lightning bolt** only when a physically verified charger-status
  input has been provided through the optional `SQE_BAT_CHARGE_PIN` build
  flag and reports active-low (or a configured polarity).
- A small **+** in the battery icon: measured battery voltage rose over
  recent samples. This is an estimate, **not proof of charging**.
- A small **?**: charger state **unknown**, including when battery is 98%
  or 100% but the ESP32 has no verified CHRG/VBUS connection.

Voltage samples are approximately 15 seconds apart. The 8-sample trend
uses a smaller near-full threshold, but charging may still cause no visible
rise when almost fully charged. False positives from load changes or ADC
noise remain possible, hence no automatic lightning bolt based on voltage.

Serial diagnostic lines start with `CYD32,BATTERY`; check `mV`,
`pct`, `trend_rising`, `charge_sensor`, and `charge_state`.

## True charging detection requires hardware

For a board modification, a qualified person must first inspect the
exact assembled PCB and confirm the available TP4054 CHRG output and an
unused ESP32 input. CHRG is an open-drain, active-low output and must use
an appropriate **3.3V pull-up** (e.g. 10k), *never connect 5V to a GPIO*.
GPIO34–GPIO39 are input-only and have **no internal pull-ups**, so they
need an external pull-up if used as charger-state inputs. Only then
configure an unused verified input with
`-DSQE_BAT_CHARGE_PIN=<gpio>` (plus `SQE_BAT_CHARGE_ACTIVE_LOW`
if polarity differs). Do not assume any particular accessible pin is
safe to use without tracing that physical board revision.

Do not assert a verified charge state from the USB serial host, Wi-Fi,
voltage trend, or battery percentage alone.
