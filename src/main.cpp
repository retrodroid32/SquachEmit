#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <NimBLEDevice.h>
#include "scenarios.h"
#if defined(SQE_WAVESHARE_C5)
#include "c5_ui.h"
#elif defined(SQE_CYD32)
#include "cyd32_ui.h"
#endif
#include <string.h>

// SquachEmit v0.3.0 LAB: bounded, explicit lab emissions; no unattended RF flood.
// In every test the expected label is a *detector classification*, not a
// claim the actual product named by that classification is present.
static constexpr uint32_t kDefaultMs = 12000;
static constexpr uint32_t kMaxMs = 30000;
static constexpr uint32_t kMinMs = 3000;
static constexpr uint32_t kRemoteRotationMs = 1300;
static constexpr uint32_t kSuiteGapMs = 2400;

static NimBLEAdvertising* g_adv = nullptr;
static bool g_wifiVendorIE = false;
static bool g_bleInit = false;
static const Scenario* g_active = nullptr;
static uint32_t g_started = 0;
static uint32_t g_duration = 0;
static uint32_t g_lastRotation = 0;
static uint8_t g_remotePart = 0;
static uint32_t g_caseNumber = 0;
static bool g_suite = false;
static size_t g_suiteNext = 0;
static uint32_t g_suiteAt = 0;
static String g_line;

static void truth(const char* event, const Scenario* s, const char* status = "ok") {
  // Existing keys remain stable for the serial truth consumer. 0 = BLE.
  const bool wifi = s && (s->kind == Kind::WifiAp || s->kind == Kind::WifiRemoteId);
  const unsigned channel = wifi ? (s->code ? (unsigned)s->code : 1u) : 0u;
  Serial.printf("TRUTH,{\"event\":\"%s\",\"seq\":%lu,\"ms\":%lu,\"id\":\"%s\",\"expected\":\"%s\",\"status\":\"%s\",\"carrier\":\"%s\",\"channel\":%u}\n",
                event, (unsigned long)g_caseNumber, (unsigned long)millis(),
                s ? s->id : "", s ? s->expected : "NONE", status,
                wifi ? "WIFI" : "BLE", channel);
}

static bool beginBle() {
  if (g_bleInit) return true;
  if (!NimBLEDevice::init("SquachEmit-Lab")) return false;
  g_adv = NimBLEDevice::getAdvertising();
  if (!g_adv) return false;
  g_adv->setMinInterval(160); // 100 ms
  g_adv->setMaxInterval(240); // 150 ms
  g_bleInit = true;
  return true;
}

static void put32(uint8_t* p, int32_t v) {
  const uint32_t u = (uint32_t)v;
  for (uint8_t j = 0; j < 4; ++j) p[j] = (uint8_t)(u >> (j * 8));
}

// Exact 25-byte message layout mirrored by SquachWatch remote_id.cpp and
// test/emitter_roundtrip_test.cpp; all coordinates/IDs are synthetic.
static bool scenarioIs(const Scenario* s, const char* id) {
  return s && strcmp(s->id, id) == 0;
}

