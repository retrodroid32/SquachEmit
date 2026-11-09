#include <Arduino.h>
#include <WiFi.h>
#include <NimBLEDevice.h>
#include "scenarios.h"
#include <string.h>

// SquachEmit v0.1.0: bounded, explicit lab emissions; no unattended RF flood.
// In every test the expected label is a *detector classification*, not a
// claim the actual product named by that classification is present.
static constexpr uint32_t kDefaultMs = 12000;
static constexpr uint32_t kMaxMs = 30000;
static constexpr uint32_t kMinMs = 3000;
static constexpr uint32_t kRemoteRotationMs = 1300;
static constexpr uint32_t kSuiteGapMs = 2400;

static NimBLEAdvertising* g_adv = nullptr;
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
  Serial.printf("TRUTH,{\"event\":\"%s\",\"seq\":%lu,\"ms\":%lu,\"id\":\"%s\",\"expected\":\"%s\",\"status\":\"%s\"}\n",
                event, (unsigned long)g_caseNumber, (unsigned long)millis(),
                s ? s->id : "", s ? s->expected : "NONE", status);
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
static void remoteFrame(uint8_t which, uint8_t* body27) {
  memset(body27, 0, 27);
  body27[0] = 0x0D; // ASTM OpenDroneID application code
  body27[1] = which + 1; // counter
  uint8_t* m = body27 + 2;
  switch (which) {
    case 0:
      m[0] = 0x02; m[1] = 0x12;
      memcpy(m + 2, "SQUACHEMIT-TEST-0001", 20);
      break;
    case 1:
      m[0] = 0x12;
      put32(m + 5, 407128000);  // synthetic aircraft 40.7128
      put32(m + 9, -740060000); // synthetic aircraft -74.0060
      m[15] = 0xC0; m[16] = 0x08; // 120m = (120+1000)*2 = 2240
      break;
    case 2:
      m[0] = 0x42;
      put32(m + 2, 407580000);  // synthetic operator 40.7580
      put32(m + 6, -739855000); // synthetic operator -73.9855
      break;
  }
}

static bool advertiseRemote(uint8_t part) {
  if (!g_adv || !g_bleInit) return false;
  g_adv->stop();
  uint8_t body[27];
  remoteFrame(part, body);
  NimBLEAdvertisementData data;
  // Do NOT add flags or a name: legacy OpenDroneID fills all 31 bytes.
  if (!data.setServiceData(NimBLEUUID((uint16_t)0xFFFA), body, sizeof(body))) return false;
  const auto packet = data.getPayload();
  if (packet.size() != 31u) return false;
  if (!g_adv->setAdvertisementData(data)) return false;
  return g_adv->start();
}

static bool advertiseBle(const Scenario* s) {
  if (!beginBle()) return false;
  g_adv->stop();
  g_adv->clearData();
  if (s->kind == Kind::BleRemoteId) {
    g_remotePart = 0;
    g_lastRotation = millis();
    return advertiseRemote(0);
  }
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
      const uint8_t payload[] = {0x4C, 0x00, 0x01, 0x10, 0x00, 0x00};
      ok = data.setManufacturerData(payload, sizeof(payload));
      break;
    }
    case Kind::BleManufacturer: {
      if (strcmp(s->id, "BLE-AIRTAG") == 0) {
        // Detector's current heuristic checks for 4C 00 12 19.
        // Test fixture, not a complete real AirTag state machine.
        const uint8_t payload[] = {0x4C, 0x00, 0x12, 0x19, 0x01, 0x02};
        ok = data.setManufacturerData(payload, sizeof(payload));
      } else if (strcmp(s->id, "BLE-IBEACON") == 0) {
        // 25-byte mfg data: Apple CID, iBeacon marker, 16-byte UUID,
        // big-endian major/minor and signed measured power.
        const uint8_t payload[25] = {
          0x4C, 0x00, 0x02, 0x15,
          0x53, 0x51, 0x55, 0x41, 0x43, 0x48, 0x45, 0x4D,
          0x49, 0x54, 0x2D, 0x54, 0x45, 0x53, 0x54, 0x21,
          0x00, 0x01, 0x00, 0x02, 0xC5
        };
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
    ok = WiFi.softAP(s->text, "SquachLab2026!", 1, false, 1);
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
  return true;
}

static void listCases() {
  for (size_t i = 0; i < kScenarioCount; ++i)
    Serial.printf("CASE,%s,%s,%s,%s\n", kScenarios[i].id,
                  kScenarios[i].kind == Kind::WifiAp ? "WIFI" : "BLE",
                  kScenarios[i].expected, kScenarios[i].description);
}

static void help() {
  Serial.println("SquachEmit 0.1.0 | 115200 8N1 | all RF tests time-limited");
  Serial.println("HELP                  commands");
  Serial.println("LIST                  show all supported cases");
  Serial.println("RUN <ID> [seconds]    transmit for 3..30 seconds (default 12)");
  Serial.println("SUITE                 run each case with 2.4s off-air gaps");
  Serial.println("STOP                  stop current case/suite immediately");
  Serial.println("STATUS                show current case and time remaining");
  Serial.println("Single 2.4GHz radio: BLE and Wi-Fi scenarios are exclusive.");
}

static void command(String cmd) {
  cmd.trim();
  if (!cmd.length()) return;
  if (cmd.equalsIgnoreCase("HELP") || cmd == "?") { help(); return; }
  if (cmd.equalsIgnoreCase("LIST")) { listCases(); return; }
  if (cmd.equalsIgnoreCase("STOP")) { cancelAll(); return; }
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
  Serial.println("BOOT,SquachEmit,0.1.0,ESP32-S3");
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
    g_remotePart = (g_remotePart + 1) % 3;
    g_lastRotation = now;
    if (!advertiseRemote(g_remotePart)) {
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
  delay(5);
}
