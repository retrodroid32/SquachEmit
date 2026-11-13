#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <NimBLEDevice.h>
#include "scenarios.h"
#include "rid_wifi_pack.h"
#include "ap_mac_lifecycle.h"
#include "identity.h"
#include "dji_oui_lab.h"
#include "uav_vendor_oui_lab.h"
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
static const char* g_wifiFailureStage = "UNKNOWN";
static esp_err_t g_wifiFailureCode = ESP_OK;
static void wifiFailure(const char* stage, esp_err_t code = ESP_FAIL) {
  g_wifiFailureStage = stage;
  g_wifiFailureCode = code;
  Serial.printf("ERROR,WIFI_STAGE,%s,%d,%s\n",stage,(int)code,esp_err_to_name(code));
}
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
static char g_address[18] = "";
static uint32_t g_radioIdentity = 0;

static void truth(const char* event, const Scenario* s, const char* status = "ok") {
  // Existing keys remain stable for the serial truth consumer. 0 = BLE.
  const bool wifi = s && (s->kind == Kind::WifiAp || s->kind == Kind::WifiRemoteId);
  const unsigned channel = wifi ? (s->code ? (unsigned)s->code : 1u) : 0u;
  Serial.printf("TRUTH,{\"event\":\"%s\",\"seq\":%lu,\"ms\":%lu,\"id\":\"%s\",\"expected\":\"%s\",\"status\":\"%s\",\"carrier\":\"%s\",\"channel\":%u,\"mac\":\"%s\"}\n",
                event, (unsigned long)g_caseNumber, (unsigned long)millis(),
                s ? s->id : "", s ? s->expected : "NONE", status,
                wifi ? "WIFI" : "BLE", channel, g_address);
}


// Each scenario gets a reproducible identity. Only explicit OUI-only
// lab fixtures replace the local test MAC with a vendor-registered prefix.
// Never alter eFuse or claim a real vendor-issued device identity.
static bool deriveIdentity(const Scenario* s, bool wifi, uint8_t out[6]) {
  if (!s) return false;
  size_t index = kScenarioCount;
  for (size_t i=0; i<kScenarioCount; ++i)
    if (&kScenarios[i] == s) { index=i; break; }
  if (index >= kScenarioCount || index >= 254) return false;
  g_radioIdentity = (uint32_t)index + 1u;
  SquachIdentity::derive(ESP.getEfuseMac(), index,
        wifi ? SquachIdentity::Radio::Wifi : SquachIdentity::Radio::Ble, out);
  // Only selected OUI-only cases use allocated vendor prefixes.
// Other Wi-Fi and BLE tests retain locally administered lab identities.
  if (wifi && SquachDjiOuiLab::find(s->id)) {
    if (!SquachDjiOuiLab::apply(s->id,out)) return false;
  }
  if (wifi && SquachUavVendorLab::find(s->id)) {
    if (!SquachUavVendorLab::apply(s->id,out)) return false;
  }
  SquachIdentity::text(out, g_address);
  return true;
}

static bool setBleIdentity(const Scenario* s) {
  uint8_t mac[6];
  if (!deriveIdentity(s, false, mac)) return false;
  const NimBLEAddress addr(mac, BLE_ADDR_RANDOM);
  if (!NimBLEDevice::setOwnAddr(addr)) {
    Serial.println("ERROR,BLE_MAC_SET");
    return false;
  }
  if (!NimBLEDevice::setOwnAddrType(BLE_OWN_ADDR_RANDOM)) {
    Serial.println("ERROR,BLE_MAC_TYPE");
    return false;
  }
  if (NimBLEDevice::getAddress() != addr) {
    Serial.println("ERROR,BLE_MAC_READBACK");
    return false;
  }
  return true;
}