// Synthetic ASTM F3411 test geometry, matched to the SquachWatch legacy
// decoder. Each model profile has a distinctly FAKE serial and a Self-ID
// description. This does not clone actual DJI/Autel/Parrot hardware or RF.
static void remoteFrame(uint8_t which, uint8_t* body27, const Scenario* s) {
  memset(body27, 0, 27);
  body27[0] = 0x0D; // OpenDroneID application code
  body27[1] = which + 1;
  uint8_t* m = body27 + 2;
  switch (which) {
    case 0: {
      m[0] = 0x02; m[1] = scenarioIs(s, "RID-BAD-SERIAL") ? 0xF2 : 0x12;
      // Intentionally invalid IDType F is still transport-recognizable.
      const char* serial = (s && s->kind == Kind::BleRemoteId && s->text)
        ? s->text : "SQUACHEMIT-TEST-0001";
      strncpy(reinterpret_cast<char*>(m + 2), serial, 20);
      break;
    }
    case 1:
      m[0] = 0x12;
      put32(m + 5, 407128000);  // fixed synthetic test aircraft coordinate
      put32(m + 9, -740060000);
      m[15] = 0xC0; m[16] = 0x08; // 120 m, encoded as 2240
      // Location TimeStamp: tenths of seconds after the UTC hour.
      // There is no RTC/GPS in the emitter: use "unknown" except in the
      // explicitly stale fixture (0.1 seconds after the top of an hour).
      if (!scenarioIs(s, "BLE-DRONE")) {
        m[21] = scenarioIs(s, "RID-STALE-LOC") ? 1 : 0xFF;
        m[22] = scenarioIs(s, "RID-STALE-LOC") ? 0 : 0xFF;
      }
      break;
    case 2:
      m[0] = 0x42;
      put32(m + 2, 407580000);  // synthetic operator differs from aircraft
      put32(m + 6, -739855000);
      break;
    case 3: {
      m[0] = 0x32; // Self-ID message, protocol version 2
      m[1] = 0;    // text description
      const char* label = s ? s->id : "GENERIC";
      char description[24] = {0};
      snprintf(description, sizeof(description), "LAB %s", label);
      memcpy(m + 2, description, strnlen(description, 23));
      break;
    }
    default: break;
  }
}

static bool advertiseRemote(uint8_t part, const Scenario* s) {
  if (!g_adv || !g_bleInit) return false;
  g_adv->stop();
  uint8_t body[27];
  remoteFrame(part, body, s);
  NimBLEAdvertisementData data;
  // Do not add flags or a name: the legacy service data fills all 31 bytes.
  if (!data.setServiceData(NimBLEUUID((uint16_t)0xFFFA), body, sizeof(body)))
    return false;
  if (data.getPayload().size() != 31u) return false;
  if (!g_adv->setAdvertisementData(data)) return false;
  return g_adv->start();
}

static bool advertiseRemoteNegative(const Scenario* s) {
  uint8_t body[27];
  remoteFrame(0, body, s);
  const bool wrongApp = scenarioIs(s, "RID-NEG-APP");
  if (wrongApp) body[0] = 0x0E;
  const size_t length = wrongApp ? sizeof(body) : sizeof(body) - 1u;
  NimBLEAdvertisementData data;
  if (!data.setServiceData(NimBLEUUID((uint16_t)0xFFFA), body, length))
    return false;
  if (!g_adv->setAdvertisementData(data)) return false;
  return g_adv->start();
}

// Standard esp_wifi_set_vendor_ie inserts a self-contained vendor IE into
// ordinary SoftAP beacons. This is not arbitrary management-frame injection.
// Byte-for-byte follows SquachWatch's RemoteId::isWifiBeacon test fixture.
static bool startWifiRemote(const Scenario* s) {
  const uint8_t channel = s->code ? static_cast<uint8_t>(s->code) : 6;
  if (channel < 1 || channel > 11) return false;
  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(s->text ? s->text : "SQE-RID-LAB",
                   "SquachLab2026!", channel, false, 1)) return false;
  uint8_t body[27];
  remoteFrame(0, body, s);
  uint8_t ie[35] = {0xDD, 33, 0xFA, 0x0B, 0xBC, 0x0D, 0x01,
                    0xF2, 25, 1};
  if (scenarioIs(s, "RID-WIFI-PARROT")) {
    ie[2] = 0x90; ie[3] = 0x3A; ie[4] = 0xE6;
  }
  memcpy(ie + 10, body + 2, 25);
  const esp_err_t err = esp_wifi_set_vendor_ie(
      true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, ie);
  g_wifiVendorIE = err == ESP_OK;
  if (!g_wifiVendorIE)
    Serial.printf("ERROR,WIFI_RID_IE,%d\n", (int)err);
  return g_wifiVendorIE;
}

