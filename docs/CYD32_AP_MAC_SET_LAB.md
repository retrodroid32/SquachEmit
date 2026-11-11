# CYD 3.2 AP_MAC_SET failure — LAB correction

Hardware report: selecting certain synthetic Wi-Fi DJI Remote ID cases,
then tapping RUN, resulted in "WIFI START FAILED / AP_MAC_SET".

## Cause

The earlier code invoked WiFi.mode(WIFI_STA), which starts the ESP32
Wi-Fi driver in station-only mode. It then called esp_wifi_set_mac on
WIFI_IF_AP before selecting AP mode. That AP MAC operation fails with
the wrong mode/interface. Retrying the same invalid call ordering cannot
correct this problem.

ESP-IDF also requires the target AP interface to be disabled when
changing its MAC address. This branch now:

1. Initializes Wi-Fi using Arduino STA mode (no SoftAP beacon yet).
2. Stops the ESP-IDF Wi-Fi driver.
3. Configures AP mode while the driver is stopped.
4. Sets the unique locally administered synthetic AP MAC.
5. Restarts the driver (including when a previous step failed, to keep
   Arduino's internal Wi-Fi started-state consistent).
6. Reads the AP MAC back and verifies it matches the intended address.
7. Starts the synthetic 2.4 GHz SoftAP + five-message Remote ID test
   using the existing bounded transmission duration.

The stable diagnostic steps include AP_STOP_FOR_MAC, AP_MODE, AP_MAC_SET,
AP_RESTART and AP_MAC_READBACK. A successful initialization prints
RADIO,AP_MAC_READY,<case-id>,<mac> before TRUTH BEGIN.

The new host regression uses an injectable fake driver to verify
the exact stop / configure AP / set MAC / restart / readback order and
failure recovery. Host CI cannot prove the physical board transmitted.

## Physical verification (CYD 3.2 ST7798)

Install only from https://retrodroid32.github.io/SquachEmit/?lab=1.
Choose this branch's LAB firmware. The Web Installer's Serial Test
Console should be set to 115200 baud.

1. Select the synthetic DJI Mavic 2 Pro Wi-Fi profile on-screen and RUN.
2. Watch for STARTING WIFI TRANSMISSION and RADIO,AP_MAC_READY.
3. Expect TRUTH BEGIN only when the driver accepts the full startup.
4. Confirm on a second SquachWatch receiver that the synthetic Remote ID
   beacon is visible.
5. Try Phantom 4 Pro V2.0, Mini 3 Pro and Mavic 3 Wi-Fi cases.
6. Alternate BLE and Wi-Fi cases repeatedly, then STOP and verify no
   lingering network or overlapping text.
7. If failure continues, record ERROR,WIFI_STAGE, ERROR,WIFI_CASE
   (and the numeric ESP error code).

This LAB transmits only artificial SQE-DJI identifiers, not genuine
DJI serials, DJI proprietary DroneID or OcuSync signals. No merge,
release/tag, or physical hardware pass is implied.