// Configure a synthetic AP MAC with the target interface in AP mode but
// disabled. Previous STA-only -> esp_wifi_set_mac(WIFI_IF_AP) caused AP_MAC_SET.
struct WiFiApMacDriver {
  bool startSta() { return WiFi.mode(WIFI_STA); }
  int32_t stop() { return (int32_t)esp_wifi_stop(); }
  int32_t setApMode() { return (int32_t)esp_wifi_set_mode(WIFI_MODE_AP); }
  int32_t setMac(const uint8_t mac[6]) {
    return (int32_t)esp_wifi_set_mac(WIFI_IF_AP, mac);
  }
  int32_t restart() { return (int32_t)esp_wifi_start(); }
  int32_t getMac(uint8_t out[6]) {
    return (int32_t)esp_wifi_get_mac(WIFI_IF_AP, out);
  }
};
static bool beginWifiIdentity(const Scenario* s) {
  uint8_t mac[6];
  if (!deriveIdentity(s,true,mac)) {
    wifiFailure("AP_IDENTITY"); return false;
  }
  WiFiApMacDriver driver;
  const SquachApMac::Result result=SquachApMac::configure(driver,mac);
  if (!result.ok()) {
    wifiFailure(SquachApMac::stageName(result.stage),(esp_err_t)result.code);
    WiFi.mode(WIFI_OFF);
    return false;
  }
  Serial.printf("RADIO,AP_MAC_READY,%s,%s\n",s->id,g_address);
  return true;
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
      // The Wi-Fi model tests use their own clearly synthetic SQE-DJI Basic ID,
      // not the shared generic fallback. Never borrow real aircraft serials.
      const bool wifiModel = s && s->kind == Kind::WifiRemoteId &&
                             strncmp(s->id,"RID-WIFI-DJI-",13)==0;
      const char* serial = (s && s->text && (s->kind == Kind::BleRemoteId || wifiModel))
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
    case 4:
      m[0] = 0x52; // Operator ID message, protocol version 2
      m[1] = 0;    // operator ID type (synthetic)
      strncpy(reinterpret_cast<char*>(m + 2), "SQE-LAB-OPERATOR", 20);
      break;
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
  if (channel < 1 || channel > 11) {
    wifiFailure("CHANNEL",ESP_ERR_INVALID_ARG); return false;
  }
  if (!beginWifiIdentity(s)) return false;
  if (!WiFi.softAP(s->text ? s->text : "SQE-RID-LAB",
                   "SquachLab2026!", channel, false, 1)) {
    wifiFailure("SOFTAP_START"); return false;
  }
  // Synthetic message pack: Basic ID, Location, System, Self-ID, Operator ID.
  // This tests complete receiver parsing but is not flight-compliant broadcast:
  // there is no true GNSS/UTC on the emitter and all identities are fictional.
  uint8_t msgs[SquachRidWifi::kMsgCount][SquachRidWifi::kMsgBytes] = {};
  for(size_t i=0;i<SquachRidWifi::kMsgCount;++i) {
    uint8_t frame[27] = {};
    remoteFrame((uint8_t)i,frame,s);
    memcpy(msgs[i],frame+2,SquachRidWifi::kMsgBytes);
  }
  uint8_t ie[SquachRidWifi::kIeBytes] = {};
  if(!SquachRidWifi::encode(msgs,1,ie,sizeof(ie))) {
    wifiFailure("PACK_ENCODE",ESP_ERR_INVALID_ARG); return false;
  }
  if (scenarioIs(s, "RID-WIFI-PARROT")) {
    ie[2] = 0x90; ie[3] = 0x3A; ie[4] = 0xE6;
  }
  const esp_err_t err = esp_wifi_set_vendor_ie(
      true, WIFI_VND_IE_TYPE_BEACON, WIFI_VND_IE_ID_0, ie);
  g_wifiVendorIE = err == ESP_OK;
  if (!g_wifiVendorIE) wifiFailure("VENDOR_IE",err);
  return g_wifiVendorIE;
}