static bool advertiseBle(const Scenario* s) {
  if (!beginBle()) return false;
  g_adv->stop();
  g_adv->clearData();
  if (s->kind == Kind::BleRemoteId) {
    g_remotePart = 0;
    g_lastRotation = millis();
    return advertiseRemote(0, s);
  }
  if (s->kind == Kind::BleRemoteNegative)
    return advertiseRemoteNegative(s);
  NimBLEAdvertisementData data;
  data.setFlags(0x06);
  bool ok = false;
  switch (s->kind) {
    case Kind::BleUuid:
      ok = data.setCompleteServices(NimBLEUUID(s->code));
      break;
    case Kind::BleName:
      ok = data.setName(s->text);
      break;
    case Kind::BleAppleNegative: {
      // A truncated iBeacon signature must not be classified as a full iBeacon.
      const uint8_t shortPayload[] = {0x4C, 0x00, 0x02, 0x15, 0x01, 0x02};
      const uint8_t normalPayload[] = {0x4C, 0x00, 0x01, 0x10, 0x00, 0x00};
      const bool shortId = strcmp(s->id, "BLE-NEG-IBEACON-SHORT") == 0;
      ok = shortId ? data.setManufacturerData(shortPayload, sizeof(shortPayload))
                   : data.setManufacturerData(normalPayload, sizeof(normalPayload));
      break;
    }
    case Kind::BleManufacturer: {
      if (strcmp(s->id, "BLE-AIRTAG") == 0) {
        // Detector's current heuristic checks for 4C 00 12 19.
        // Test fixture, not a complete real AirTag state machine.
        const uint8_t payload[] = {0x4C, 0x00, 0x12, 0x19, 0x01, 0x02};
        ok = data.setManufacturerData(payload, sizeof(payload));
      } else if (strcmp(s->id, "BLE-IBEACON") == 0 ||
                 strcmp(s->id, "BLE-IBEACON-V2") == 0) {
        // 25-byte mfg data: Apple CID, iBeacon marker, 16-byte UUID,
        // big-endian major/minor and signed measured power.
        uint8_t payload[25] = {
          0x4C, 0x00, 0x02, 0x15,
          0x53, 0x51, 0x55, 0x41, 0x43, 0x48, 0x45, 0x4D,
          0x49, 0x54, 0x2D, 0x54, 0x45, 0x53, 0x54, 0x21,
          0x00, 0x01, 0x00, 0x02, 0xC5
        };
        if (strcmp(s->id, "BLE-IBEACON-V2") == 0) {
          // Same valid iBeacon structure, independent UUID + major/minor.
          payload[19] ^= 0x01; payload[21] = 0x03; payload[23] = 0x04;
        }
        ok = data.setManufacturerData(payload, sizeof(payload));
      } else {
        const uint8_t payload[] = {(uint8_t)(s->code & 0xFF),
                                   (uint8_t)(s->code >> 8), 0x53, 0x51, 0x45};
        ok = data.setManufacturerData(payload, sizeof(payload));
      }
      break;
    }
    default: return false;
  }
  if (!ok || data.getPayload().size() > 31u) return false;
  if (!g_adv->setAdvertisementData(data)) return false;
  return g_adv->start();
}

static void stopTx() {
  if (g_adv && g_bleInit) g_adv->stop();
  if (g_wifiVendorIE) {
    esp_wifi_set_vendor_ie(false, WIFI_VND_IE_TYPE_BEACON,
                           WIFI_VND_IE_ID_0, nullptr);
    g_wifiVendorIE = false;
  }
  if (WiFi.getMode() != WIFI_OFF) {
    WiFi.softAPdisconnect(true);
    WiFi.mode(WIFI_OFF);
  }
}

static void finish(const char* status = "ok") {
  if (g_active) truth("END", g_active, status);
  stopTx();
  g_active = nullptr;
  g_started = 0;
}

static void cancelAll() {
  g_suite = false;
  finish("stopped");
  Serial.println("STATE,IDLE");
}

