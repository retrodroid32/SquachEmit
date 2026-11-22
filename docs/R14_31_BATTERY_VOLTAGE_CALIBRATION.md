# SquachEmit R14.31 — Battery voltage calibration

For the CYD 3.2 E32R32P board only.

## User-defined voltage scale

The display intentionally uses this **linear voltage scale**, supplied
by the owner, rather than claiming a manufacturer's lithium-ion fuel-gauge
curve:

- **3.20 V or below = 0%**
- **3.70 V = 50%**
- **4.00 V = 80%**
- **4.20 V or above = 100%**

Inside the 3.20–4.20 V range, `percent = (V - 3.20) * 100` (with
V in volts). The header shows the result rounded to 5% steps (`~80%`)
to avoid implying more accuracy than the ESP32 ADC / voltage sensing
circuit can offer. The icon fill uses the same calibrated percentage.
This voltage-based estimate is **not true battery state-of-charge**,
especially while USB charging or under RF load.

## On-device calibration

Open **Main Menu → SETTINGS → BATTERY** (bottom-right button of the
first Settings screen), then:

1. Allow the measured value to settle; the screen initially says
   **ACQUIRING** until four ADC sampling windows agree.
2. Measure the battery terminal voltage with a multimeter, using a
   battery in a safe operating range and the same power condition.
3. Compare the screen's **RAW** battery voltage with the multimeter.
4. Tap **+10 mV / -10 mV** for fine changes or **+50 mV / -50 mV**
   for faster changes. Match the screen's **ADJUSTED** voltage to
   the meter. The difference is the additive battery-side offset.
5. Tap **DONE** or **BACK**. The adjustment saves immediately in
   ESP32 Preferences/NVS, and is restored across restarts.
6. Tap **RESET** on this screen to save an offset of exactly 0 mV.

The adjustment is bounded to **-500...+500 mV**, added to the already
reconstructed battery-side voltage (`2 × BAT_ADC`). Calibration does
not change the ADC attenuation, battery power circuitry, RF generator,
or animation timing.

Example: if the display shows **3.95 V RAW** but the multimeter reads
**4.05 V**, press **+50 mV** twice to set an offset of +100 mV. The
corrected display will read **4.05 V**, corresponding to **~85%** using
the chosen linear scale.

## Diagnostics

Serial output (`CYD32,BATTERY`) includes `raw_mV`, `filtered_mV`,
`offset_mV`, `corrected_mV`, `linear_pct`, sample spread,
and consensus status. The adjusted voltage is presented separately
from the original sampled voltage; an offset never overwrites the raw
measurement.

Manufacturer reference:
https://www.lcdwiki.com/res/E32R32P/3.2inch_ESP32-32E_E32R32P_E32N32P_User_Manual.pdf

## R14.32 — Main-screen battery display mode

In **Settings → Battery**, use the right-hand footer button to choose
**SHOW %** or **SHOW V**. The button displays the *currently selected*
mode; tap it to switch.

- **SHOW %** displays a battery icon and approximate percentage, for
  example `~85%`, computed from the calibrated battery voltage and the
  user-defined **3.20 V = 0% / 4.20 V = 100%** linear scale.
- **SHOW V** displays the same battery icon and corrected battery voltage,
  for example `4.05V` (rounded to 50 mV).

The selection applies to the main-screen header in **both portrait and
landscape**, and is saved immediately in device Preferences/NVS under
`batshow_v`, independently of the `batoff_mv` calibration offset.
The selection survives a reboot. The battery bar still follows the
calibrated 3.20–4.20 V voltage scale in either mode.

The existing calibration controls (±10 mV, ±50 mV and RESET) remain
unchanged. On portrait, the center footer remains STOP while transmitting.
No charging indicator is reintroduced.