static bool advertiseBle(const Scenario* s) {
  if (!beginBle()) return false;
  g_adv->stop();
  if (!setBleIdentity(s)) return false;
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

static bool isOuiOnly(const Scenario* s) {
  return s && (SquachDjiOuiLab::find(s->id)!=nullptr ||
               SquachUavVendorLab::find(s->id)!=nullptr);
}
// CYD 3.2 time selection controls touchscreen RUN and every SUITE case.
// Other boards keep their existing 12-second default.
static uint32_t selectedDurationMs(){
#if defined(SQE_CYD32)
  return cyd32UiDurationMs();
#else
  return kDefaultMs;
#endif
}

static bool startScenario(const Scenario* s, uint32_t duration = kDefaultMs) {
  if (!s) return false;
  if (g_active) finish("interrupted");
  stopTx();
  ++g_caseNumber;
  g_address[0] = '\0';
  bool ok = false;
  if (s->kind == Kind::WifiAp) {
    const bool ouiTest=isOuiOnly(s);
    // The vendor-OUI fixtures contain NO Remote ID; the BSSID is the
    // selected registry prefix with a generated, synthetic 3-byte suffix.
    if (!beginWifiIdentity(s)) {
      if(ouiTest) {
        Serial.printf("ERROR,WIFI_OUI_CASE,%s,%s,%d\n",
                      s->id,g_wifiFailureStage,(int)g_wifiFailureCode);
#if defined(SQE_CYD32)
        cyd32UiSetRadioError(g_wifiFailureStage);
#endif
      }
      truth("ERROR",s,"identity_failed");stopTx();return false;
    }
    // Channel 0 is the legacy channel-1 profile; explicit 1..11 for LAB.
    const uint8_t channel = s->code ? static_cast<uint8_t>(s->code) : 1;
    if (channel >= 1 && channel <= 11)
      ok = WiFi.softAP(s->text, "SquachLab2026!", channel, false, 1);
    if(ouiTest && !ok) wifiFailure("SOFTAP_START");
  } else if (s->kind == Kind::WifiRemoteId) {
    // One bounded recovery after a failed Wi-Fi mode transition.
    for(unsigned attempt=1; attempt<=2 && !ok; ++attempt){
      Serial.printf("RADIO,WIFI_ATTEMPT,%s,%u\n",s->id,attempt);
      g_wifiFailureStage="UNKNOWN"; g_wifiFailureCode=ESP_OK;
      ok=startWifiRemote(s);
      if(!ok && attempt==1){
        Serial.printf("WARN,WIFI_RETRY,%s,%s\n",s->id,g_wifiFailureStage);
        stopTx();
        delay(120);
      }
    }
  } else {
    ok = advertiseBle(s);
  }
  if (!ok) {
    if(s->kind == Kind::WifiRemoteId || isOuiOnly(s))
      Serial.printf("ERROR,WIFI_CASE,%s,%s,%d\n",s->id,g_wifiFailureStage,(int)g_wifiFailureCode);
    truth("ERROR", s, "radio_start_failed");
    stopTx();
#if defined(SQE_CYD32)
    cyd32UiSetRadioError((s->kind == Kind::WifiRemoteId ||
          isOuiOnly(s)) ? g_wifiFailureStage : "RADIO_START");
#endif
    return false;
  }
#if defined(SQE_CYD32)
  cyd32UiSetRadioError(nullptr);
#endif
  g_active = s;
  g_started = millis();
  g_duration = duration;
  truth("BEGIN", s);
  if(isOuiOnly(s))
    Serial.printf("TEST,OUI_ONLY,%s,%s,NO_REMOTE_ID\n",s->id,g_address);
  Serial.printf("INFO,%s,%s,%s\n", s->id, s->expected, s->description);
  Serial.printf("IDENTITY,%s,%s\n", s->id, g_address);
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
  Serial.printf("SquachEmit LAB | %u cases | 115200 8N1 | all RF tests time-limited\n", (unsigned)kScenarioCount);
  Serial.println("HELP                  commands");
  Serial.println("LIST                  show all supported cases");
  Serial.println("RUN <ID> [seconds]    transmit 3..30s; default is selected TX time (CYD3.2) or 12s");
  Serial.println("SUITE                 run each case with 2.4s off-air gaps");
  Serial.println("STOP                  stop current case/suite immediately");
  Serial.println("STATUS                show current case and time remaining");
  Serial.println("IDENTITY              current synthetic case MAC");
  Serial.println("Remote ID profiles are synthetic; no genuine vendor serials or flight links.");
  Serial.println("BLE5 extended/coded PHY and Wi-Fi NAN: OFFLINE FIXTURES ONLY.");
  Serial.println("Single 2.4GHz radio: BLE and Wi-Fi scenarios are exclusive.");
#if defined(SQE_CYD32)
  Serial.println("UI ON/OFF/GET, UI TOUCH x y DOWN|MOVE|UP — local software-rendered desktop UI");
  Serial.println("UI STATE transmits only navigation and test state; no LCD pixel readback.");
#endif
#if defined(SQE_CYD32)
  Serial.println("Tap TX TIME on the home screen to set 3..30s for RUN and SUITE.");
  Serial.println("CALTOUCH             reset touchscreen fit, then tap to calibrate");
  Serial.println("COLORS               show idle-only RED/GREEN/BLUE/WHITE/BLACK test");
  Serial.println("INVERT ON|OFF        save absolute panel inversion state");
#endif
}

// UI state is opt-in and is not coupled to receiver LOG evidence, RF test
// dispatch, baud switching or any physical TFT readback.
static bool g_uiStreamOn=false;
static bool g_uiStatePending=false;
static void command(String cmd) {
  cmd.trim();
  if (!cmd.length()) return;
  if (cmd.startsWith("UI ") || cmd.startsWith("ui ")) {
#if defined(SQE_CYD32)
    cmd.toUpperCase();
    if(cmd=="UI ON"){
      g_uiStreamOn=true;g_uiStatePending=true;
      Serial.println("UI READY 1 320 240 MODEL");
      return;
    }
    if(cmd=="UI OFF"){
      g_uiStreamOn=false;g_uiStatePending=false;
      cyd32UiRemoteTouch(0,0,"UP");
      Serial.println("UI OFF OK");
      return;
    }
    if(cmd=="UI GET"){
      g_uiStatePending=true;return;
    }
    unsigned x=0,y=0;char phase[8]={0};char garbage=0;
    if(sscanf(cmd.c_str(),"UI TOUCH %u %u %7s %c",&x,&y,phase,&garbage)==3
       && x<320 && y<240
       && cyd32UiRemoteTouch((uint16_t)x,(uint16_t)y,phase)){
      g_uiStatePending=true;
      Serial.println("UI TOUCH OK");
      return;
    }
    Serial.println("UI ERROR UNSUPPORTED_COMMAND");
#else
    Serial.println("UI ERROR UNSUPPORTED_BOARD");
#endif
    return;
  }
  // Old pixel-capture commands intentionally do nothing in R7. No
  // accidental GRAM readback or 2Mbps handoff is possible from desktop.
  if(cmd.startsWith("MIRROR ")||cmd.startsWith("mirror ")){
    Serial.println("MIRROR ERROR REMOVED_USE_UI_STATE");
    return;
  }
  if (cmd.equalsIgnoreCase("HELP") || cmd == "?") { help(); return; }
  if (cmd.equalsIgnoreCase("LIST")) { listCases(); return; }
  if (cmd.equalsIgnoreCase("STOP")) { cancelAll(); return; }
#if defined(SQE_CYD32)
  if (cmd.equalsIgnoreCase("CALTOUCH")) { cyd32UiResetCalibration(); return; }
  if (cmd.equalsIgnoreCase("INVERT ON")) { cyd32UiSetInversion(true); return; }
  if (cmd.equalsIgnoreCase("INVERT OFF")) { cyd32UiSetInversion(false); return; }
  if (cmd.equalsIgnoreCase("COLORS")) {
    if (g_active || g_suite) {Serial.println("ERROR,COLORS_REQUIRES_IDLE");return;}
    cyd32UiColorBars();return;
  }
#endif
  if (cmd.equalsIgnoreCase("IDENTITY")) {
    Serial.printf("IDENTITY,%s,%s,%lu\n",g_active?g_active->id:"IDLE",
                  g_active?g_address:"",(unsigned long)g_radioIdentity);
    return;
  }
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
    uint32_t duration = selectedDurationMs();
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
  Serial.println("BOOT,SquachEmit,v1.1.0,ESP32-C5-Waveshare-2.8,172-CASES");
  c5UiBegin();
#elif defined(SQE_CYD32)
  Serial.println("BOOT,SquachEmit,v1.1.0,ESP32-2432S032R-ST7798,172-CASES-TXTIME");
  cyd32UiBegin();
#else
  Serial.println("BOOT,SquachEmit,v1.1.0,ESP32-S3,172-CASES");
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
    } else if (!startScenario(&kScenarios[g_suiteNext++],selectedDurationMs())) {
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
      if(chosen) {
        const bool ouiCase=isOuiOnly(chosen);
        Serial.printf("UI,RUN_TAP,%s,%s\n",chosen->id,
              ouiCase?"WIFI_OUI":chosen->kind==Kind::WifiRemoteId?"WIFI_RID":"OTHER");
        g_suite=false;
        if(chosen->kind==Kind::WifiRemoteId || ouiCase) {
          cyd32UiSetRadioStarting(chosen->id);
          // Render the accepted touch before the potentially blocking Wi-Fi
          // init, so an operator sees a response even when startup fails.
          cyd32UiTick(nullptr,0,false);
        }
        Serial.printf("UI,RUN_DISPATCH,%s\n",chosen->id);
        startScenario(chosen,selectedDurationMs());
      } else {
        Serial.println("ERROR,UI_RUN_NO_SELECTION");
        cyd32UiSetRadioError("NO_SELECTION");
      }
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
  // The CYD remains authoritative for navigation, selection and RF.
  // Only tiny UI records are sent; no LCD readback or huge pixel streams.
#if defined(SQE_CYD32)
  if(g_uiStreamOn||g_uiStatePending){
    cyd32UiStreamState(g_active?g_active->id:nullptr,remaining,g_suite,g_uiStatePending);
    g_uiStatePending=false;
  }
#endif
#endif
  delay(5);
}