static bool startScenario(const Scenario* s, uint32_t duration = kDefaultMs) {
  if (!s) return false;
  if (g_active) finish("interrupted");
  stopTx();
  ++g_caseNumber;
  bool ok = false;
  if (s->kind == Kind::WifiAp) {
    // One ordinary WPA2 SoftAP. No spoofed BSSIDs or attacks.
    WiFi.mode(WIFI_AP);
    // Channel 0 is the legacy channel-1 profile; explicit 1..11 for LAB.
    const uint8_t channel = s->code ? static_cast<uint8_t>(s->code) : 1;
    if (channel >= 1 && channel <= 11)
      ok = WiFi.softAP(s->text, "SquachLab2026!", channel, false, 1);
  } else if (s->kind == Kind::WifiRemoteId) {
    ok = startWifiRemote(s);
  } else {
    ok = advertiseBle(s);
  }
  if (!ok) {
    truth("ERROR", s, "radio_start_failed");
    stopTx();
    return false;
  }
  g_active = s;
  g_started = millis();
  g_duration = duration;
  truth("BEGIN", s);
  Serial.printf("INFO,%s,%s,%s\n", s->id, s->expected, s->description);
  if (s->kind == Kind::WifiAp || s->kind == Kind::WifiRemoteId)
    Serial.printf("RADIO,CHANNEL,%u\n", s->code ? (unsigned)s->code : 1u);
  return true;
}

static void listCases() {
  for (size_t i = 0; i < kScenarioCount; ++i)
    Serial.printf("CASE,%s,%s,%s,%s\n", kScenarios[i].id,
                  (kScenarios[i].kind == Kind::WifiAp ||
                   kScenarios[i].kind == Kind::WifiRemoteId) ? "WIFI" : "BLE",
                  kScenarios[i].expected, kScenarios[i].description);
}

static void help() {
  Serial.println("SquachEmit 0.4.0-LAB | 115200 8N1 | all RF tests time-limited");
  Serial.println("HELP                  commands");
  Serial.println("LIST                  show all supported cases");
  Serial.println("RUN <ID> [seconds]    transmit for 3..30 seconds (default 12)");
  Serial.println("SUITE                 run each case with 2.4s off-air gaps");
  Serial.println("STOP                  stop current case/suite immediately");
  Serial.println("STATUS                show current case and time remaining");
  Serial.println("Remote ID profiles are synthetic; no genuine vendor serials or flight links.");
  Serial.println("BLE5 extended/coded PHY and Wi-Fi NAN: OFFLINE FIXTURES ONLY.");
  Serial.println("Single 2.4GHz radio: BLE and Wi-Fi scenarios are exclusive.");
#if defined(SQE_CYD32)
  Serial.println("CALTOUCH             reset touchscreen fit, then tap to calibrate");
#endif
}

static void command(String cmd) {
  cmd.trim();
  if (!cmd.length()) return;
  if (cmd.equalsIgnoreCase("HELP") || cmd == "?") { help(); return; }
  if (cmd.equalsIgnoreCase("LIST")) { listCases(); return; }
  if (cmd.equalsIgnoreCase("STOP")) { cancelAll(); return; }
#if defined(SQE_CYD32)
  if (cmd.equalsIgnoreCase("CALTOUCH")) { cyd32UiResetCalibration(); return; }
#endif
  if (cmd.equalsIgnoreCase("STATUS")) {
    const uint32_t now = millis();
    const uint32_t left = g_active ? (g_duration > now - g_started ? g_duration - (now - g_started) : 0) : 0;
    Serial.printf("STATUS,%s,%lu,%s\n", g_active ? g_active->id : "IDLE",
                  (unsigned long)left, g_suite ? "SUITE" : "MANUAL");
    return;
  }
  if (cmd.equalsIgnoreCase("SUITE")) {
    cancelAll();
    g_suite = true;
    g_suiteNext = 0;
    g_suiteAt = millis();
    Serial.println("STATE,SUITE_START");
    return;
  }
  if (cmd.startsWith("RUN ") || cmd.startsWith("run ")) {
    cmd = cmd.substring(4);
    cmd.trim();
    const int space = cmd.indexOf(' ');
    String id = (space < 0 ? cmd : cmd.substring(0, space));
    id.toUpperCase();
    const Scenario* s = findScenario(id.c_str());
    if (!s) { Serial.println("ERROR,unknown_id (use LIST)"); return; }
    uint32_t duration = kDefaultMs;
    if (space >= 0) {
      const String seconds = cmd.substring(space + 1);
      const long sec = seconds.toInt();
      if (sec < 3 || sec > 30 || seconds.length() == 0) {
        Serial.println("ERROR,seconds_must_be_3_to_30"); return;
      }
      duration = (uint32_t)sec * 1000UL;
    }
    g_suite = false;
    startScenario(s, duration);
    return;
  }
  Serial.println("ERROR,unknown_command (use HELP)");
}

void setup() {
  Serial.begin(115200);
  const uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1800) delay(10);
  WiFi.mode(WIFI_OFF);
  #if defined(SQE_WAVESHARE_C5)
  Serial.println("BOOT,SquachEmit,0.4.0-LAB,ESP32-C5-Waveshare-2.8");
  c5UiBegin();
#elif defined(SQE_CYD32)
  Serial.println("BOOT,SquachEmit,0.4.0-SQUACHWARE-R3-LAB,ESP32-2432S032R-ST7798");
  cyd32UiBegin();
#else
  Serial.println("BOOT,SquachEmit,0.4.0-LAB,ESP32-S3");
#endif
  help();
}

void loop() {
  while (Serial.available() > 0) {
    const int ch = Serial.read();
    if (ch == '\n' || ch == '\r') {
      if (g_line.length()) { command(g_line); g_line = ""; }
    } else if (ch >= 32 && ch <= 126 && g_line.length() < 100) {
      g_line += (char)ch;
    }
  }
  const uint32_t now = millis();
  if (g_active && g_active->kind == Kind::BleRemoteId && now - g_lastRotation >= kRemoteRotationMs) {
    const uint8_t parts = scenarioIs(g_active, "BLE-DRONE") ? 3 :
                          scenarioIs(g_active, "RID-NO-OPERATOR") ? 2 : 4;
    g_remotePart = (g_remotePart + 1) % parts;
    g_lastRotation = now;
    if (!advertiseRemote(g_remotePart, g_active)) {
      finish("remote_rotation_failed");
      g_suite = false;
    }
  }
  if (g_active && now - g_started >= g_duration) {
    finish();
    g_suiteAt = millis() + kSuiteGapMs;
  }
  if (g_suite && !g_active && (int32_t)(millis() - g_suiteAt) >= 0) {
    if (g_suiteNext >= kScenarioCount) {
      Serial.println("STATE,SUITE_DONE");
      g_suite = false;
    } else if (!startScenario(&kScenarios[g_suiteNext++])) {
      g_suite = false;
      Serial.println("STATE,SUITE_ABORTED");
    }
  }
#if defined(SQE_WAVESHARE_C5) || defined(SQE_CYD32)
  const uint32_t current = millis();
  const uint32_t remaining = (g_active && g_duration > current-g_started)
    ? g_duration-(current-g_started) : 0;
#if defined(SQE_CYD32)
  const Cyd32UiAction action = cyd32UiTick(
      g_active ? g_active->id : nullptr, remaining, g_suite);
  switch(action) {
    case Cyd32UiAction::Run: {
      const Scenario* chosen = cyd32UiSelection();
      if(chosen){g_suite=false;startScenario(chosen);}
      break;
    }
    case Cyd32UiAction::Stop: cancelAll(); break;
    case Cyd32UiAction::Suite:
      cancelAll();
      g_suite=true;g_suiteNext=0;g_suiteAt=millis();
      Serial.println("STATE,SUITE_START");
      break;
    default: break;
  }
#else
  switch(c5UiTick(g_active ? g_active->id : nullptr,remaining,g_suite)){
    case C5UiAction::Run: {
      const Scenario* selected=c5UiSelection();
      if(selected){g_suite=false;startScenario(selected);}
      break;
    }
    case C5UiAction::Stop: cancelAll(); break;
    case C5UiAction::Suite:
      cancelAll();
      g_suite=true;g_suiteNext=0;g_suiteAt=millis();
      Serial.println("STATE,SUITE_START");
      break;
    default: break;
  }
#endif
#endif
  delay(5);
}
