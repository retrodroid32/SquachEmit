#if defined(SQE_CYD32)
#include "cyd32_ui.h"
#include "scenarios.h"
#include "scenario_library.h"
#include "scenario_display.h"
#include "cyd32_squachy_port.h"
#include "cyd32_fast_sprite.h"
#include "cyd32_frame_push.h"
#include "sqe_bubble_timing.h"
#include "sqe_battery_filter.h"
#include "sqe_battery_calibration.h"
#include "sqe_bubble_layout.h"
#include "sqe_category_quips.h"
#include "sqe_touch_fit.h"
#include "sqe_touch_wizard.h"
#if defined(SQE_ORIGINAL_SQUACHY)
#include <squachy.h>
#include <theme.h>
#include <detection.h>
#include <draw_band.h>
#include <settings.h>
#endif
#include "squachwatch_synthwave_exact.h"
#include "squachwatch_fireflies_exact.h"
#include "squachwatch_terminal_exact.h"
#include "squachwatch_digital_rain_exact.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <string.h>
#include <time.h>

// This screen structure follows the owner's SquachWatch-CYD CLEAR and
// SETTINGS screens: centered large Squachy, fine 14px vapor title,
 // square soft buttons and full-width outlined list rows.
 // Original reduced drawing; no SquachWatch dependency or GPL assets.
namespace {
constexpr int kW=320,kH=240;
constexpr int kFooterY=214,kFooterH=23;
constexpr int kMenuTop=43,kMenuStep=39,kCasesTop=51,kCasesStep=36;
constexpr uint16_t kBg=0x0801,kTaskbar=0x0803;
uint16_t kPurple=0xAC1F;
uint16_t kCyan=0x07FF,kPink=0xF96F;
constexpr uint16_t kVaporPink=0xFB99;
constexpr uint16_t kVaporPurple=0xBB5F,kYellow=0xFFD2,kGreen=0x07E0;
constexpr uint16_t kWhite=0xFFFF,kMuted=0x7393,kInk=0x1801;
constexpr uint16_t kFurDark=0x38C0,kFurMain=0x5941,kFurLight=0x9326;
constexpr uint16_t kSkin=0xF60F,kSkinDark=0xCB88;

enum class Screen : uint8_t { Home, Categories, Library, Duration, Details, Diagnostics, Favorites, History, Settings, Help, Identity, Recovery, Activity, Evidence, QuickFilter, Portrait, MainMenu, Outfits, OtaInfo, Backgrounds, BatteryCalibration };
constexpr size_t kPageRows=4;

// SquachWatch-style staged rendering: physical ST7798 panel is never
// cleared on animated frames. Only a 320x120 / 240x160 8-bit band is
// allocated, not a second full-screen buffer.
TFT_eSPI panel;
TFT_eSPI* paint=&panel;
#define lcd (*paint)
// SquachWatch FastSprite replaces expensive drawPixel / drawLine /
// drawFastVLine / GLCD drawChar calls with direct 8-bit buffer writes.
// Its original viewport + setWindow safety clipping is preserved on CYD32.
FastSprite halfFrame(&panel);
TFT_eSprite squachSprite(&panel);
bool squachSpriteReady=false,bandFrameReady=false,bandFrameActive=false;
// Use one 8-bit full-frame sprite when RAM allows; retain HALF8 fallback.
uint8_t sceneFrameBands=2;
// A/B switch: optimized rows over overlapped SPI by default. PUSH OFF
// restores TFT_eSPI::pushSprite immediately if a board behaves differently.
bool fastPushDesired=true;
uint32_t fastPushFallbacks=0;
uint32_t fastPushLastRows=0,fastPushLastHashUs=0,fastPushLastWireUs=0;
bool fastPushLastUsed=false;
int bandFrameTop=0;
bool advanceScene=true;
// Both LCD bands must sample the same animation clock; millis() in each
// rendering pass produces a different scene/character frame at the seam.
uint32_t logicalFrameMs=0;
uint32_t frameNow(){return bandFrameActive?logicalFrameMs:millis();}
uint32_t lastSquachFrame=0, lastSceneFrame=0, petUntil=0;
Preferences prefs;
Screen screen=Screen::Home;
DeviceCategory category=DeviceCategory::Axon;
size_t selected=0,page=0,categoryPage=0,favoritesPage=0,historyPage=0;
// Fixed 256-slot bitmap: kScenarioCount is externally linked (not constexpr).
uint8_t favBits[32]={0};
struct HistoryEntry { char id[56]; char status[20]; uint32_t uptime; };
HistoryEntry history[8]{};
uint8_t historyHead=0,historyCount=0;
uint32_t lastListsAt=0;
bool playlistRunning=false,playlistPaused=false;
unsigned playlistIndex=0,playlistTotal=0;
uint8_t brightnessLevel=2,animationLevel=1,screenTimeout=0,themeLevel=0,fontMode=1;
bool settingsMore=false;
bool openedFromMainMenu=false;
bool quickFromCategories=false;
bool settingsSubmenuFlow=false;
bool pcControl=false,screenDimmed=false;
unsigned resetReasonCode=0;
uint32_t lastTouchActivity=0,lastDeviceAt=0;
char identityCase[56]={0},identityMac[18]={0};
uint32_t txBegins=0,txEnds=0,txErrors=0,actualTxDuration=0,lastActivityAt=0;
bool txDriverActive=false;
char rxId[56]={0},rxLabel[28]={0},rxQuality[16]={0};
int rxRssi=0;
uint32_t lastEvidenceAt=0,lastFilterAt=0;
uint8_t filterLetter=0; // 0 = ALL; 1..26 = A..Z
size_t filterPage=0;
uint8_t helpPage=0,menuPage=0,outfitLevel=0,backgroundLevel=0,backgroundPage=0,outfitPage=0;
bool onAirIndicator=false;
uint16_t batteryMillivolts=0;
SqeBattery::Meter batteryMeter{};
uint16_t batteryAdcSamples[SqeBattery::kSamples]={0};
uint8_t batteryAdcCount=0;
bool batteryAdcCollecting=false;
int8_t batteryPercent=-1; // only used to gate settled ADC validity
int16_t batteryOffsetMv=0; // NVS battery-side voltage correction
bool batteryDisplayVolts=false; // false: %, true: V; persists across restarts
uint32_t lastBatterySample=0;
uint32_t lastClockMinute=UINT32_MAX;
bool portraitCalibrated=false,portraitMode=false;
// SquachWatch five-target affine fit, stored separately from old TFT_eSPI blob.
TouchFit::Fit portraitFit{};
bool portraitFitLoaded=false;
// Device-side portrait touch recovery independent of stored touch fit.
bool forcePortraitCalibration=false;
uint32_t portraitRawHoldStarted=0;
bool portraitCalibrationPending=false;
bool portraitTouchTrace=false;
uint32_t lastPortraitTouchTraceAt=0,portraitMenuRawRescues=0;
uint16_t portraitTouchData[5]={0};
SqeBubble::State landscapeBubble{};
char speechActiveCase[56]={0};
const char* landscapeSpeechText="READY FOR A SIGNAL CHECK!";
// Deprecated event-only quips remain cataloged but never enter the popup selector.
uint8_t durationSeconds=12; // 3..30s, saved independently of touch calibration
bool remoteDown=false;
uint16_t remoteX=0,remoteY=0;
uint32_t remoteAt=0,mirrorSequence=0;
bool ready=false,calibrated=false,redraw=true,wasPressed=false,previousSuite=false;
bool panelInverted=false;
uint32_t lastPoll=0,lastPress=0,lastCount=UINT32_MAX,colorBarsUntil=0,lastDiagAt=0;
uint32_t portraitLastCount=UINT32_MAX;
uint8_t talkIndex=0;
char previousActive[56]={0};
char radioStartError[28]={0}; // visible radio-start failure stage
char radioStartingProfile[56]={0}; // visible acknowledgement of requested Wi-Fi RUN

void label(int x,int y,const char* s,uint16_t col,uint8_t font=1) {
  lcd.setTextFont(font);
  lcd.setTextSize(1);
  lcd.setTextColor(col);
  lcd.setCursor(x,y);
  if(s)lcd.print(s);
}
void fit(int x,int y,const char* s,int width,uint16_t col,uint8_t font=2) {
  if(!s)return;
  lcd.setTextFont(font);
  char b[64]={0};
  strncpy(b,s,sizeof(b)-1);
  while(b[0]&&lcd.textWidth(b)>width)b[strlen(b)-1]='\0';
  label(x,y,b,col,font);
}
uint16_t blend(uint16_t a,uint16_t b,uint8_t t) {
  const int ar=(a>>11)&31,ag=(a>>5)&63,ab=a&31;
  const int br=(b>>11)&31,bg=(b>>5)&63,bb=b&31;
  return (uint16_t)(((ar+(br-ar)*t/255)<<11)|
                    ((ag+(bg-ag)*t/255)<<5)|
                     (ab+(bb-ab)*t/255));
}
// Battery-backed CYD32 revisions expose VBAT through a 1:1 divider on GPIO34.
// Charger status needs a verified physical GPIO; voltage alone cannot prove charging.
uint16_t correctedBatteryMv(){
  return SqeBatteryCalibration::correctedMv(batteryMillivolts,batteryOffsetMv);
}
void setBatteryOffsetMv(int32_t next){
  const int16_t candidate=SqeBatteryCalibration::clampOffset(next);
  if(candidate==batteryOffsetMv)return;
  batteryOffsetMv=candidate;
  if(prefs.begin("sqemit32",false)){
    if(prefs.putInt("batoff_mv",(int32_t)batteryOffsetMv)!=sizeof(int32_t))
      Serial.println("WARN,BATTERY_OFFSET_SAVE_FAILED");
    prefs.end();
  }else Serial.println("WARN,BATTERY_OFFSET_NVS_UNAVAILABLE");
  Serial.printf("CYD32,BATCAL,OFFSET_MV=%d,CORRECTED_MV=%u\n",
                (int)batteryOffsetMv,(unsigned)correctedBatteryMv());
  redraw=true;
}
void setBatteryDisplayVolts(bool volts){
  if(batteryDisplayVolts==volts)return;
  batteryDisplayVolts=volts;
  if(prefs.begin("sqemit32",false)){
    prefs.putBool("batshow_v",batteryDisplayVolts);
    prefs.end();
  }else Serial.println("WARN,BATTERY_DISPLAY_NVS_UNAVAILABLE");
  Serial.printf("CYD32,BATCAL,HEADER_MODE=%s\n",batteryDisplayVolts?"VOLTS":"PERCENT");
  redraw=true;
}
void sampleBattery(){
  const uint32_t now=millis();
  // Allow ESP32 supply, RF and the 50k-ohm equivalent divider to settle.
  if(now<3000u)return;
#if defined(ARDUINO_ARCH_ESP32)
  if(!batteryAdcCollecting){
    // Startup: acquire four separate 32-sample windows before showing
    // any percentage. Later, refresh every 15 seconds as before.
    const uint32_t cadence=batteryPercent<0?3000u:15000u;
    if(lastBatterySample && now-lastBatterySample<cadence)return;
    analogSetPinAttenuation(34,ADC_11db);
    // Discard initial conversion results after enabling attenuation.
    for(int i=0;i<8;++i)(void)analogReadMilliVolts(34);
    batteryAdcCount=0;
    batteryAdcCollecting=true;
  }
  // Spread 32 ADC samples over multiple display loops so Squachy retains
  // its animation frame rate. ADC1 works alongside Wi-Fi/BLE.
  for(int i=0;i<4&&batteryAdcCount<SqeBattery::kSamples;++i)
    batteryAdcSamples[batteryAdcCount++]=(uint16_t)analogReadMilliVolts(34);
  if(batteryAdcCount<SqeBattery::kSamples)return;
  batteryAdcCollecting=false;
  lastBatterySample=now;
  const bool valid=batteryMeter.accept(batteryAdcSamples);
  batteryMillivolts=batteryMeter.filteredMillivolts;
  batteryPercent=batteryMeter.displayedPercent;
  Serial.printf("CYD32,BATTERY,raw_mV=%u,filtered_mV=%u,offset_mV=%d,corrected_mV=%u,linear_pct=%u,spread_mV=%u,core_spread_mV=%u,valid=%u,consensus_windows=%u,unstable_windows=%u\n",
    (unsigned)batteryMeter.rawMillivolts,
    (unsigned)batteryMeter.filteredMillivolts,
    (int)batteryOffsetMv,(unsigned)correctedBatteryMv(),
    (unsigned)SqeBatteryCalibration::linearPercent(correctedBatteryMv()),
    (unsigned)batteryMeter.sampleSpreadMillivolts,
    (unsigned)batteryMeter.coreSpreadMillivolts,
    valid?1u:0u,
    (unsigned)batteryMeter.windowCount,(unsigned)batteryMeter.unstableWindows);
#else
  (void)now;
#endif
}
int batteryDisplayTick(){
  if(batteryPercent<0||batteryMillivolts<2800u||batteryMillivolts>4350u)return -1;
  const uint16_t v=correctedBatteryMv();
  return batteryDisplayVolts?(int)SqeBatteryCalibration::centiVolts(v):
         (int)SqeBatteryCalibration::linearPercent(v);
}
void drawBatteryStatus(int x,int y){
  const bool valid=batteryPercent>=0 && batteryMillivolts>=2800u &&
                   batteryMillivolts<=4350u;
  const uint16_t calibrated=correctedBatteryMv();
  const uint8_t percent=SqeBatteryCalibration::linearPercent(calibrated);
  const uint16_t col=!valid?kMuted:calibrated<=3500u?kPink:kWhite;
  char value[8];
  if(!valid)snprintf(value,sizeof(value),batteryDisplayVolts?"--.--V":"--%%");
  else if(batteryDisplayVolts){
    // Round the calibrated battery voltage to the nearest 0.01V, not 0.05V.
    const unsigned shownCv=SqeBatteryCalibration::centiVolts(calibrated);
    snprintf(value,sizeof(value),"%u.%02uV",
             shownCv/100u,shownCv%100u);
  }else snprintf(value,sizeof(value),"~%u%%",(unsigned)percent);
  // Reset font before measuring to keep the title/status aligned with sprite.
  lcd.setTextFont(1);lcd.setTextSize(1);
  label(x-4-lcd.textWidth(value),y-4,value,col);
  lcd.fillRect(x+1,y-5,15,10,kTaskbar);
  if(valid){
    const int fill=(int)((unsigned)percent*13u/100u);
    const uint16_t levelCol=calibrated<=3500u?kPink:kGreen;
    if(fill>0)lcd.fillRect(x+2,y-4,fill,8,levelCol);
  }
  lcd.drawRect(x,y-6,17,12,kWhite);
  lcd.fillRect(x+17,y-3,2,6,kWhite);
}
// Small symmetric broadcasting radio glyph in the header (19 x 13).
// Central signal dot + paired bracket waves: legible at 320x240 and 240x320.
void drawTxBeacon(int cx,int cy,bool transmitting){
  const uint16_t col=transmitting?kGreen:kCyan;
  lcd.fillCircle(cx,cy,2,col);
  // Inner propagation marks.
  lcd.drawLine(cx-5,cy-4,cx-7,cy-1,col);
  lcd.drawFastVLine(cx-7,cy-1,3,col);
  lcd.drawLine(cx-7,cy+2,cx-5,cy+4,col);
  lcd.drawLine(cx+5,cy-4,cx+7,cy-1,col);
  lcd.drawFastVLine(cx+7,cy-1,3,col);
  lcd.drawLine(cx+7,cy+2,cx+5,cy+4,col);
  // Outer waves. Short enough to stay inside the top status strip.
  lcd.drawLine(cx-9,cy-5,cx-11,cy-2,col);
  lcd.drawFastVLine(cx-11,cy-2,5,col);
  lcd.drawLine(cx-11,cy+3,cx-9,cy+5,col);
  lcd.drawLine(cx+9,cy-5,cx+11,cy-2,col);
  lcd.drawFastVLine(cx+11,cy-2,5,col);
  lcd.drawLine(cx+11,cy+3,cx+9,cy+5,col);
  if(transmitting)lcd.fillCircle(cx,cy,1,kWhite);
}
void titleBar(){
  for(int x=0;x<kW;++x){
    const uint16_t c=x<160?blend(kPurple,kCyan,(uint8_t)(x*255/160)):
                         blend(kCyan,kPink,(uint8_t)((x-160)*255/160));
    lcd.drawFastVLine(x,0,14,c);
  }
  lcd.drawFastHLine(0,14,kW,kPurple);
  // Black hamburger tile; the full 48x23 top-left area is tappable.
  lcd.fillRoundRect(5,1,27,13,3,kWhite);
  for(int y=4;y<=10;y+=3)lcd.drawFastHLine(11,y,15,kInk);
  // The ST7798 frame sprite sometimes inherits a 2x text size from the
  // preceding animation. Measure in the same font and size used to print.
  // The fixed GLCD font 1 has 6-pixel pitch per ASCII glyph.
  static constexpr char kAppTitle[]="SQUACHEMIT";
  static constexpr int kTitleGlyphW=6;
  const int nameW=(int)(sizeof(kAppTitle)-1)*kTitleGlyphW;
  const int nameX=(kW-nameW)/2;
  lcd.setTextFont(1);
  lcd.setTextSize(1);
  lcd.fillRect(nameX-4,1,nameW+8,12,kTaskbar);
  label(nameX,3,kAppTitle,kWhite);
  // R14.14: compact radio beacon, not the old triangle-and-mast glyph.
  // Muted at idle, vivid green only while the transmitter is active.
#if defined(ARDUINO_ARCH_ESP32)
  const time_t epoch=time(nullptr);
  if(epoch>1704067200){
    struct tm current{};
    localtime_r(&epoch,&current);
    char stamp[20];strftime(stamp,sizeof(stamp),"%m/%d %H:%M",&current);
    label(43,3,stamp,kWhite);
  }else label(43,3,"--/-- --:--",kMuted);
#else
  label(43,3,"--/-- --:--",kMuted);
#endif
  drawBatteryStatus(259,7);
  drawTxBeacon(298,7,onAirIndicator);
  if(pcControl)label(313,3,"P",kCyan);
}
void applyTheme(){
  static const uint16_t palette[][3]={
    {0xAC1F,0x07FF,0xF96F}, // original vapor purple/cyan/pink
    {0x04D3,0x07FF,0x05F3}, // ocean teal/blue
    {0xB300,0xFE00,0xF920}  // sunset amber/orange
  };
  const uint8_t n=themeLevel<3?themeLevel:0;
  kPurple=palette[n][0];kCyan=palette[n][1];kPink=palette[n][2];
}
void applyBrightness(){
  const uint8_t levels[]={64,150,255};
  ledcWrite(6,screenDimmed?32:levels[brightnessLevel]);
}
void persistUiSettings(){
  if(prefs.begin("sqemit32",false)){
    prefs.putUChar("bright",brightnessLevel);
    prefs.putUChar("anim",animationLevel);
    prefs.putUChar("timeout",screenTimeout);
    prefs.putUChar("theme",themeLevel);
    prefs.putUChar("font",fontMode);
    prefs.end();
  }else Serial.println("WARN,SETTINGS_SAVE_UNAVAILABLE");
}
void button(int x,int y,int w,int h,const char* s,bool pressed=false){
  lcd.fillRect(x,y,w,h,pressed?kPurple:kBg);
  lcd.drawRect(x,y,w,h,kPurple);
  lcd.setTextFont(1);
  const int width=lcd.textWidth(s);
  label(x+(w-width)/2,y+(h-8)/2,s,pressed?kWhite:kCyan);
}
void footer(const char* a,const char* b,const char* c){
  button(8,kFooterY,96,kFooterH,a);
  if(b&&*b)button(112,kFooterY,96,kFooterH,b);
  if(c&&*c)button(216,kFooterY,96,kFooterH,c);
}
void pinnedBack(){
  // SquachWatch's settings-like subpages pin a full-width BACK strip.
  lcd.fillRect(0,211,kW,29,kBg);
  lcd.drawFastHLine(0,211,kW,kPurple);
  label(125,219,"[ BACK ]",kCyan,2);
}
void wedge(int x1,int y1,int x2,int y2,int r,uint16_t col){
  const int dx=x2-x1,dy=y2-y1;
  const int divisor=abs(dx)+abs(dy);
  if(divisor<1)return;
  const int nx=dy*r/divisor,ny=-dx*r/divisor;
  lcd.fillTriangle(x1+nx,y1+ny,x1-nx,y1-ny,x2+nx,y2+ny,col);
  lcd.fillTriangle(x2+nx,y2+ny,x2-nx,y2-ny,x1-nx,y1-ny,col);
  lcd.fillCircle(x1,y1,r,col);
  lcd.fillCircle(x2,y2,r,col);
}
#if defined(SQE_ORIGINAL_SQUACHY)
// These are the original upstream draw functions, not R10 lookalikes.
// SquachWatch's Theme.cpp is compiled as a shared GPL module on CYD32.
void drawOriginalScene(int top,int bottom){
  // The actual SquachWatch wrapper computes per-frame movement, and
  // suppresses it during band 2. Calling individual scenes bypassed this.
  static const DetectionEngine noReceiver;
  const Settings::Background bg = static_cast<Settings::Background>(
      backgroundLevel==9 ? 10 : backgroundLevel); // Synthwave is 10; 9=retired tunnel.
  Settings::previewBackground(bg); // RAM-only, SquachEmit persists bg separately.
  Theme::setBackgroundFloor(bottom);
  Theme::drawActiveBackground(lcd,frameNow(),top,bottom,noReceiver,advanceScene);
  Theme::clearBackgroundFloor();
}

#endif
void retroBackdrop(){
#if defined(SQE_ORIGINAL_SQUACHY)
  drawOriginalScene(15,181);
  return;
#endif
  if(backgroundLevel!=9){
    lcd.fillRect(0,15,320,166,lcd.color565(3,4,16));
    if(backgroundLevel==0){
      SquachWatchScenes::drawDigitalRain(lcd,millis(),15,181,advanceScene);
    }else if(backgroundLevel==1){ // SquachWatch Starfield motif
      for(int i=0;i<95;++i){
        const int x=(i*157+29)%320, y=17+(i*83+11)%162;
        lcd.drawPixel(x,y,i%9==0?kWhite:(i%3==0?kCyan:kMuted));
      }
    }else if(backgroundLevel==5){
      SquachWatchScenes::drawFireflies(lcd,millis(),15,181);
    }else if(backgroundLevel==6){ // FIRE: glowing embers and tongues
      lcd.fillRect(0,130,320,51,lcd.color565(49,6,2));
      for(int i=0;i<72;++i){
        const int x=(i*47+13)%320,y=50+(i*83+29)%120;
        const uint16_t glow=i%3==0?kYellow:i%3==1?kPink:lcd.color565(220,35,0);
        lcd.drawLine(x,180,x-((i%3)-1)*6,y,glow);
      }
    }else if(backgroundLevel==7){ // SNOWFALL: moon and drifting flakes
      lcd.fillCircle(275,43,18,kWhite);
      lcd.fillRect(0,152,320,29,lcd.color565(19,47,81));
      for(int i=0;i<100;++i)lcd.fillCircle((i*73+23)%320,
          18+(i*101+17)%151,i%7==0?2:1,kWhite);
    }else if(backgroundLevel==3){ // AQUARIUM: waves, plants and fish
      lcd.fillRect(0,15,320,166,lcd.color565(0,34,52));
      for(int y=25;y<180;y+=20)lcd.drawFastHLine(0,y,320,lcd.color565(0,65,80));
      for(int x=0;x<320;x+=42){
        lcd.drawLine(x,180,x+13,116,kGreen);
        lcd.drawLine(x+4,167,x+22,135,kCyan);
      }
      for(int i=0;i<7;++i){
        const int x=20+(i*49)%290,y=40+(i*37)%112;
        lcd.fillTriangle(x,y,x+17,y-7,x+17,y+7,i%2?kYellow:kPink);
        lcd.fillTriangle(x+18,y,x+25,y-6,x+25,y+6,kCyan);
      }
    }else if(backgroundLevel==4){
      SquachWatchScenes::drawTerminalLog(lcd,millis(),15,181);
    }else if(backgroundLevel==2){ // FLYING TOASTERS: procedural silhouettes
      for(int i=0;i<10;++i){
        const int x=12+(i*83)%287,y=28+(i*41)%127;
        lcd.fillRoundRect(x,y,21,19,4,kYellow);
        lcd.fillRect(x+4,y+4,13,8,lcd.color565(84,42,3));
        lcd.drawLine(x,y+7,x-9,y-4,kWhite);
        lcd.drawLine(x+21,y+7,x+30,y-4,kWhite);
      }
    }else if(backgroundLevel==8){ // THE GIBSON: circuit grid, dynamic activity
      for(int x=12;x<320;x+=22){
        lcd.drawFastVLine(x,18,163,x%3?kPurple:kCyan);
      }
      for(int y=24;y<178;y+=18)lcd.drawFastHLine(0,y,320,kPurple);
      for(int i=0;i<25;++i){
        const int x=(i*79+33)%320,y=22+(i*53)%145;
        lcd.fillRect(x,y,4,4,i%3?kGreen:kPink);
      }
    }
    return;
  }
  // Exact source from SquachWatch-CYD/src/theme.cpp (full-band adapter).
  SquachWatchScenes::drawSynthwave(lcd,millis(),15,181,0.44f);
  return;

}

void oldSquachy(bool emitting,bool suite){
  // Reduced Squachy silhouette taken from visual inspection of the
  // *actual SquachWatch renders* (hair tuft, black shades, pink cheeks,
  // square pink mouth, small ears, short legs). Independent artwork:
  // no copy of SquachWatch's GPLv3 sprite/character rendering functions.
  const int x=160,y=86;
  // Dark outline first, then shaggy body and short round feet.
  lcd.fillRoundRect(x-30,y+23,60,71,14,kInk);
  lcd.fillRoundRect(x-26,y+24,52,67,12,kFurMain);
  lcd.fillRoundRect(x-15,y+35,30,35,6,kFurDark);
  // Feet: they sit on the ground, not on top of the diagnostics strip.
  lcd.fillRoundRect(x-29,y+86,29,11,5,kInk);
  lcd.fillRoundRect(x+1,y+86,29,11,5,kInk);
  lcd.fillRoundRect(x-27,y+85,26,10,4,kFurMain);
  lcd.fillRoundRect(x+2,y+85,25,10,4,kFurMain);
  // Arms hang outside the torso, with dark keyline.
  lcd.fillRoundRect(x-40,y+26,20,58,7,kInk);
  lcd.fillRoundRect(x+20,y+26,20,58,7,kInk);
  lcd.fillRoundRect(x-36,y+28,14,53,6,kFurMain);
  lcd.fillRoundRect(x+22,y+28,14,53,6,kFurMain);
  lcd.fillCircle(x-30,y+81,7,kFurLight);
  lcd.fillCircle(x+30,y+81,7,kFurLight);
  // Uneven pointed hair, the source character's unmistakable crown.
  lcd.fillTriangle(x-28,y-13,x-16,y-38,x-3,y-11,kInk);
  lcd.fillTriangle(x-8,y-12,x+15,y-45,x+24,y-10,kInk);
  lcd.fillTriangle(x-25,y-12,x-14,y-33,x+0,y-10,kFurLight);
  lcd.fillTriangle(x-5,y-12,x+14,y-39,x+21,y-10,kFurLight);
  lcd.fillRoundRect(x-39,y-20,78,66,18,kInk);
  lcd.fillRoundRect(x-35,y-18,70,60,16,kFurLight);
  lcd.fillRoundRect(x-31,y-16,62,55,15,kFurMain);
  // Short round Sasquatch ears, with contrasting inner fur.
  lcd.fillCircle(x-35,y+7,10,kInk);
  lcd.fillCircle(x+35,y+7,10,kInk);
  lcd.fillCircle(x-35,y+7,7,kFurMain);
  lcd.fillCircle(x+35,y+7,7,kFurMain);
  lcd.fillCircle(x-35,y+7,3,kSkinDark);
  lcd.fillCircle(x+35,y+7,3,kSkinDark);
  // Eye ridge, signature squared dark sunglasses with blue lens gleam.
  lcd.fillRoundRect(x-27,y-2,54,21,5,kInk);
  lcd.fillRoundRect(x-24,y-1,22,17,3,lcd.color565(8,19,52));
  lcd.fillRoundRect(x+2,y-1,22,17,3,lcd.color565(8,19,52));
  lcd.fillRect(x-17,y+2,4,7,lcd.color565(37,113,172));
  lcd.fillRect(x+9,y+2,4,7,lcd.color565(37,113,172));
  lcd.drawFastHLine(x-3,y+7,6,kFurDark);
  // Pink cheeks, outlined muzzle, bright rectangular expressive mouth.
  lcd.fillCircle(x-29,y+22,7,kVaporPink);
  lcd.fillCircle(x+29,y+22,7,kVaporPink);
  lcd.fillRoundRect(x-25,y+25,50,20,6,kInk);
  lcd.fillRoundRect(x-21,y+27,42,15,5,emitting?kPink:kSkin);
  lcd.fillRect(x-15,y+28,30,4,kWhite);
  if(suite)lcd.drawFastHLine(x-13,y+36,26,kInk);
  else if(emitting){
    lcd.fillRect(x-9,y+34,18,6,kInk);
    lcd.fillRect(x-7,y+33,14,2,kVaporPink);
  }else{
    lcd.drawLine(x-10,y+35,x,y+39,kInk);
    lcd.drawLine(x,y+39,x+10,y+35,kInk);
  }
}

void drawSquachyFrame(bool emitting,bool suite,uint32_t now){
#if defined(SQE_ORIGINAL_SQUACHY)
  (void)emitting;(void)suite;
  // R14.18: ALL outfits use SquachWatch's original animated body, including
  // SquachEmit's black hoodie (1) and cyan lab jacket (2). Never render
  // those two through the separate color-keyed scratch sprite: with RGB332
  // backing that produced the solid pink rectangle seen on physical CYD32.
  const int8_t original=(outfitLevel<=2)?0:(int8_t)(outfitLevel-2);
  Squachy::setOutfitPreview(original);
  Squachy::setSqemitCustomLook(outfitLevel==1?1:outfitLevel==2?2:0);
  // Keep SquachEmit's own centered speech bubble and the already validated
  // SquachWatch body coordinates/animations for every selected outfit.
  Squachy::holdBubble(true);
  Squachy::tick(lcd,160,65,116,now,advanceScene,0.3f,false,0,100);
  Squachy::setSqemitCustomLook(0);
  Squachy::setOutfitPreview(-1);
#else
  // Legacy reduced-art fallback for builds that explicitly opt out of the
  // original engine. Production CYD 3.2 always defines SQE_ORIGINAL_SQUACHY.
  if(!squachSpriteReady)return;
  SquachyPort::render(squachSprite,now,emitting,suite,
                     (int32_t)(petUntil-now)>0,outfitLevel);
  if(bandFrameActive){
    squachSprite.pushToSprite(&halfFrame,104,70,0xF81E);
  }else squachSprite.pushSprite(104,70,0xF81E);
#endif
  lastSquachFrame=now;
}
void bigSquachy(bool emitting,bool suite){
#if defined(SQE_ORIGINAL_SQUACHY)
  // Every original + custom style goes through the same transparent
  // procedural renderer; never composite the reduced sprite onto LCD.
  drawSquachyFrame(emitting,suite,frameNow());
#else
  if(squachSpriteReady)drawSquachyFrame(emitting,suite,frameNow());
  else oldSquachy(emitting,suite);
#endif
}
// Right-side status is ONE label on ONE baseline.  During an active test,
// countdown REPLACES the case total; never render both in the same 15px band.
void rightStatus(bool emitting,uint32_t count){
  constexpr int kRightX=236,kRightY=183,kRightW=kW-kRightX,kRightH=15;
  lcd.fillRect(kRightX,kRightY,kRightW,kRightH,kBg);
  char text[24];
  if(emitting)snprintf(text,sizeof(text),"%lus LEFT",(unsigned long)count);
  else snprintf(text,sizeof(text),"%u CASES",(unsigned)kScenarioCount);
  lcd.setTextFont(1);
  lcd.setTextSize(1);
  const int textX=kW-12-lcd.textWidth(text);
  // Both labels share y=186.  No glyph touches the separator at y=198.
  label(textX,186,text,emitting?kPink:kVaporPink);
}
void countdownOnly(uint32_t count){
  // Only redraw the right status slot once per second, preserving mascot FPS.
  rightStatus(true,count);
}

// Strict speech state gating: no RF means IDLE only, including suite
// cooldowns, Wi-Fi startup and failed starts. The RF driver's active
// scenario ID (not the highlighted menu selection) governs TX dialogue.
SqeBubble::Kind bubbleContext(const char* active,bool suite){
  (void)suite;
  return active&&*active?SqeBubble::OnAir:SqeBubble::Idle;
}
long speechRandom(long lo,long hi){return random(lo,hi);}
SqeQuips::Topic categoryTopic(const Scenario* sc){
  if(!sc)return SqeQuips::Topic::Idle;
  switch(deviceCategory(*sc)){
    // New v1.50 category pages use generic speech until their own
    // dedicated banks are validated. Never show an unrelated type quip.
    case DeviceCategory::Alpr:
    case DeviceCategory::Cameras:
    case DeviceCategory::Listeners:
    case DeviceCategory::Ring:
      return SqeQuips::Topic::General;
    case DeviceCategory::Axon:return SqeQuips::Topic::Axon;
    case DeviceCategory::Drones:return SqeQuips::Topic::Drones;
    case DeviceCategory::Flock:return SqeQuips::Topic::Flock;
    case DeviceCategory::General:return SqeQuips::Topic::General;
    case DeviceCategory::Hacker:return SqeQuips::Topic::Hacker;
    case DeviceCategory::IBeacon:return SqeQuips::Topic::IBeacon;
    case DeviceCategory::Raven:return SqeQuips::Topic::Raven;
    case DeviceCategory::SerialModules:return SqeQuips::Topic::SerialModules;
    case DeviceCategory::SmartGlasses:return SqeQuips::Topic::SmartGlasses;
    case DeviceCategory::Trackers:return SqeQuips::Topic::Trackers;
    default:return SqeQuips::Topic::General;
  }
}
// Only the currently transmitted scenario's actual top-level category
// or TRANSMITTING may appear on air. When off air, only IDLE is legal.
// The optional carrier, rare, Easter egg, event and suite/playlist banks
// remain in the catalog but are not selected in this strict mode.
void selectLandscapeSpeech(uint32_t now,const char* active,bool suite){
  (void)now;(void)suite;
  const bool onAir=active&&*active;
  const Scenario* transmitting=onAir?findScenario(active):nullptr;
  const SqeQuips::Topic topic=SqeQuips::stateTopic(
      onAir,categoryTopic(transmitting),(uint32_t)random(0,100));
  landscapeSpeechText=SqeQuips::choose(topic,(uint32_t)random(0,32767));
}
void advanceLandscapeBubble(uint32_t now,const char* active,bool suite){
  // Suite cases can change without an intervening UI idle tick. Never
  // carry a bubble from the previous on-air category into the new case.
  const char* activeId=active&&*active?active:"";
  if(strcmp(speechActiveCase,activeId)!=0){
    snprintf(speechActiveCase,sizeof(speechActiveCase),"%s",activeId);
    if(activeId[0]&&landscapeBubble.context==(uint8_t)SqeBubble::OnAir)
      landscapeBubble.context=255; // reannounce new active RF case
  }
  const bool changed=SqeBubble::update(landscapeBubble,now,
      bubbleContext(active,suite),speechRandom);
  // Pick ONCE per popup, never on each LCD frame or sprite band.
  if(changed&&landscapeBubble.showing)
    selectLandscapeSpeech(now,active,suite);
  // FULL8/HALF8 redraws happen via renderBufferedHome(). Static fallback
  // must repaint the backdrop to erase a disappearing speech bubble.
  if(changed&&!portraitMode&&screen==Screen::Home&&!bandFrameReady)
    redraw=true;
}
const char* speechLine(){return landscapeSpeechText;}
// Preserve the rendered text width after truncation: this is what we use
// for BOTH the bubble's width and exact horizontal centering.
void bubbleLine(const char* source,char* dest,size_t cap,int font,int maxPx){
  if(!cap)return;
  dest[0]='\0';
  if(!source)return;
  strncpy(dest,source,cap-1);
  dest[cap-1]='\0';
  lcd.setTextFont(font);
  lcd.setTextSize(1);
  while(dest[0]&&lcd.textWidth(dest)>maxPx)
    dest[strlen(dest)-1]='\0';
}
// Preserve full long joke wording on a small ST7798 by wrapping at a
// whole-word boundary. Only use second line when a single font-2 line
// exceeds the available 282 px. No scrolling or long redraw work.
void speechWrap(const char* phrase,char first[64],char second[64],int maxPx){
  second[0]='\0';
  if(!phrase||!phrase[0])return;
  lcd.setTextFont(2);lcd.setTextSize(1);
  if(lcd.textWidth(phrase)<=maxPx)return; // bubbleLine() already copied it
  char whole[64];
  strncpy(whole,phrase,sizeof(whole)-1);whole[sizeof(whole)-1]='\0';
  int best=-1,bestScore=0x7fffffff;
  for(size_t i=1;i<strlen(whole);++i){
    if(whole[i]!=' ')continue;
    const char saved=whole[i];whole[i]='\0';
    const int w1=lcd.textWidth(whole);
    const int w2=lcd.textWidth(whole+i+1);
    whole[i]=saved;
    if(w1>maxPx||w2>maxPx)continue;
    const int wider=w1>w2?w1:w2;
    const int difference=w1>w2?w1-w2:w2-w1;
    const int score=wider*4+difference;
    if(score<bestScore){bestScore=score;best=(int)i;}
  }
  if(best>=0){
    whole[best]='\0';
    snprintf(first,64,"%s",whole);
    snprintf(second,64,"%s",whole+best+1);
  }
}
void speechBubble(const char* active,bool suite){
  (void)active;(void)suite;
  const uint32_t now=frameNow();
  if(!SqeBubble::visible(landscapeBubble,now))return;
  const bool failed=landscapeBubble.context==(uint8_t)SqeBubble::Failed;
  char primary[64],detail[32];
  bubbleLine(speechLine(),primary,sizeof(primary),2,282);
  bubbleLine(failed?radioStartError:"",detail,sizeof(detail),1,282);
  if(!failed)speechWrap(speechLine(),primary,detail,282);
  const bool hasDetail=detail[0]!='\0';
  lcd.setTextFont(2);
  const int primaryW=lcd.textWidth(primary);
  lcd.setTextFont(failed?1:2);
  const int detailW=hasDetail?lcd.textWidth(detail):0;
  const float scale=SqeBubble::popScale(landscapeBubble,now);
  const SqeBubbleLayout::Rect box=SqeBubbleLayout::bounds(primaryW,detailW,hasDetail,scale);
  lcd.fillRoundRect(box.x,box.y,box.w,box.h,5,kBg);
  lcd.drawRoundRect(box.x,box.y,box.w,box.h,5,kVaporPink);
  if(box.w>14&&box.h>14)
    lcd.drawRoundRect(box.x+2,box.y+2,box.w-4,box.h-4,4,kPurple);
  if(scale<0.999f)return; // text appears only after pop-in; no clipping
  // Truly centered, no fixed left margin. Normal quips have a short,
  // content-hugging one-line bubble; failed-start details get two lines.
  label(SqeBubbleLayout::centeredTextX(box,primaryW),
        hasDetail?box.y+5:box.y+(box.h-16)/2,
        primary,failed?kPink:kWhite,2);
  if(hasDetail)
    label(SqeBubbleLayout::centeredTextX(box,detailW),
          box.y+(failed?27:24),detail,failed?kYellow:kWhite,
          failed?1:2);
}
void statusStrip(const char* active,bool suite){
  lcd.fillRect(0,183,320,29,kBg);
  lcd.drawFastHLine(6,182,308,kPurple);
  const bool starting=!(active&&*active) && radioStartingProfile[0];
  label(12,186,active&&*active?"ON AIR":starting?"STARTING":
        playlistRunning?(playlistPaused?"PAUSED":"PLAYLIST"):suite?"SUITE":"ACTIVITY >",
        active&&*active?kPink:(starting?kYellow:kGreen));
  // Time control is an explicit tap target; it does not overlap the countdown.
  lcd.drawRect(93,183,137,15,kPurple);
  label(99,186,"TX TIME",kCyan);
  char timeText[20];
  snprintf(timeText,sizeof(timeText),"%us [SET]",(unsigned)durationSeconds);
  label(150,186,timeText,kYellow);
  if(!(active&&*active))rightStatus(false,0);
  lcd.drawFastHLine(6,198,308,kPurple);
  label(12,201,"PROFILE",kVaporPink);
  const Scenario* chosen=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  char friendly[58]={0};
  if(chosen)scenarioDeviceName(*chosen,friendly,sizeof(friendly));
  fit(66,200,chosen?friendly:"NONE",164,kWhite,1);
  label(245,201,"DETAILS >",kCyan);
}

void durationValue(){
  // Adjust only this number's rectangle. Full-screen repaints on +/- taps
  // caused the ST7798 to visibly flash; the controls and backdrop stay put.
  lcd.fillRect(96,82,128,65,kBg);
  char value[20];
  snprintf(value,sizeof(value),"%u",(unsigned)durationSeconds);
  lcd.setTextFont(2);
  label((kW-lcd.textWidth(value))/2,95,value,kWhite,2);
  label(139,126,"SECONDS",kMuted);
}
void durationScreen(){
  lcd.fillScreen(kBg);
  titleBar();
  label(20,27,"TRANSMIT TIME",kVaporPink,2);
  label(22,57,"FOR EACH RUN AND SUITE CASE",kCyan);
  button(12,85,83,54,"- 1 SEC");
  button(225,85,83,54,"+ 1 SEC");
  durationValue();
  button(12,155,92,29,"3 SEC");
  button(114,155,92,29,"10 SEC");
  button(216,155,92,29,"30 SEC");
  label(26,191,"SAVED FOR THE NEXT TEST",kMuted);
  footer("BACK","RESET 12","DONE");
}
void setDuration(uint8_t value){
  if(value<3 || value>30 || durationSeconds==value)return;
  durationSeconds=value;
  if(prefs.begin("sqemit32",false)){
    prefs.putUChar("txtime",durationSeconds);
    prefs.end();
  }
  Serial.printf("CYD32,TX_TIME,%u\n",(unsigned)durationSeconds);
  durationValue();
}

void portraitPage(const char* active,uint32_t remaining,bool suite);
void homeScreen(const char* active,uint32_t remaining,bool suite);
// One complete logical frame, assembled as two memory-bounded 8-bit
// bands exactly like SquachWatch's CYD recovery renderer. The TFT sees
// only a finished sprite per band, not erase/redraw primitives.
void renderBufferedHome(const char* active,uint32_t remaining,bool suite){
  if(!bandFrameReady)return;
  const int w=panel.width(),h=panel.height();
  const int bands=sceneFrameBands,bandH=h/bands;
  const uint32_t frameMs=millis();
  const uint32_t begunUs=micros();
  uint32_t drawUs=0,pushUs=0;
  logicalFrameMs=frameMs;
  Cyd32FramePush::newFrame();
#if defined(SQE_ORIGINAL_SQUACHY)
  DrawBand::setEnabled(true);
#endif
  for(int pass=0;pass<bands;++pass){
    bandFrameTop=pass*bandH;
    advanceScene=(pass==0);
#if defined(SQE_ORIGINAL_SQUACHY)
    DrawBand::set(bandFrameTop,bandFrameTop+bandH);
#endif
    paint=&halfFrame;
    bandFrameActive=true;
    halfFrame.setViewport(0,-bandFrameTop,w,h,true);
    const uint32_t drawStart=micros();
    if(portraitMode)portraitPage(active,remaining,suite);
    else homeScreen(active,remaining,suite);
    drawUs+=micros()-drawStart;
    paint=&panel;
    bandFrameActive=false;
    halfFrame.resetViewport();
    const uint32_t pushStart=micros();
    // Fast transfer is limited to the proven FULL8 one-pass mode.
    // In HALF8, keep R14.6's normal push and explicit cache invalidation.
    const bool useFast=(bands==1)&&fastPushDesired&&Cyd32FramePush::enabled();
    if(useFast){
      if(!Cyd32FramePush::push(panel,halfFrame.buf(),w,bandH,0,bandFrameTop)){
        ++fastPushFallbacks;
        fastPushDesired=false;
        Cyd32FramePush::setEnabled(false);
        Cyd32FramePush::invalidate();
        halfFrame.pushSprite(0,bandFrameTop);
        Serial.println("CYD32,PUSH,FALLBACK,INVALID_FRAME_OR_DRIVER");
        fastPushLastUsed=false;
      }else fastPushLastUsed=true;
    }else{
      Cyd32FramePush::invalidate();
      halfFrame.pushSprite(0,bandFrameTop);
      fastPushLastUsed=false;
    }
    pushUs+=micros()-pushStart;
  }
#if defined(SQE_ORIGINAL_SQUACHY)
  DrawBand::setEnabled(false);
#endif
  lastSceneFrame=frameMs;
  fastPushLastRows=(uint32_t)Cyd32FramePush::lastRows();
  fastPushLastHashUs=Cyd32FramePush::hashUs();
  fastPushLastWireUs=Cyd32FramePush::wireUs();
  // Report real observed performance once per five seconds, not once
  // per frame: distinguish CPU render bottleneck from SPI transfer.
  static uint32_t lastReportMs=0,frames=0,sumDrawUs=0,sumPushUs=0;
  static uint32_t sumRows=0,sumHashUs=0,sumWireUs=0;
  ++frames;
  sumDrawUs+=drawUs;
  sumPushUs+=pushUs;
  sumRows+=fastPushLastRows;
  sumHashUs+=fastPushLastHashUs;
  sumWireUs+=fastPushLastWireUs;
  if(frameMs-lastReportMs>=5000u){
    const uint32_t period=lastReportMs?frameMs-lastReportMs:5000u;
    // Keep original fields intact for serial tools. Extras support PUSH
    // ON/OFF side-by-side comparisons on the exact same physical panel.
    Serial.printf("CYD32,FRAME,PERF,fps=%lu,draw_us=%lu,push_us=%lu,frame_us=%lu,heap=%lu,push_mode=%s,rows=%lu,hash_us=%lu,wire_us=%lu\n",
      (unsigned long)((frames*1000UL)/period),
      (unsigned long)(sumDrawUs/frames),(unsigned long)(sumPushUs/frames),
      (unsigned long)(micros()-begunUs),(unsigned long)ESP.getFreeHeap(),
      fastPushLastUsed?"FAST":"STANDARD",
      (unsigned long)(sumRows/frames),(unsigned long)(sumHashUs/frames),
      (unsigned long)(sumWireUs/frames));
    lastReportMs=frameMs;frames=0;sumDrawUs=0;sumPushUs=0;
    sumRows=0;sumHashUs=0;sumWireUs=0;
  }
}
// Render the entire RGB332 scene ONCE when internal RAM permits. A
// 320x240 buffer is 76,800 bytes. Reserve heap for RF and costumes;
// fall back to the previous two 38.4KB passes on constrained devices.
void allocateSceneFrame(){
  paint=&panel;bandFrameActive=false;
  // A new buffer, panel rotation or direct painting invalidates row hashes.
  Cyd32FramePush::invalidate();
  if(bandFrameReady){halfFrame.deleteSprite();bandFrameReady=false;}
  halfFrame.setColorDepth(8);
  const int w=panel.width(),h=panel.height();
  sceneFrameBands=2;
  const bool fullCreated=halfFrame.createSprite(w,h)!=nullptr;
  const uint32_t afterFull=ESP.getFreeHeap();
  const uint32_t largestFull=ESP.getMaxAllocHeap();
  const bool fullSafe=fullCreated &&
      afterFull>=64u*1024u && largestFull>=24u*1024u;
  if(fullSafe){
    bandFrameReady=true;
    sceneFrameBands=1;
  }else{
    if(fullCreated)halfFrame.deleteSprite();
    bandFrameReady=halfFrame.createSprite(w,h/2)!=nullptr;
  }
  Serial.printf("CYD32,FRAME,%s,%dx%d,bands=%u,heap=%lu,largest=%lu,fullCandidate=%u,fullFree=%lu,fullLargest=%lu\n",
    bandFrameReady?(sceneFrameBands==1?"FULL8":"HALF8"):"STATIC_FALLBACK",
    w,h/sceneFrameBands,(unsigned)sceneFrameBands,
    (unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getMaxAllocHeap(),
    (unsigned)fullCreated,(unsigned long)afterFull,(unsigned long)largestFull);
}
void homeScreen(const char* active,uint32_t remaining,bool suite){
  const bool emitting=active&&*active;
  lcd.fillScreen(kBg);
  retroBackdrop();
  bigSquachy(emitting,suite);
  titleBar();
  speechBubble(active,suite);
  // Status is in the bottom strip: no huge headline over the mascot.
  statusStrip(active,suite);
  if(emitting){
    countdownOnly((remaining+999)/1000);
    lcd.fillRect(236,163,77,15,kBg);
    lcd.drawRect(239,167,70,7,kPurple);
    const uint32_t duration=actualTxDuration?actualTxDuration:remaining;
    const uint32_t elapsed=duration>remaining?duration-remaining:0;
    const int w=duration?int((elapsed*68ULL)/duration):0;
    if(w>0)lcd.fillRect(240,168,w,5,kGreen);
  }
  footer("TESTS",(emitting||playlistRunning||suite)?"STOP":"RUN",
         playlistRunning?(playlistPaused?"RESUME":"PAUSE"):"SUITE");
}


void detailScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(14,25,"SELECTED TEST PROFILE",kVaporPink,2);
  const Scenario* c=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  char friendly[58]={0};
  if(c)scenarioDeviceName(*c,friendly,sizeof(friendly));
  fit(14,60,c?friendly:"NO PROFILE",292,kWhite,2);
  fit(14,96,c?c->id:"NONE",292,kCyan);
  fit(14,121,c?c->expected:"NONE",292,kGreen);
  const bool wifi=c&&(c->kind==Kind::WifiAp||c->kind==Kind::WifiRemoteId);
  char detail[64];
  snprintf(detail,sizeof(detail),"RADIO: %s   CHANNEL: %u",wifi?"WIFI":"BLE",
           wifi?(unsigned)(c->code?c->code:1u):0u);
  label(14,146,detail,kMuted);
  snprintf(detail,sizeof(detail),"DURATION: %us   SYNTHETIC TEST",(unsigned)durationSeconds);
  label(14,174,detail,kYellow);
  const bool fav=c&&((favBits[selected/8]>>(selected%8))&1u);
  label(14,196,fav?"FAVORITE: YES [TAP TO REMOVE]":"FAVORITE: NO [TAP TO ADD]",kMuted);
  label(246,22,"FAVS >",kCyan);
  label(246,44,"HIST >",kCyan);
  footer("BACK","FAVORITES","RUN");
}
void diagValues(){
  lcd.fillRect(8,61,304,144,kBg);
  char value[72];
  snprintf(value,sizeof(value),"FREE HEAP: %lu B",(unsigned long)ESP.getFreeHeap());
  label(14,67,value,kCyan,2);
  snprintf(value,sizeof(value),"LARGEST: %lu B",(unsigned long)ESP.getMaxAllocHeap());
  label(14,93,value,kWhite);
  snprintf(value,sizeof(value),"UPTIME: %lu s",(unsigned long)(millis()/1000UL));
  label(14,116,value,kWhite);
  snprintf(value,sizeof(value),"SERIAL TX SPACE: %u B",(unsigned)Serial.availableForWrite());
  label(14,139,value,kWhite);
  label(14,162,previousActive[0]?"RADIO: ON AIR":"RADIO: OFF / IDLE",
        previousActive[0]?kPink:kGreen);
  fit(14,184,radioStartError[0]?radioStartError:"NO START ERROR",286,kYellow);
}
void diagnosticsScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(14,25,"DEVICE DIAGNOSTICS",kVaporPink,2);
  diagValues();lastDiagAt=millis();
  footer("MENU","REFRESH","HOME");
}


void menuRow(int y,const char* title,const char* sub,bool chosen=false);

void activityScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(12,24,"TRANSMITTER ACTIVITY",kVaporPink,2);
  char line[75];
  label(12,52,txDriverActive?"RADIO DRIVER: ACTIVE":"RADIO DRIVER: IDLE",txDriverActive?kPink:kGreen);
  snprintf(line,sizeof(line),"STARTS: %lu  ENDS: %lu",(unsigned long)txBegins,(unsigned long)txEnds);
  label(12,85,line,kCyan);
  snprintf(line,sizeof(line),"START/END ERRORS: %lu",(unsigned long)txErrors);
  label(12,111,line,kYellow);
  snprintf(line,sizeof(line),"LAST DURATION: %lu MS",(unsigned long)actualTxDuration);
  label(12,136,line,kWhite);
  label(12,160,"Driver events, NOT RF power",kMuted);
  label(12,187,"See PC LOG for RX detection",kMuted);
  footer("BACK","EVIDENCE","DETAILS");
}
void evidenceScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(12,24,"RECEIVER EVIDENCE",kVaporPink,2);
  label(12,55,"PC-REPORTED - NOT SELF-VERIFIED",kYellow);
  if(!rxId[0]){
    label(12,100,"NO RECEIVER RESULT RECEIVED",kMuted);
    label(12,133,"Connect Windows Workbench",kWhite);
  }else{
    fit(12,86,rxId,290,kWhite,2);
    fit(12,111,rxLabel,290,kCyan);
    char line[60];
    snprintf(line,sizeof(line),"RSSI: %d  |  %s",rxRssi,rxQuality);
    label(12,138,line,kGreen);
    label(12,165,"Source: Windows exact-MAC LOG",kMuted);
  }
  label(12,193,"No direct RF measurement on board",kMuted);
  footer("BACK","DETAILS","HOME");
}

void portraitCounter(const char* active,uint32_t remaining,bool suite){
  const bool on=active&&*active;
  lcd.fillRect(12,178,216,56,kBg);
  lcd.drawRect(12,178,216,56,kPurple);
  label(24,184,on?"RADIO ON AIR":suite?"SUITE RUNNING":"READY - NO RF",on?kPink:kGreen,2);
  char counter[38];
  snprintf(counter,sizeof(counter),on?"%lus LEFT":"TX TIME %us",
           (unsigned long)(on?(remaining+999)/1000:durationSeconds));
  label(26,210,counter,kYellow,2);
}
void portraitScreen(const char* active,uint32_t remaining,bool suite){
  lcd.fillScreen(kBg);
  lcd.drawRect(0,0,240,320,kPurple);
  label(56,12,"SQUACHEMIT",kWhite,2);
  label(40,45,"PORTRAIT LAB MODE",kVaporPink);
  label(14,78,"SELECTED TEST",kCyan);
  const Scenario* caseToShow=(active&&*active)?findScenario(active):
    selected<kScenarioCount?&kScenarios[selected]:nullptr;
  char labelText[58]={0};
  if(caseToShow)scenarioDeviceName(*caseToShow,labelText,sizeof(labelText));
  fit(14,103,caseToShow?labelText:"NONE",210,kWhite,2);
  fit(14,131,caseToShow?caseToShow->id:"NONE",211,kMuted);
  portraitCounter(active,remaining,suite);
  label(14,248,"ROTATE BACK FOR FULL LIBRARY",kMuted);
  button(8,278,72,34,"BACK");
  button(84,278,72,34,(active&&*active)||suite||playlistRunning?"STOP":"RUN");
  button(160,278,72,34,playlistRunning?(playlistPaused?"RESUME":"PAUSE"):"SUITE");
  label(15,264,"TOUCH CONTROLS ENABLED",kGreen);
}
bool loadCal();
// Same XPT2046 shared-SPI pressure-gated raw reader as SquachWatch CYD32.
// Both calibration and normal portrait taps must use this exact reader.
bool readPortraitRaw(int16_t& a,int16_t& b){
  if(panel.getTouchRawZ()<350)return false;
  uint16_t rx=0,ry=0;
  panel.getTouchRaw(&rx,&ry);
  a=(int16_t)rx; b=(int16_t)ry;
  return true;
}
bool portraitFitPlausible(const TouchFit::Fit& fit){
  if(fit.w0!=240||fit.h0!=320)return false;
  const float v[6]={fit.xa,fit.xb,fit.xc,fit.ya,fit.yb,fit.yc};
  for(float x:v)if(!isfinite(x))return false;
  float a=0,b=0;
  return TouchFit::toRaw(fit,120,160,a,b) && isfinite(a)&&isfinite(b);
}
bool loadPortraitFit(){
  TouchFit::Fit f{};
  bool found=false;
  if(prefs.begin("sqemitfit",true)){
    found=prefs.getUChar("v",0)==1u&&
      prefs.getBytesLength("fit")==sizeof(f)&&
      prefs.getBytes("fit",&f,sizeof(f))==sizeof(f);
    prefs.end();
  }
  if(found&&portraitFitPlausible(f)){portraitFit=f;return true;}
  if(found)Serial.println("CYD32,TOUCH,INVALID_AFFINE_FIT");
  return false;
}
bool runPortraitFivePointWizard(){
  paint=&panel;bandFrameActive=false;
  Cyd32FramePush::invalidate();
  Serial.println("CYD32,TOUCH,5_POINT_START;RELEASE_AND_FOLLOW_TARGETS");
  TouchFit::Fit proposed{};
  const auto result=SqeTouchWizard::runInteractive(panel,readPortraitRaw,0,
      nullptr,800,kBg,kWhite,kCyan,proposed);
  if(result!=SqeTouchWizard::Outcome::SAVED||!portraitFitPlausible(proposed)){
    Serial.printf("CYD32,TOUCH,5_POINT_NOT_SAVED,result=%u\n",(unsigned)result);
    wasPressed=true;lastPress=millis();redraw=true;
    return false;
  }
  if(!prefs.begin("sqemitfit",false)){
    Serial.println("ERROR,CYD32_TOUCH_NVS_OPEN");
    return false;
  }
  const bool saved=prefs.putBytes("fit",&proposed,sizeof(proposed))==sizeof(proposed)&&
      prefs.putUChar("v",1)==1;
  prefs.end();
  if(!saved){Serial.println("ERROR,CYD32_TOUCH_NVS_SAVE");return false;}
  portraitFit=proposed;
  portraitFitLoaded=true;
  portraitCalibrated=true;
  portraitRawHoldStarted=0;
  wasPressed=true;lastPress=millis();redraw=true;
  Serial.println("CYD32,TOUCH,5_POINT_VERIFIED_SAVED");
  return true;
}
// A stored blob with the expected length is not necessarily a usable fit.
// Reject erased/corrupt 10-byte records rather than leaving portrait touch
// completely unusable until somebody connects a serial console.
bool plausiblePortraitTouchFit(const uint16_t d[5]){
  return d[0]<4096u&&d[2]<4096u&&
    d[1]>=300u&&d[1]<=4095u&&d[3]>=300u&&d[3]<=4095u&&
    (d[4]&~7u)==0u;
}
void rotateCurrentScreen(){
  const bool makePortrait=!portraitMode;
  // R14.12 uses SquachWatch's five inset targets and verification, not
  // TFT_eSPI's corner routine (which may consume the previous press).
  // Do not trust the old 10-byte touchport blob in this code path.
  if(makePortrait){
    portraitFitLoaded=loadPortraitFit();
    portraitCalibrated=portraitFitLoaded;
    if((!portraitFitLoaded||forcePortraitCalibration)&&
       (onAirIndicator||previousSuite||playlistRunning||txDriverActive)){
      forcePortraitCalibration=false;
      Serial.println("CYD32,ROTATE,DEFER_TOUCH_CAL_RF_ACTIVE");
      return;
    }
  }
  // Preserve the logical screen, selected case and list page.
  // Finish any offscreen pass before touching the panel or the sprite.
  paint=&panel;bandFrameActive=false;
  panel.setRotation(makePortrait?0:1);
  panel.invertDisplay(panelInverted);
  panel.fillScreen(kBg);
  // Exactly the same 38,400-pixel allocation size in either orientation.
  // Recreate during an explicit user rotate only (never per animation
  // frame); if memory is fragmented, fall back to static rather than
  // drawing erase/redraw directly on the ST7798.
  allocateSceneFrame();
  Serial.printf("CYD32,FRAME,ROTATED_%s,%dx%d,bands=%u,heap=%lu,largest=%lu\n",
    bandFrameReady?(sceneFrameBands==1?"FULL8":"HALF8"):"STATIC_FALLBACK",
    panel.width(),panel.height()/sceneFrameBands,(unsigned)sceneFrameBands,
    (unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getMaxAllocHeap());
  if(makePortrait){
    if(!portraitFitLoaded||forcePortraitCalibration){
      const bool ok=runPortraitFivePointWizard();
      forcePortraitCalibration=false;
      if(!ok){
        // Do not strand the user on an unusable portrait screen. Restore
        // the verified landscape calibration without overwriting any fit.
        panel.setRotation(1);
        panel.invertDisplay(panelInverted);
        if(calibrated)loadCal();
        allocateSceneFrame();
        Cyd32FramePush::invalidate();
        Serial.println("CYD32,ROTATE,PORTRAIT_CAL_FAILED_RETURN_LANDSCAPE");
        wasPressed=true;lastPress=millis();redraw=true;
        return;
      }
    }
  }else if(calibrated)loadCal();
  portraitMode=makePortrait;
  portraitLastCount=UINT32_MAX;lastCount=UINT32_MAX;
  wasPressed=true;redraw=true;
  if(prefs.begin("sqemit32",false)){
    prefs.putBool("portrait",portraitMode);prefs.end();
  }
  Serial.printf("CYD32,ROTATE,%s,%dx%d\n",
    portraitMode?"PORTRAIT":"LANDSCAPE",lcd.width(),lcd.height());
}
// On-device calibration escape: working landscape Settings -> MORE.
void calibratePortraitFromDevice(){
  if(onAirIndicator||previousSuite||playlistRunning||txDriverActive){
    Serial.println("CYD32,TOUCH,CAL_REQUIRES_IDLE");
    return;
  }
  if(portraitMode){
    cyd32UiCalibratePortraitTouch();
  }else{
    forcePortraitCalibration=true;
    rotateCurrentScreen();
    if(portraitMode){
      screen=Screen::Home;settingsMore=false;redraw=true;
    }
  }
}
Cyd32UiAction touchPortrait(int x,int y,bool active,bool suite){
  if(y<278)return Cyd32UiAction::None;
  if(x<80){
    rotateCurrentScreen();
    return Cyd32UiAction::None;
  }
  if(x<160)
    return (active||suite||playlistRunning)?Cyd32UiAction::Stop:Cyd32UiAction::Run;
  return playlistRunning?(playlistPaused?Cyd32UiAction::PlaylistResume:
         Cyd32UiAction::PlaylistPause):Cyd32UiAction::Suite;
}

void batteryCalibrationScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(10,24,"BATTERY VOLTAGE CAL",kVaporPink,2);
  char line[64];
  if(batteryPercent<0)snprintf(line,sizeof(line),"RAW: ACQUIRING...");
  else{
    const unsigned rawCv=SqeBatteryCalibration::centiVolts(batteryMillivolts);
    snprintf(line,sizeof(line),"RAW: %u.%02u V",rawCv/100u,rawCv%100u);
  }
  menuRow(46,line,"GPIO34, 2:1 DIVIDER, SETTLING FILTER");
  if(batteryPercent<0)snprintf(line,sizeof(line),"ADJUSTED: WAITING");
  else{
    const uint16_t adjusted=correctedBatteryMv();
    const unsigned correctedCv=SqeBatteryCalibration::centiVolts(adjusted);
    snprintf(line,sizeof(line),"ADJUSTED: %u.%02u V / ~%u%%",
      correctedCv/100u,correctedCv%100u,
      (unsigned)SqeBatteryCalibration::linearPercent(adjusted));
  }
  char extra[64];
  snprintf(extra,sizeof(extra),"OFFSET %+d mV | HEADER: %s",(int)batteryOffsetMv,
           batteryDisplayVolts?"VOLTS":"PERCENT");
  menuRow(85,line,extra);
  button(8,127,147,32,"- 10 mV");
  button(165,127,147,32,"+ 10 mV");
  button(8,168,147,32,"- 50 mV");
  button(165,168,147,32,"+ 50 mV");
  footer("BACK","RESET",batteryDisplayVolts?"SHOW V":"SHOW %");
}
void settingsScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(10,24,settingsMore?"SETTINGS / DISPLAY":"HANDHELD SETTINGS",kVaporPink,2);
  if(settingsMore){
    char line[64];
    const char* theme[]={"VAPOR","OCEAN","SUNSET"};
    snprintf(line,sizeof(line),"THEME: %s",theme[themeLevel]);
    menuRow(46,line,"TAP TO CYCLE COLORS");
    menuRow(85,fontMode?"TEXT SIZE: STANDARD":"TEXT SIZE: COMPACT","TAP TO CHANGE LIST TYPE");
    menuRow(124,"SYNTHETIC IDENTITY","SEE LAST ACTUALLY TRANSMITTED MAC");
    menuRow(163,"CAL PORTRAIT TOUCH","ROTATE + 4-CORNER TOUCH SETUP");
    footer("BACK","< PREV","HELP");return;
  }
  char line[50];
  const char* br[]={"DIM","NORMAL","BRIGHT"};
  snprintf(line,sizeof(line),"BACKLIGHT: %s",br[brightnessLevel]);
  menuRow(46,line,"TAP TO CYCLE BRIGHTNESS");
  const char* an[]={"SLOW","NORMAL","FAST"};
  snprintf(line,sizeof(line),"ANIMATION: %s",an[animationLevel]);
  menuRow(85,line,"TAP TO CYCLE FRAME RATE");
  const char* time[]={"OFF","30 SECONDS","60 SECONDS"};
  snprintf(line,sizeof(line),"IDLE DIM: %s",time[screenTimeout]);
  menuRow(124,line,"WAKE TOUCH NEVER STARTS A TEST");
  snprintf(line,sizeof(line),"PANEL INVERT: %s",panelInverted?"ON":"OFF");
  menuRow(163,line,"BOARD-SPECIFIC DISPLAY POLARITY");
  footer("BACK","MORE >","BATTERY");
}

constexpr uint8_t kMenuEntries=12;
const char* const menuTitles[kMenuEntries]={
 "TEST LIBRARY","QUICK FIND","FAVORITES","TX HISTORY",
 "HEALTH / DIAGNOSTICS","SETTINGS","SQUACHY OUTFITS",
 "OTA UPDATES","TX ACTIVITY","HELP","RECOVERY","BACKGROUNDS"};
const char* const menuHints[kMenuEntries]={
 "369 SYNTHETIC CASES","A-Z CASE FILTER","SAVED SIGNALS","LAST TRANSMISSIONS",
 "HEAP, RADIO, UPTIME","DISPLAY, ROTATION, THEME","COSTUMES AND ANIMATION",
 "FIRMWARE UPDATE STATUS","DRIVER EVENTS","USAGE AND SAFETY","BOOT AND TOUCH","FIVE SCENES"};
void mainMenuScreen(){
  lcd.fillScreen(kBg);titleBar();label(13,25,"SQUACHEMIT / MENU",kVaporPink,2);
  for(uint8_t i=0;i<4;++i){
    const uint8_t n=menuPage*4+i;
    if(n<kMenuEntries)menuRow(46+i*39,menuTitles[n],menuHints[n]);
  }
  char count[24];snprintf(count,sizeof(count),"PAGE %u/3",(unsigned)(menuPage+1));
  label(12,201,count,kMuted);footer("HOME",menuPage?"< PREV":"",menuPage<2?"NEXT >":"");
}
static const char* const kOutfitNames[]={
  "CLASSIC SQUACHY","BLACK HOODIE","NEON LAB JACKET",
  "TANOOKI","UNICORN","TINFOIL","SHADOW","PLUMBER BRO","TALL BRO",
  "SPACE","BLUE BLUR","CAPTAIN","WOLF PELT","CHROME WING",
  "VOID EYE","SNOW PARKA","SHARK SUIT","YZZERD","OVER 9000",
  "SHAMBLER","TH3 0N3","STRIPES"};
constexpr uint8_t kOutfitCount=22;
void outfitScreen(){
  lcd.fillScreen(kBg);titleBar();label(13,25,"SQUACHY / WARDROBE",kVaporPink,2);
  for(uint8_t i=0;i<4;++i){
    const uint8_t n=outfitPage*4+i;
    menuRow(46+i*39,n<kOutfitCount?kOutfitNames[n]:"",
      n<kOutfitCount?(outfitLevel==n?"EQUIPPED":"TAP TO WEAR"):"",
      n<kOutfitCount&&outfitLevel==n);
  }
  char pg[30];snprintf(pg,sizeof(pg),"PAGE %u/6",(unsigned)(outfitPage+1));
  label(12,202,pg,kMuted);footer("BACK","< PREV","NEXT >");
}
// SquachWatch Settings::Background selectable order (retired TUNNEL and
// BORING-only BLACK intentionally omitted). Keep cycle order identical.
static const char* const kBackgroundNames[]={
  "DIGITAL RAIN","STARFIELD","FLYING TOASTERS","AQUARIUM",
  "TERMINAL LOG","FIREFLIES","FIRE","SNOWFALL",
  "THE GIBSON","SYNTHWAVE"};
void selectBackground(uint8_t next){
  if(next>=10||next==backgroundLevel)return;
  const uint8_t prior=backgroundLevel;
  backgroundLevel=next;
  backgroundPage=next/4;
  // Original SquachWatch background states allocate memory only when needed.
  // Release these bounded allocations every time a scene is changed.
  if(prior==0)SquachWatchScenes::releaseDigitalRain();
  if(prior==4)SquachWatchScenes::releaseTerm();
  if(prefs.begin("sqemit32",false)){
    prefs.putUChar("bg_sqwatch",backgroundLevel);prefs.end();
  }
  // Changing a background never changes the selected case or radio state.
  redraw=true;
}
void cycleBackground(int delta){
  selectBackground((uint8_t)((backgroundLevel+10+(delta<0?-1:1))%10));
}
void backgroundsScreen(){
  lcd.fillScreen(kBg);titleBar();label(13,25,"SQUACHEMIT / BACKGROUNDS",kVaporPink,2);
  for(uint8_t i=0;i<4;++i){
    const uint8_t n=backgroundPage*4+i;
    menuRow(46+i*39,n<10?kBackgroundNames[n]:"",
      n<10?(backgroundLevel==n?"ACTIVE - TAP AGAIN":"TAP TO SET"):"",n<10&&backgroundLevel==n);
  }
  char pg[24];snprintf(pg,sizeof(pg),"PAGE %u/3",(unsigned)(backgroundPage+1));
  label(10,202,pg,kMuted);
  footer("BACK","< PREV","NEXT >");
}
void otaInfoScreen(){
  lcd.fillScreen(kBg);titleBar();label(12,25,"FIRMWARE / OTA",kVaporPink,2);
  label(12,59,"OTA UNAVAILABLE ON THIS BUILD",kYellow);
  label(12,83,"CURRENT: HUGE_APP PARTITION",kCyan);
  label(12,107,"NO SAFE SECOND APP SLOT",kWhite);
  label(12,131,"USE LAB WEB FLASHER OVER USB",kWhite);
  label(12,156,"DO NOT FLASH WHILE TX ACTIVE",kGreen);
  label(12,180,"DUAL SLOT OTA NEEDS BUILD GATE",kMuted);
  footer("BACK","HELP","HOME");
}
void identityScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(12,26,"SYNTHETIC SIGNAL IDENTITY",kVaporPink,2);
  const Scenario* chosen=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  fit(12,66,chosen?chosen->id:"NO CASE",294,kWhite,2);
  const bool match=chosen&&strcmp(identityCase,chosen->id)==0&&identityMac[0];
  label(12,107,match?"LAST ON-AIR MAC:":"NO MATCHING TRANSMISSION",kMuted);
  fit(12,133,match?identityMac:"IDENTITY NOT YET OBSERVED",294,match?kCyan:kYellow,2);
  label(12,174,"Manufacturer labels are synthetic",kGreen);
  label(12,190,"Identity only shown after actual TX",kMuted);
  footer("BACK","HELP","RECOVERY");
}
void helpScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(12,24,"SQUACHEMIT HELP",kVaporPink,2);
  if(helpPage==0){
    label(12,61,"1. Select a test category",kCyan);
    label(12,92,"2. Choose a synthetic profile",kWhite);
    label(12,123,"3. Check DETAILS, then RUN",kWhite);
    label(12,154,"4. STOP ends RF immediately",kGreen);
  }else if(helpPage==1){
    label(12,61,"FAVORITES: tap Details to save",kCyan);
    label(12,92,"RUN LIST: favorites playlist",kWhite);
    label(12,123,"PAUSE waits for a safe gap",kWhite);
    label(12,154,"RESUME needs a deliberate tap",kGreen);
  }else{
    label(12,61,"USB: 115200 baud, UI ON",kCyan);
    label(12,92,"HEALTH: heap and radio status",kWhite);
    label(12,123,"No RF is sent on device boot",kWhite);
    label(12,154,"Test only with authorization",kGreen);
  }
  char pageLabel[28];snprintf(pageLabel,sizeof(pageLabel),"PAGE %u/3",(unsigned)helpPage+1u);
  label(12,192,pageLabel,kMuted);
  footer("BACK","NEXT","RECOVERY");
}
void recoveryScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(12,26,"BOOT / RECOVERY INFO",kVaporPink,2);
  char msg[58];
  snprintf(msg,sizeof(msg),"RESET REASON CODE: %u",resetReasonCode);
  label(12,68,msg,kCyan,2);
  label(12,105,"NO RF TRANSMISSION ON BOOT",kGreen);
  label(12,133,"BAD PORTRAIT TOUCH?",kWhite);
  lcd.fillRoundRect(11,153,298,39,4,kPurple);
  label(24,164,"CAL PORTRAIT TOUCH",kWhite,2);
  label(12,198,"IDLE ONLY / FOUR CORNERS",kMuted);
  footer("BACK","HELP","HOME");
}

size_t totalMatches(){
  return deviceCategoryCaseCount(category);
}
const Scenario* atRank(size_t rank,size_t* raw=nullptr){
  size_t i=0;
  if(!deviceCategoryCaseAtRank(category,rank,&i))return nullptr;
  if(raw)*raw=i;
  return &kScenarios[i];
}
void firstSelected(){
  size_t raw=0;
  if(atRank(page*kPageRows,&raw))selected=raw;
}
void menuRow(int y,const char* title,const char* sub,bool chosen){
  // Native SquachWatch settings row: squared panel, thin purple outline,
  // no rounded glossy cards and no crowded filter pills.
  lcd.fillRect(4,y+1,kW-10,34,chosen?0x2825:kBg);
  lcd.drawRect(4,y+1,kW-10,34,chosen?kPink:kPurple);
  fit(15,y+4,title,280,chosen?kWhite:kCyan,fontMode?2:1);
  if(sub)fit(15,y+23,sub,282,kMuted,1);
}

size_t favoriteCount(){
  size_t count=0;
  for(size_t i=0;i<kScenarioCount;++i)if(favBits[i/8]&(1u<<(i%8)))++count;
  return count;
}
const Scenario* favoriteAt(size_t rank,size_t* raw=nullptr){
  for(size_t i=0;i<kScenarioCount;++i)if(favBits[i/8]&(1u<<(i%8))){
    if(rank--==0){if(raw)*raw=i;return &kScenarios[i];}
  }
  return nullptr;
}
void toggleFavorite(){
  if(selected>=kScenarioCount)return;
  favBits[selected/8]^=(uint8_t)(1u<<(selected%8));
  if(prefs.begin("sqemitfav",false)){
    if(prefs.putBytes("fav",favBits,sizeof(favBits))!=sizeof(favBits))
      Serial.println("WARN,FAVORITES_SAVE_FAILED");
    prefs.end();
  }else Serial.println("WARN,FAVORITES_NVS_UNAVAILABLE");
  redraw=true;
}
void favoritesScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(10,25,"FAVORITE TEST PROFILES",kVaporPink,2);
  if(favoriteCount())label(244,25,"RUN LIST>",kCyan);
  const size_t count=favoriteCount();
  for(size_t row=0;row<4;++row){
    const Scenario* c=favoriteAt(favoritesPage*4+row);
    char name[58]={0};
    if(c)scenarioDeviceName(*c,name,sizeof(name));
    menuRow(45+(int)row*37,c?name:"",c?c->id:"");
  }
  char info[40];snprintf(info,sizeof(info),"%u SAVED PROFILES",(unsigned)count);
  label(12,196,info,kMuted);
  footer("BACK","< PREV","NEXT >");
}
const HistoryEntry* historyAt(size_t rank){
  if(rank>=historyCount)return nullptr;
  return &history[(historyHead+8-1-(uint8_t)rank)%8];
}
void historyScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(10,25,"RECENT TRANSMISSIONS",kVaporPink,2);
  for(size_t row=0;row<4;++row){
    const HistoryEntry* h=historyAt(historyPage*4+row);
    char sub[72]={0};
    if(h)snprintf(sub,sizeof(sub),"%s  |  UPTIME %lus",
                  h->status,(unsigned long)h->uptime);
    menuRow(45+(int)row*37,h?h->id:"",h?sub:"");
  }
  label(12,196,"TX RESULTS ONLY - NOT RX DETECTIONS",kMuted);
  footer("BACK","< PREV","NEXT >");
}


bool filterMatch(const Scenario& c){
  if(!filterLetter)return true;
  char name[58]={0};scenarioDeviceName(c,name,sizeof(name));
  char first=name[0];
  if(first>='a'&&first<='z')first=(char)(first-'a'+'A');
  return first==(char)('A'+filterLetter-1);
}
size_t filterCount(){
  size_t count=0;for(size_t i=0;i<kScenarioCount;++i)
    if(filterMatch(kScenarios[i]))++count;
  return count;
}
const Scenario* filterAt(size_t rank,size_t* raw=nullptr){
  for(size_t i=0;i<kScenarioCount;++i)if(filterMatch(kScenarios[i])){
    if(rank--==0){if(raw)*raw=i;return &kScenarios[i];}
  }
  return nullptr;
}
void quickFilterScreen(){
  lcd.fillScreen(kBg);titleBar();
  label(10,23,"QUICK FIND / FIRST LETTER",kVaporPink,2);
  char head[52];snprintf(head,sizeof(head),"FILTER: %c  (%u MATCHES)",
      filterLetter?(char)('A'+filterLetter-1):'*',(unsigned)filterCount());
  label(12,45,head,kCyan);
  label(248,45,"<  >",kYellow);
  for(size_t row=0;row<4;++row){
    const Scenario* c=filterAt(filterPage*4+row);
    char name[58]={0};if(c)scenarioDeviceName(*c,name,sizeof(name));
    menuRow(63+(int)row*34,c?name:"",c?c->id:"");
  }
  footer("BACK","< PREV","NEXT >");
}
void categoriesScreen(){
  lcd.fillScreen(kBg);
  titleBar();
  label(8,21,"TEST LIBRARY / DEVICE TYPE",kVaporPink,2);
  label(245,22,"FIND >",kCyan);
  for(size_t row=0;row<kPageRows;++row){
    const size_t index=categoryPage*kPageRows+row;
    if(index>=kDeviceCategoryCount)break;
    const DeviceCategory c=static_cast<DeviceCategory>(index);
    char sub[42];
    snprintf(sub,sizeof(sub),"%u TEST CASES", (unsigned)deviceCategoryCaseCount(c));
    menuRow(44+(int)row*39,deviceCategoryName(c),sub);
  }
  char pages[28];
  snprintf(pages,sizeof(pages),"PAGE %u/%u",(unsigned)(categoryPage+1),
           (unsigned)((kDeviceCategoryCount+kPageRows-1)/kPageRows));
  label(12,199,pages,kMuted);
  footer("BACK","< PREV","NEXT >");
}
void libraryScreen(const char* active,bool suite){
  lcd.fillScreen(kBg);
  titleBar();
  char header[52];
  snprintf(header,sizeof(header),"%s  %u CASES",deviceCategoryName(category),
           (unsigned)totalMatches());
  fit(8,23,header,301,kVaporPink,2);
  for(size_t row=0;row<4;++row){
    size_t raw=0;
    const Scenario* s=atRank(page*4+row,&raw);
    char title[65]={0},hint[82]={0};
    if(s){
      char name[55]={0};
      scenarioDeviceName(*s,name,sizeof(name));
      // Name first on every row; the original stable case ID remains in
      // the second line for reproducible Web Serial/CSV test evidence.
      snprintf(title,sizeof(title),"%02u  %.48s",
               (unsigned)(page*4+row+1),name);
      if(sqeStarts(s->id,"OUI-"))
        snprintf(hint,sizeof(hint),"NO REMOTE ID | %s",s->id);
      else snprintf(hint,sizeof(hint),"%s  |  %s",s->id,s->expected);
    }
    menuRow(48+(int)row*36,title,hint,s&&raw==selected);
  }
  char pages[30];
  const size_t total=(totalMatches()+3)/4;
  snprintf(pages,sizeof(pages),"PAGE %u/%u", (unsigned)(page+1),
           (unsigned)(total?total:1));
  label(12,198,pages,kMuted);
  if(active&&*active)label(233,198,"ON AIR",kPink);
  footer("BACK","< PREV",(active&&*active)?"STOP":"NEXT >");
}
void calibrationScreen(){
  lcd.fillScreen(kBg);
  titleBar();
  label(28,35,"SQUACHEMIT TOUCH SETUP",kCyan,2);
  menuRow(87,"TAP SCREEN TO CALIBRATE","FOLLOW THE 5 CROSSHAIRS");
  label(12,179,"NO RF ON BOOT  /  115200 BAUD",kGreen);
}
bool loadCal(){
  if(!prefs.begin("sqemit32",true))return false;
  uint16_t data[5]={0};
  const bool ok=prefs.getBytesLength("touchland")==10 &&
       prefs.getBytes("touchland",data,10)==10;
  prefs.end();
  if(ok)lcd.setTouch(data);
  return ok;
}
void startCal(){
  uint16_t data[5]={0};
  lcd.fillScreen(kBg);
  label(18,15,"SQUACHWATCH TOUCH CAL",kCyan,2);
  lcd.calibrateTouch(data,kWhite,kBg,15);
  lcd.setTouch(data);
  if(prefs.begin("sqemit32",false)){
    if(prefs.putBytes("touchland",data,10)!=10)
      Serial.println("WARN,CYD32_TOUCH_SAVE");
    prefs.end();
  }
  calibrated=true;
  wasPressed=true;
  redraw=true;
  Serial.println("CYD32,TOUCH,CALIBRATED_LANDSCAPE");
}
void cycleBackground(int delta);
// Main Menu (gear) entries return to their actual caller, not the
// normal Home shortcut hierarchy. Preserve the menu page on return.
void backToMenuOr(Screen fallback){
  screen=openedFromMainMenu?Screen::MainMenu:fallback;
  openedFromMainMenu=false;quickFromCategories=false;redraw=true;
}
Cyd32UiAction touchHome(int x,int y,bool active,bool suite){
  // Same gesture as SquachWatch CLEAR: outer 10% of the main scene.
  // Exclude top status bar, footer, and current transmit safety controls.
  if(y>=23&&y<182&&(x<kW/10||x>=kW-kW/10)){
    cycleBackground(x<kW/10?-1:+1);
    return Cyd32UiAction::None;
  }
  if(y>=kFooterY){
    if(x<108){openedFromMainMenu=false;quickFromCategories=false;screen=Screen::Categories;redraw=true;return Cyd32UiAction::None;}
    if(x<213)return (active||suite||playlistRunning)?Cyd32UiAction::Stop:Cyd32UiAction::Run;
    return playlistRunning?(playlistPaused?Cyd32UiAction::PlaylistResume:Cyd32UiAction::PlaylistPause):Cyd32UiAction::Suite;
  }
  if(!active&&!suite&&x<93&&y>=183&&y<199){
    screen=Screen::Activity;redraw=true;return Cyd32UiAction::None;
  }
  if(!active && !suite && x>=230 && y>=199 && y<214){
    screen=Screen::Details;redraw=true;return Cyd32UiAction::None;
  }
  if(!active && !suite && x>=93 && x<230 && y>=183 && y<199){
    screen=Screen::Duration;
    redraw=true;
    return Cyd32UiAction::None;
  }
  if(x>=116&&x<=204&&y>=70&&y<183){
    petUntil=millis()+1500;
    // Squachy updates on its sprite timer; do not clear the entire TFT.
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchCategories(int x,int y){
  if(x>=233&&y>=15&&y<43){quickFromCategories=true;screen=Screen::QuickFilter;filterPage=0;redraw=true;return Cyd32UiAction::None;}
  if(y>=kFooterY){
    if(x<108){backToMenuOr(Screen::Home);return Cyd32UiAction::None;}
    if(x<213){if(categoryPage>0){--categoryPage;redraw=true;}return Cyd32UiAction::None;}
    if((categoryPage+1)*kPageRows<kDeviceCategoryCount){++categoryPage;redraw=true;}
    return Cyd32UiAction::None;
  }
  if(y<44||y>=44+(int)kPageRows*39)return Cyd32UiAction::None;
  const size_t row=(size_t)((y-44)/39);
  const size_t index=categoryPage*kPageRows+row;
  if(index<kDeviceCategoryCount){
    category=static_cast<DeviceCategory>(index);
    page=0;
    firstSelected();
    screen=Screen::Library;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchLibrary(int x,int y,bool active){
  if(y>=kFooterY){
    if(x<108){screen=Screen::Categories;redraw=true;return Cyd32UiAction::None;}
    if(x<213){if(page>0){--page;firstSelected();redraw=true;}return Cyd32UiAction::None;}
    if(active)return Cyd32UiAction::Stop;
    if((page+1)*4<totalMatches()){++page;firstSelected();redraw=true;}
    return Cyd32UiAction::None;
  }
  if(y>=48&&y<48+4*36){
    const size_t row=(size_t)((y-48)/36);
    size_t idx=0;
    if(row<4&&atRank(page*4+row,&idx)){
      selected=idx;
      screen=Screen::Home;
      redraw=true;
    }
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchDuration(int x,int y){
  if(y>=kFooterY){
    if(x>=112 && x<213){setDuration(12);return Cyd32UiAction::None;}
    screen=Screen::Home;redraw=true;return Cyd32UiAction::None;
  }
  if(y>=85 && y<139){
    if(x>=12 && x<95 && durationSeconds>3)setDuration(durationSeconds-1);
    if(x>=225 && x<308 && durationSeconds<30)setDuration(durationSeconds+1);
  }
  if(y>=155&&y<184){
    if(x>=12&&x<104)setDuration(3);
    else if(x>=114&&x<206)setDuration(10);
    else if(x>=216&&x<308)setDuration(30);
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchDetails(int x,int y,bool active){
  openedFromMainMenu=false;quickFromCategories=false;
  if(!active&&x>=230&&y>=20&&y<62){
    if(y<41){favoritesPage=0;screen=Screen::Favorites;}
    else {historyPage=0;screen=Screen::History;}
    redraw=true;return Cyd32UiAction::None;
  }
  if(!active&&y>=188&&y<211){toggleFavorite();return Cyd32UiAction::None;}
  if(y<kFooterY)return Cyd32UiAction::None;
  if(x<108){screen=Screen::Home;redraw=true;return Cyd32UiAction::None;}
  if(x<213){favoritesPage=0;screen=Screen::Favorites;redraw=true;return Cyd32UiAction::None;}
  if(!active){screen=Screen::Home;redraw=true;return Cyd32UiAction::Run;}
  return Cyd32UiAction::None;
}
Cyd32UiAction touchDiagnostics(int x,int y){
  if(y<kFooterY)return Cyd32UiAction::None;
  if(x<108)screen=Screen::MainMenu;
  else if(x>=213)screen=Screen::Home;
  redraw=true;
  return Cyd32UiAction::None;
}
Cyd32UiAction touchActivity(int x,int y){
  if(y>=kFooterY){
    screen=x<108?Screen::Home:x<213?Screen::Evidence:Screen::Details;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchEvidence(int x,int y){
  if(y>=kFooterY){
    screen=x<108?Screen::Activity:x<213?Screen::Details:Screen::Home;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
void openSettingsSubmenu(Screen destination);
void backFromSettingsSubmenu();
Cyd32UiAction touchSettings(int x,int y){
  if(y>=kFooterY){
    if(x<108){
      // BACK from DISPLAY returns to the first Settings page; only the
      // first page exits to the main menu. Shared by landscape/portrait.
      if(settingsMore){settingsMore=false;}
      else {screen=Screen::MainMenu;settingsSubmenuFlow=false;}
    }
    else if(x<213){settingsMore=!settingsMore;}
    else if(settingsMore){openSettingsSubmenu(Screen::Help);}
    else {openSettingsSubmenu(Screen::BatteryCalibration);}
    redraw=true;return Cyd32UiAction::None;
  }
  if(y>=46&&y<201){
    const int row=(y-46)/39;
    if(settingsMore){
      if(row==0){themeLevel=(themeLevel+1)%3;applyTheme();persistUiSettings();}
      else if(row==1){fontMode=(fontMode+1)%2;persistUiSettings();}
      else if(row==2){openSettingsSubmenu(Screen::Identity);}
      else if(row==3){calibratePortraitFromDevice();}
      redraw=true;return Cyd32UiAction::None;
    }
    if(row==0){brightnessLevel=(brightnessLevel+1)%3;applyBrightness();persistUiSettings();}
    else if(row==1){animationLevel=(animationLevel+1)%3;persistUiSettings();}
    else if(row==2){screenTimeout=(screenTimeout+1)%3;persistUiSettings();}
    else if(row==3){panelInverted=!panelInverted;lcd.invertDisplay(panelInverted);
      if(prefs.begin("sqemit32",false)){prefs.putBool("invert2",panelInverted);prefs.end();}}
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchBatteryCalibration(int x,int y){
  if(y>=kFooterY){
    if(x<108){backFromSettingsSubmenu();}
    else if(x<213)setBatteryOffsetMv(0);
    else setBatteryDisplayVolts(!batteryDisplayVolts);
    return Cyd32UiAction::None;
  }
  if(y>=124&&y<164){
    setBatteryOffsetMv((int32_t)batteryOffsetMv+(x<160?-10:10));
  }else if(y>=164&&y<205){
    setBatteryOffsetMv((int32_t)batteryOffsetMv+(x<160?-50:50));
  }
  return Cyd32UiAction::None;
}
// Explicit Settings origin for ALL child views, including Battery.
// A child BACK always goes to the first Settings page; direct Main Menu
// Help/Recovery retain their normal Main Menu return.
void openSettingsSubmenu(Screen destination){
  settingsSubmenuFlow=true;
  screen=destination;
  settingsMore=false;
  redraw=true;
}
void backFromSettingsSubmenu(){
  if(screen==Screen::BatteryCalibration||settingsSubmenuFlow){
    screen=Screen::Settings;
    settingsMore=false;
    settingsSubmenuFlow=false;
  }else{
    screen=Screen::MainMenu;
  }
  redraw=true;
}
Cyd32UiAction touchHelp(int x,int y){
  if(y>=kFooterY){
    if(x<108)backFromSettingsSubmenu();
    else if(x<213)helpPage=(helpPage+1)%3;
    else screen=Screen::Recovery;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchIdentity(int x,int y){
  if(y>=kFooterY){
    if(x<108)backFromSettingsSubmenu();
    else if(x<213)screen=Screen::Help;
    else screen=Screen::Recovery;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchRecovery(int x,int y){
  if(y>=150&&y<194){
    calibratePortraitFromDevice();
    return Cyd32UiAction::None;
  }
  if(y>=kFooterY){
    if(x<108)backFromSettingsSubmenu();
    else if(x<213)screen=Screen::Help;
    else {screen=Screen::Home;settingsSubmenuFlow=false;settingsMore=false;}
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchFavorites(int x,int y){
  if(x>=220&&y>=15&&y<44&&favoriteCount()){
    screen=Screen::Home;redraw=true;return Cyd32UiAction::PlaylistStart;
  }
  if(y>=kFooterY){
    if(x<108){backToMenuOr(Screen::Details);}
    else if(x<213){if(favoritesPage>0){--favoritesPage;redraw=true;}}
    else if((favoritesPage+1)*4<favoriteCount()){++favoritesPage;redraw=true;}
    return Cyd32UiAction::None;
  }
  if(y>=45&&y<45+4*37){
    size_t raw=0;
    if(favoriteAt(favoritesPage*4+(size_t)((y-45)/37),&raw)){
      selected=raw;screen=Screen::Home;redraw=true;
    }
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchHistory(int x,int y){
  if(y>=kFooterY){
    if(x<108){backToMenuOr(Screen::Details);}
    else if(x<213){if(historyPage>0){--historyPage;redraw=true;}}
    else if((historyPage+1)*4<historyCount){++historyPage;redraw=true;}
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchQuickFilter(int x,int y){
  if(y>=kFooterY){
    if(x<108){if(quickFromCategories){quickFromCategories=false;screen=Screen::Categories;redraw=true;}else backToMenuOr(Screen::Categories);}
    else if(x<213){if(filterPage>0)--filterPage;redraw=true;}
    else{if((filterPage+1)*4<filterCount())++filterPage;redraw=true;}
    return Cyd32UiAction::None;
  }
  if(y>=39&&y<63){
    if(x<158){filterLetter=(uint8_t)((filterLetter+26)%27);}
    else {filterLetter=(uint8_t)((filterLetter+1)%27);}
    filterPage=0;redraw=true;return Cyd32UiAction::None;
  }
  if(y>=63&&y<199){
    size_t raw=0;
    if(filterAt(filterPage*4+(size_t)((y-63)/34),&raw)){
      selected=raw;screen=Screen::Home;redraw=true;
    }
  }
  return Cyd32UiAction::None;
}

void portraitRow(int y,const char* head,const char* sub){
  lcd.fillRect(5,y,230,44,kBg);lcd.drawRect(5,y,230,44,kPurple);
  fit(13,y+5,head,212,kCyan,2);if(sub)fit(13,y+25,sub,212,kMuted,1);
}
void portraitPage(const char* active,uint32_t remaining,bool suite){
  // Every page shares the same logical Screen and selection in both rotations.
  lcd.fillScreen(kBg);
  lcd.fillRect(0,0,240,31,kPurple);
  // Match a generous 65x42 touch target. The previous 31x20 menu
  // button was too small for the resistive panel in portrait mode.
  lcd.fillRoundRect(5,3,46,27,3,kWhite);
  for(int y=10;y<=22;y+=6)lcd.drawFastHLine(16,y,23,kInk);
  label(81,9,"SQUACHEMIT",kWhite);
  drawBatteryStatus(190,13);
  drawTxBeacon(221,13,active&&*active);
  const char* name=screen==Screen::Home?"HOME":screen==Screen::MainMenu?"MENU":
    screen==Screen::Categories?"TEST LIBRARY":screen==Screen::Library?"SIGNALS":
    screen==Screen::Settings?"SETTINGS":screen==Screen::BatteryCalibration?"BAT CAL":
    screen==Screen::Outfits?"OUTFITS":
    screen==Screen::Backgrounds?"BACKGROUNDS":screen==Screen::Favorites?"FAVORITES":
    screen==Screen::QuickFilter?"QUICK FIND":screen==Screen::History?"HISTORY":
    screen==Screen::Duration?"TX TIME":screen==Screen::Details?"DETAILS":
    screen==Screen::Diagnostics?"HEALTH":screen==Screen::OtaInfo?"OTA":
    screen==Screen::Activity?"TX ACTIVITY":screen==Screen::Evidence?"EVIDENCE":
    screen==Screen::Help?"HELP":screen==Screen::Identity?"IDENTITY":"RECOVERY";
  // HOME is already evident from the main scene; printing "HOME" here
  // was an unexplained extra line above the SquachWatch background.
  if(screen!=Screen::Home)label(12,37,name,kVaporPink,2);
  else label(78,42,"HOLD SCREEN 5s: CAL TOUCH",kMuted);
  if(screen==Screen::QuickFilter){
    char filterInfo[28];
    snprintf(filterInfo,sizeof(filterInfo),"%c >",
             filterLetter?(char)('A'+filterLetter-1):'*');
    label(195,43,filterInfo,kYellow);
  }
  if(screen==Screen::Favorites)label(151,43,"RUN LIST",kGreen);
  if(screen==Screen::Home){
    // Keep the same Squachy and selected profile when rotating to portrait.
    lcd.fillRect(0,60,240,156,lcd.color565(3,4,16));
    // Portrait variant of the selected SquachWatch-style scene.
#if defined(SQE_ORIGINAL_SQUACHY)
    drawOriginalScene(60,215);
#else
    if(backgroundLevel==9){
      SquachWatchScenes::drawSynthwave(lcd,millis(),60,215,0.44f);
    }else if(backgroundLevel==4){
      SquachWatchScenes::drawTerminalLog(lcd,millis(),60,215);
    }else if(backgroundLevel==0){
      SquachWatchScenes::drawDigitalRain(lcd,millis(),60,215,advanceScene);
    }else if(backgroundLevel==5){
      SquachWatchScenes::drawFireflies(lcd,millis(),60,215);
    }else if(backgroundLevel==1||backgroundLevel==7){
      for(int i=0;i<82;++i){
        const int x=(i*73+17)%240,y=63+(i*83+13)%148;
        lcd.fillCircle(x,y,i%7==0?2:1,
          backgroundLevel==7?kWhite:(i%3?kCyan:kWhite));
      }
    }else if(backgroundLevel==6){
      for(int i=0;i<54;++i){
        const int x=(i*41+7)%240,y=90+(i*31)%115;
        lcd.drawLine(x,213,x-3,y,i%3?kPink:kYellow);
      }
    }else if(backgroundLevel==3){
      lcd.fillRect(0,60,240,156,lcd.color565(0,31,53));
      for(int x=5;x<240;x+=26){
        lcd.drawLine(x,214,x+15,139,kGreen);
        lcd.fillTriangle(x,95,x+18,87,x+18,103,kYellow);
      }
    }else if(backgroundLevel==2){
      for(int i=0;i<8;++i){
        const int x=12+(i*43)%215,y=71+(i*31)%130;
        lcd.fillRoundRect(x,y,19,16,3,kYellow);
        lcd.drawLine(x,y+7,x-7,y-5,kWhite);
      }
    }else if(backgroundLevel==8){
      for(int x=0;x<240;x+=16)lcd.drawFastVLine(x,60,156,kPurple);
      for(int y=65;y<215;y+=16)lcd.drawFastHLine(0,y,240,kCyan);
    }
#endif
#if defined(SQE_ORIGINAL_SQUACHY)
    // Portrait must use the SAME upstream Squachy procedural renderer as
    // landscape for every style. A separate RGB332 custom sprite previously
    // drew an opaque magenta box behind hoodie/jacket (or disappeared).
    Squachy::setOutfitPreview(outfitLevel<=2?0:(int8_t)(outfitLevel-2));
    Squachy::setSqemitCustomLook(outfitLevel==1?1:outfitLevel==2?2:0);
    Squachy::holdBubble(true);
    Squachy::tick(lcd,120,68,138,frameNow(),advanceScene,0.3f,false,0,100);
    Squachy::setSqemitCustomLook(0);
    Squachy::setOutfitPreview(-1);
#else
    // Non-original-art fallback only; standard CYD32 never enters here.
    if(squachSpriteReady){
      SquachyPort::render(squachSprite,frameNow(),active&&*active,suite,
                          (int32_t)(petUntil-frameNow())>0,outfitLevel);
      if(bandFrameActive)
        squachSprite.pushToSprite(&halfFrame,64,75,0xF81E);
      else squachSprite.pushSprite(64,75,0xF81E);
    }
#endif
    lcd.fillRoundRect(7,205,226,34,4,kTaskbar);
    lcd.drawRoundRect(7,205,226,34,4,kPink);
    const char* talk=active&&*active?"SYNTHETIC TX IN PROGRESS":
      suite?"RUNNING FULL 369-CASE SUITE":"PICK A PROFILE. PRESS RUN!";
    fit(13,212,talk,212,kWhite,2);
    const Scenario* current=selected<kScenarioCount?&kScenarios[selected]:nullptr;
    fit(12,246,current?current->id:"NO PROFILE",216,kCyan);
    char status[42];
    snprintf(status,sizeof(status),active&&*active?"TX %lus LEFT":"TX TIME %us",
             (unsigned long)(active&&*active?(remaining+999)/1000:durationSeconds));
    fit(12,267,status,216,active&&*active?kGreen:kYellow);
    button(3,287,74,30,"TESTS");
    button(83,287,74,30,active||suite?"STOP":"RUN");
    button(163,287,74,30,"SUITE");
    return;
  }
  if(screen==Screen::BatteryCalibration){
    char raw[28],adjusted[40],offset[28];
    if(batteryPercent<0){
      snprintf(raw,sizeof(raw),"ACQUIRING...");
      snprintf(adjusted,sizeof(adjusted),"WAITING FOR ADC");
    }else{
      const unsigned rawCv=SqeBatteryCalibration::centiVolts(batteryMillivolts);
      const uint16_t adjustedMv=correctedBatteryMv();
      const unsigned correctedCv=SqeBatteryCalibration::centiVolts(adjustedMv);
      snprintf(raw,sizeof(raw),"%u.%02u V",rawCv/100u,rawCv%100u);
      snprintf(adjusted,sizeof(adjusted),"%u.%02u V  ~%u%%",
        correctedCv/100u,correctedCv%100u,
        (unsigned)SqeBatteryCalibration::linearPercent(adjustedMv));
    }
    snprintf(offset,sizeof(offset),"OFFSET %+d mV",(int)batteryOffsetMv);
    portraitRow(65,"RAW BATTERY",raw);
    portraitRow(114,"CORRECTED / SAVED",adjusted);
    portraitRow(163,"-10 mV  /  +10 mV","LEFT -   RIGHT +");
    portraitRow(212,"-50 mV  /  +50 mV",offset);
    button(3,287,74,30,"BACK");
    // Portrait must never hide emergency STOP during a running test.
    button(83,287,74,30,(active&&*active)||suite||playlistRunning?"STOP":"RESET");
    button(163,287,74,30,batteryDisplayVolts?"SHOW V":"SHOW %");
    return;
  }
  for(int i=0;i<4;++i){
    const char* head="",*sub="";
    char labelBuf[64]={0},value[48]={0};
    const Scenario* c=nullptr;
    if(screen==Screen::MainMenu){
      const int n=menuPage*4+i;
      if(n<kMenuEntries){head=menuTitles[n];sub=menuHints[n];}
    }else if(screen==Screen::Categories){
      const int n=categoryPage*4+i;
      if(n<(int)kDeviceCategoryCount){head=deviceCategoryName((DeviceCategory)n);sub="TAP TO VIEW SIGNALS";}
    }else if(screen==Screen::Library||screen==Screen::Favorites||screen==Screen::QuickFilter){
      if(screen==Screen::Library)c=atRank(page*4+i);
      else if(screen==Screen::Favorites)c=favoriteAt(favoritesPage*4+i);
      else c=filterAt(filterPage*4+i);
      if(c){scenarioDeviceName(*c,labelBuf,sizeof(labelBuf));head=labelBuf;sub=c->id;}
    }else if(screen==Screen::History){
      const HistoryEntry* h=historyAt(historyPage*4+i);
      if(h){head=h->id;sub=h->status;}
    }else if(screen==Screen::Outfits){
      const int n=outfitPage*4+i;
      if(n<kOutfitCount){head=kOutfitNames[n];sub=n==outfitLevel?"EQUIPPED":"SELECT OUTFIT";}
    }else if(screen==Screen::Backgrounds){
      const int n=backgroundPage*4+i;
      if(n<10){head=kBackgroundNames[n];sub=n==backgroundLevel?"ACTIVE":"SELECT BACKGROUND";}
    }else if(screen==Screen::Settings){
      static const char* basics[]={"BACKLIGHT","ANIMATION","IDLE DIM","INVERSION"};
      static const char* more[]={"THEME","FONT SIZE","IDENTITY","CAL PORTRAIT TOUCH"};
      head=settingsMore?more[i]:basics[i];
      if(!settingsMore){
        const char* br[]={"DIM","NORMAL","BRIGHT"};
        const char* an[]={"SLOW","NORMAL","FAST"};
        const char* idle[]={"OFF","30 SECONDS","60 SECONDS"};
        sub=i==0?br[brightnessLevel]:i==1?an[animationLevel]:
            i==2?idle[screenTimeout]:panelInverted?"ON":"OFF";
      }else{
        static const char* theme[]={"VAPOR","OCEAN","SUNSET"};
        sub=i==0?theme[themeLevel]:i==1?(fontMode?"STANDARD":"COMPACT"):
            i==2?"LAST TX MAC":"4-CORNER TOUCH SETUP";
      }
    }else if(screen==Screen::Home){
      const Scenario* current=selected<kScenarioCount?&kScenarios[selected]:nullptr;
      if(i==0){head="SELECTED PROFILE";sub=current?current->id:"NONE";}
      if(i==1){head="TRANSMIT DURATION";snprintf(value,sizeof(value),"%u SECONDS",durationSeconds);sub=value;}
      if(i==2){head=active&&*active?"ON AIR":suite?"SUITE ACTIVE":"TRANSMITTER READY";
        sub=active&&*active?"SYNTHETIC TEST IN PROGRESS":"NO RF UNTIL RUN";}
      if(i==3){head="PROFILE DETAILS";sub="TAP TO OPEN";}
    }else if(screen==Screen::Duration){
      static const char* d[]={"3 SECONDS","10 SECONDS","30 SECONDS","+1 SECOND"};
      head=d[i];sub="TAP TO SET";
    }else if(screen==Screen::Diagnostics){
      if(i==0){head="FREE HEAP";snprintf(value,sizeof(value),"%lu BYTES",(unsigned long)ESP.getFreeHeap());sub=value;}
      if(i==1){head="LARGEST BLOCK";snprintf(value,sizeof(value),"%lu BYTES",(unsigned long)ESP.getMaxAllocHeap());sub=value;}
      if(i==2){head="UPTIME";snprintf(value,sizeof(value),"%lu SEC",(unsigned long)(millis()/1000));sub=value;}
      if(i==3){head="TRANSMITTER";sub=active&&*active?"ON AIR":"IDLE";}
    }else if(screen==Screen::Details){
      const Scenario* c=selected<kScenarioCount?&kScenarios[selected]:nullptr;
      if(i==0){head="SELECTED SIGNAL";sub=c?c->id:"NONE";}
      if(i==1){head="EXPECTED CLASS";sub=c?c->expected:"NONE";}
      if(i==2){head="FAVORITE - TAP TO TOGGLE";
        sub=c&&((favBits[selected/8]>>(selected%8))&1u)?"SAVED":"NOT SAVED";}
      if(i==3){head="DURATION";snprintf(value,sizeof(value),"%u SECONDS",durationSeconds);sub=value;}
    }else if(screen==Screen::Activity){
      static const char* labels[]={"RADIO DRIVER","START EVENTS","END EVENTS","START/END ERRORS"};
      head=labels[i];
      if(i==0)sub=txDriverActive?"ACTIVE":"IDLE";
      else{
        const uint32_t count=i==1?txBegins:i==2?txEnds:txErrors;
        snprintf(value,sizeof(value),"%lu",(unsigned long)count);sub=value;
      }
    }else if(screen==Screen::Evidence){
      static const char* labels[]={"RECEIVER RESULT","REPORTED CLASS","SIGNAL RSSI","SOURCE"};
      head=labels[i];
      if(i==0)sub=rxId[0]?rxId:"NO REPORT";
      if(i==1)sub=rxLabel[0]?rxLabel:"NONE";
      if(i==2){snprintf(value,sizeof(value),"%d dBm",rxRssi);sub=rxId[0]?value:"UNKNOWN";}
      if(i==3)sub="PC LOG - NOT SELF VERIFIED";
    }else if(screen==Screen::Identity){
      static const char* labels[]={"CURRENT PROFILE","LAST TX MAC","IDENTITY STATUS","SOURCE"};
      head=labels[i];
      if(i==0)sub=selected<kScenarioCount?kScenarios[selected].id:"NONE";
      if(i==1)sub=identityMac[0]?identityMac:"NO OBSERVATION";
      if(i==2)sub=identityCase[0]?identityCase:"NONE";
      if(i==3)sub="ACTUAL TX RECORD";
    }else if(screen==Screen::Recovery){
      static const char* labels[]={"RESET REASON","RF ON BOOT","CAL PORTRAIT TOUCH","FIRMWARE"};
      head=labels[i];
      if(i==0){snprintf(value,sizeof(value),"%u",resetReasonCode);sub=value;}
      if(i==1)sub="TRANSMISSION DISABLED";
      if(i==2)sub="TAP TO RE-RUN 4 CORNERS";
      if(i==3)sub="USE LAB USB FLASHER";
    }else if(screen==Screen::OtaInfo){
      static const char* labels[]={"OTA STATUS","PARTITION","SAFE UPDATE SLOT","INSTALL METHOD"};
      static const char* values[]={"UNAVAILABLE","HUGE_APP SINGLE SLOT","NOT AVAILABLE","USB WEB FLASHER"};
      head=labels[i];sub=values[i];
    }else if(screen==Screen::Help){
      static const char* labels[]={"CHOOSE SIGNAL","CHECK PROFILE","RUN TX","STOP TX"};
      head=labels[i];sub=i==0?"TESTS / QUICK FIND":i==1?"DETAILS":i==2?"3-30 SECONDS":"ANYTIME";
    }else{
      static const char* h[]={"CURRENT PROFILE","DEVICE STATUS","INFORMATION","BACK TO MENU"};
      head=h[i];sub=i==0?(selected<kScenarioCount?kScenarios[selected].id:"NONE"):
        i==1?(active&&*active?"TRANSMITTING":"IDLE"):
        i==2?(screen==Screen::OtaInfo?"USB FLASHER REQUIRED":"SEE LANDSCAPE DETAILS"):
        "TAP BACK";
    }
    portraitRow(65+i*49,head,sub);
  }
  if(screen==Screen::Home&&active&&*active){
    char left[24];snprintf(left,sizeof(left),"%lus LEFT",(unsigned long)((remaining+999)/1000));
    label(10,267,left,kGreen);
  }else label(10,267,"ROTATE: TOP-RIGHT",kMuted);
  button(3,287,74,30,screen==Screen::Home?"TESTS":"BACK");
  // Permit menu navigation during bounded TX without hiding STOP.
  // Every portrait page keeps its emergency center STOP button.
  const bool radioBusy=(active&&*active)||suite||playlistRunning;
  if(radioBusy||screen!=Screen::MainMenu||menuPage)
    button(83,287,74,30,radioBusy?"STOP":screen==Screen::Home?"RUN":"PREV");
  if(screen!=Screen::MainMenu||menuPage<2)
    button(163,287,74,30,screen==Screen::Home?"SUITE":"NEXT");
  if(screen==Screen::MainMenu){char pg[18];snprintf(pg,sizeof(pg),"PAGE %u/3",(unsigned)menuPage+1);label(12,267,pg,kMuted);}
}
// TFT_eSPI getTouch() rejects a calibrated coordinate that falls outside
// the panel. A bezel-edge touch can also have pressure below the 450
// landscape threshold. Recover ONLY the portrait menu from two stable raw
// samples, with signed calibration math and a narrow physical corner gate.
// All other control taps still require normal validated getTouch().
bool recoverPortraitMenuTouch(uint16_t& x,uint16_t& y){
  if(portraitFitLoaded||!portraitMode||!portraitCalibrated||
     portraitTouchData[1]<200||portraitTouchData[3]<200||
     (portraitTouchData[4]&~7u))return false;
  if(lcd.getTouchRawZ()<230)return false;
  uint16_t rx=0,ry=0,rx2=0,ry2=0;
  lcd.getTouchRaw(&rx,&ry);
  lcd.getTouchRaw(&rx2,&ry2);
  const int32_t dx=(int32_t)rx-(int32_t)rx2;
  const int32_t dy=(int32_t)ry-(int32_t)ry2;
  if(dx>65||dx< -65||dy>65||dy< -65)return false;
  const uint16_t flags=portraitTouchData[4];
  const int32_t u=(flags&1u)?(int32_t)ry:(int32_t)rx;
  const int32_t v=(flags&1u)?(int32_t)rx:(int32_t)ry;
  int32_t px=(u-(int32_t)portraitTouchData[0])*240/(int32_t)portraitTouchData[1];
  int32_t py=(v-(int32_t)portraitTouchData[2])*320/(int32_t)portraitTouchData[3];
  if(flags&2u)px=240-px;
  if(flags&4u)py=320-py;
  // Accept only the top-left header and at most 20px of bezel overscan.
  if(px< -20||px>=90||py< -20||py>=70)return false;
  x=(uint16_t)(px<0?0:px);
  y=(uint16_t)(py<0?0:py);
  ++portraitMenuRawRescues;
  Serial.printf("CYD32,TOUCH,MENU_EDGE_RECOVERED,x=%u,y=%u,raw=%u:%u,z_threshold=230,count=%lu\n",
     (unsigned)x,(unsigned)y,(unsigned)rx,(unsigned)ry,
     (unsigned long)portraitMenuRawRescues);
  return true;
}
Cyd32UiAction portraitTap(int x,int y,bool active,bool suite){
  // Physical 3.2" resistive touch benefits from a larger menu target.
  // Keep top-right rotation separate from the Quick Filter's y>=29
  // control and don't intercept its text row.
  if(y<70&&x<90){
    screen=Screen::MainMenu;menuPage=0;openedFromMainMenu=false;quickFromCategories=false;settingsSubmenuFlow=false;redraw=true;
    if(portraitTouchTrace)Serial.printf("CYD32,TOUCH,MENU_OPEN,x=%d,y=%d\n",x,y);
    return Cyd32UiAction::None;
  }
  if(y<30&&x>=190){
    rotateCurrentScreen();
    return Cyd32UiAction::None;
  }
  if(y<27)return Cyd32UiAction::None;
  if(screen==Screen::Home){
    // 240px portrait uses the same SquachWatch edge gesture (10%).
    if(y>=60&&y<215&&(x<24||x>=216)){
      cycleBackground(x<24?-1:+1);return Cyd32UiAction::None;
    }
    if(y>=287){
      if(x<80){openedFromMainMenu=false;quickFromCategories=false;screen=Screen::Categories;redraw=true;return Cyd32UiAction::None;}
      if(x<160)return active||suite||playlistRunning?Cyd32UiAction::Stop:Cyd32UiAction::Run;
      return Cyd32UiAction::Suite;
    }
    if(y>=80&&y<204){petUntil=millis()+1600;redraw=true;}
    else if(y>=240&&y<263){screen=Screen::Details;redraw=true;}
    else if(y>=263&&y<287){screen=Screen::Duration;redraw=true;}
    return Cyd32UiAction::None;
  }
  if(screen==Screen::QuickFilter&&y>=29&&y<65&&x>=170){
    filterLetter=(uint8_t)((filterLetter+1)%27);filterPage=0;redraw=true;
    return Cyd32UiAction::None;
  }
  if(screen==Screen::Favorites&&y>=29&&y<65&&x>=145&&favoriteCount()){
    return Cyd32UiAction::PlaylistStart;
  }
  if(screen==Screen::BatteryCalibration){
    if(y>=287){
      if(x<80){backFromSettingsSubmenu();}
      else if(x>=160)setBatteryDisplayVolts(!batteryDisplayVolts);
      else if(active||suite||playlistRunning)return Cyd32UiAction::Stop;
      else setBatteryOffsetMv(0);
    }else if(y>=163&&y<261){
      const int increment=y<212?10:50;
      setBatteryOffsetMv((int32_t)batteryOffsetMv+(x<120?-increment:increment));
    }
    return Cyd32UiAction::None;
  }
  if(y>=287){
    // A transmitting device must have accessible STOP even when
    // looking at portrait menus, categories, or settings.
    if(x>=80&&x<160&&(active||suite||playlistRunning))
      return Cyd32UiAction::Stop;
    if(x<80){
      // BACK has the same destination as the corresponding landscape page.
      if(screen==Screen::MainMenu||screen==Screen::Backgrounds||
         screen==Screen::Outfits||screen==Screen::OtaInfo){
        screen=screen==Screen::MainMenu?Screen::Home:Screen::MainMenu;
        redraw=true;return Cyd32UiAction::None;
      }
      if(screen==Screen::Categories)return touchCategories(50,218);
      if(screen==Screen::Library)return touchLibrary(50,218,active);
      if(screen==Screen::Favorites)return touchFavorites(50,218);
      if(screen==Screen::History)return touchHistory(50,218);
      if(screen==Screen::QuickFilter)return touchQuickFilter(50,218);
      if(screen==Screen::Details)return touchDetails(50,218,active);
      if(screen==Screen::Diagnostics)return touchDiagnostics(50,218);
      if(screen==Screen::Activity)return touchActivity(50,218);
      if(screen==Screen::Evidence)return touchEvidence(50,218);
      if(screen==Screen::Settings)return touchSettings(50,218);
      if(screen==Screen::Identity)return touchIdentity(50,218);
      if(screen==Screen::Recovery)return touchRecovery(50,218);
      if(screen==Screen::Help)return touchHelp(50,218);
      if(screen==Screen::Duration)return touchDuration(50,218);
      screen=Screen::MainMenu;redraw=true;return Cyd32UiAction::None;
    }
    const int px=x<160?150:250;
    if(screen==Screen::MainMenu){
      if(x<160){if(menuPage)--menuPage;}else if(menuPage<2)++menuPage;
      redraw=true;return Cyd32UiAction::None;
    }
    if(screen==Screen::Categories)return touchCategories(px,218);
    if(screen==Screen::Library)return touchLibrary(px,218,active);
    if(screen==Screen::Favorites)return touchFavorites(px,218);
    if(screen==Screen::History)return touchHistory(px,218);
    if(screen==Screen::QuickFilter)return touchQuickFilter(px,218);
    if(screen==Screen::Settings)return touchSettings(px,218);
    if(screen==Screen::Backgrounds){
      if(x<160){if(backgroundPage)--backgroundPage;}
      else if(backgroundPage<2)++backgroundPage;
      redraw=true;return Cyd32UiAction::None;
    }
    if(screen==Screen::Outfits){
      if(x<160){if(outfitPage)--outfitPage;}
      else if(outfitPage<5)++outfitPage;
      redraw=true;return Cyd32UiAction::None;
    }
    if(screen==Screen::Duration)return touchDuration(px,218);
    if(screen==Screen::Details)return touchDetails(px,218,active);
    if(screen==Screen::Diagnostics)return touchDiagnostics(px,218);
    if(screen==Screen::Activity)return touchActivity(px,218);
    if(screen==Screen::Evidence)return touchEvidence(px,218);
    if(screen==Screen::Identity)return touchIdentity(px,218);
    if(screen==Screen::Recovery)return touchRecovery(px,218);
    if(screen==Screen::Help)return touchHelp(px,218);
    screen=Screen::MainMenu;redraw=true;return Cyd32UiAction::None;
  }
  if(y<65||y>=261)return Cyd32UiAction::None;
  const int i=(y-65)/49;
  if(screen==Screen::MainMenu){
    const int n=menuPage*4+i;
    if(n<kMenuEntries){
      static const Screen targets[]={Screen::Categories,Screen::QuickFilter,
        Screen::Favorites,Screen::History,Screen::Diagnostics,Screen::Settings,
        Screen::Outfits,Screen::OtaInfo,Screen::Activity,Screen::Help,
        Screen::Recovery,Screen::Backgrounds};
      openedFromMainMenu=true;quickFromCategories=false;settingsSubmenuFlow=false;screen=targets[n];redraw=true;
    }
  }else if(screen==Screen::Categories)return touchCategories(150,48+i*39);
  else if(screen==Screen::Library)return touchLibrary(150,53+i*36,active);
  else if(screen==Screen::Favorites)return touchFavorites(150,50+i*37);
  else if(screen==Screen::QuickFilter)return touchQuickFilter(150,68+i*34);
  else if(screen==Screen::Settings){
    if(settingsMore&&i==3){
      calibratePortraitFromDevice();
    }else return touchSettings(150,50+i*39);
  }else if(screen==Screen::Outfits){
    const int n=outfitPage*4+i;
    if(n<kOutfitCount){outfitLevel=(uint8_t)n;
      if(prefs.begin("sqemit32",false)){prefs.putUChar("outfit",outfitLevel);prefs.end();}
      redraw=true;}
  }else if(screen==Screen::Backgrounds){
    const int n=backgroundPage*4+i;
    if(n<10)selectBackground((uint8_t)n);
  }else if(screen==Screen::Duration){
    setDuration(i==0?3:i==1?10:i==2?30:(durationSeconds<30?durationSeconds+1:30));
    redraw=true;
  }else if(screen==Screen::Recovery){
    if(i==2)calibratePortraitFromDevice();
  }else if(screen==Screen::Details){
    if(i==2)toggleFavorite();
    if(i==3){screen=Screen::Duration;redraw=true;}
  }
  return Cyd32UiAction::None;
}
void rgbBars(){
  colorBarsUntil=millis()+7000;
  lcd.fillScreen(kBg);
  titleBar();
  label(24,25,"ST7798 RGB565 TEST",kCyan,2);
  const uint16_t colors[]={TFT_RED,TFT_GREEN,TFT_BLUE,TFT_WHITE,TFT_BLACK};
  const char* names[]={"RED","GREEN","BLUE","WHITE","BLACK"};
  for(uint8_t i=0;i<5;++i){
    const int x=7+62*i;
    lcd.fillRect(x,62,56,84,colors[i]);
    lcd.drawRect(x,62,56,84,kPurple);
    label(x,157,names[i],kWhite);
  }
  label(11,193,"TAP TO EXIT  /  7S AUTO",kVaporPink);
}
} // namespace

void cyd32UiNoteRunAction(){
  // Legacy UI hook retained for caller compatibility. R14.17 speech is
  // strictly state-scoped: repeated RUN taps never inject extra messages.
}
void cyd32UiSetRadioStarting(const char* scenarioId){
  radioStartError[0]='\0';
  snprintf(radioStartingProfile,sizeof(radioStartingProfile),"%s",
           scenarioId?scenarioId:"");
  redraw=true;
}
void cyd32UiSetRadioError(const char* stage){
  radioStartingProfile[0]='\0';
  snprintf(radioStartError,sizeof(radioStartError),"%s",stage?stage:"");
  redraw=true;
}
void cyd32UiBegin(){
  bool savedPortrait=false;
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,LOW);
  lcd.init();
  lcd.setRotation(1); // 320x240 landscape
  // From SquachWatch's real CYD32: ST7798-compatible, BGR, inversion OFF.
  // TFT inversion needs an absolute setting after setRotation().
  panelInverted=true; // physically observed E32R32P board requires inversion ON
  if(prefs.begin("sqemit32",true)){
    // New key avoids inheriting R3's incorrect "invert=false" default.
    panelInverted=prefs.getBool("invert2",true);
    durationSeconds=prefs.getUChar("txtime",12);
    brightnessLevel=prefs.getUChar("bright",2);
    animationLevel=prefs.getUChar("anim",1);
    screenTimeout=prefs.getUChar("timeout",0);
    themeLevel=prefs.getUChar("theme",0);
    fontMode=prefs.getUChar("font",1);
    batteryOffsetMv=SqeBatteryCalibration::clampOffset(prefs.getInt("batoff_mv",0));
    batteryDisplayVolts=prefs.getBool("batshow_v",false);
    savedPortrait=prefs.getBool("portrait",false);
     outfitLevel=prefs.getUChar("outfit",0);
     if(prefs.isKey("bg_sqwatch")){
        backgroundLevel=prefs.getUChar("bg_sqwatch",9);
      }else{
        // Migrate the old R10 catalog to the upstream selection order.
        static const uint8_t oldToUpstream[]={9,0,1,5,6,7,3,4,2,8};
        const uint8_t previous=prefs.getUChar("bg",0);
        backgroundLevel=previous<10?oldToUpstream[previous]:9;
      }
    prefs.end();
  }
  if(durationSeconds<3 || durationSeconds>30)durationSeconds=12;
  if(brightnessLevel>2)brightnessLevel=2;
  if(animationLevel>2)animationLevel=1;
  if(screenTimeout>2)screenTimeout=0;
  if(themeLevel>2)themeLevel=0;
  if(fontMode>1)fontMode=1;
  if(outfitLevel>21)outfitLevel=0;
  if(backgroundLevel>9)backgroundLevel=9;
  applyTheme();
#if defined(SQE_ORIGINAL_SQUACHY)
  // Keep exactly one speech UI: SquachEmit's top panel. SquachWatch's
  // own bubble remains suppressed until the user leaves this firmware.
  Squachy::holdBubble(true);
#endif
  if(prefs.begin("sqemitfav",true)){
    if(prefs.getBytesLength("fav")==sizeof(favBits))prefs.getBytes("fav",favBits,sizeof(favBits));
    prefs.end();
  }
  lcd.invertDisplay(panelInverted);
  lcd.fillScreen(kBg);
  // Ported SquachWatch fast SPI push; safe disable via serial PUSH OFF.
  if(Cyd32FramePush::begin())Cyd32FramePush::setEnabled(fastPushDesired);
  else fastPushDesired=false;
  // The original character renders directly into the completed FULL8/HALF8
  // scene buffer. No extra 8-bit scratch tile or magenta transparency key.
  allocateSceneFrame();
#if !defined(SQE_ORIGINAL_SQUACHY)
  squachSprite.setColorDepth(8);
  squachSpriteReady=(squachSprite.createSprite(112,111)!=nullptr);
  if(!squachSpriteReady)Serial.println("WARN,CYD32_SPRITE_ALLOC;STATIC_FALLBACK");
#endif
  ledcSetup(6,5000,8);
  ledcAttachPin(TFT_BL,6);
  screenDimmed=false;applyBrightness();
  lastTouchActivity=millis();
  calibrated=loadCal();
  if(calibrated&&savedPortrait)rotateCurrentScreen();
  ready=true;
  redraw=true;
  Serial.printf("CYD32,PANEL,BGR,INVERT_%s\n",panelInverted?"ON":"OFF");
  Serial.println(calibrated?"CYD32,UI,CLEAR_TOUCH_SAVED":"CYD32,UI,CLEAR_TOUCH_SETUP");
}
uint32_t cyd32UiDurationMs(){return (uint32_t)durationSeconds*1000UL;}
const Scenario* cyd32UiSelection(){
  return selected<kScenarioCount?&kScenarios[selected]:nullptr;
}
void cyd32UiSetInversion(bool enabled){
  panelInverted=enabled;
  lcd.invertDisplay(enabled);
  if(prefs.begin("sqemit32",false)){
    prefs.putBool("invert2",enabled);
    prefs.end();
  }
  redraw=true;
  Serial.printf("CYD32,PANEL,BGR,INVERT_%s\n",enabled?"ON":"OFF");
}
bool cyd32UiInversion(){return panelInverted;}
void cyd32UiColorBars(){if(ready){Cyd32FramePush::invalidate();rgbBars();}}
// Serial-only A/B for the exact same screen and ST7798 clock. These
// toggles are deliberately not persisted so a reboot restores the default.
void cyd32UiPrintFramePushStatus(){
  Serial.printf("CYD32,PUSH,STATUS,requested=%u,available=%u,active=%u,frame=%s,last_mode=%s,rows=%lu,hash_us=%lu,wire_us=%lu,fallbacks=%lu\n",
    fastPushDesired?1u:0u,Cyd32FramePush::available()?1u:0u,
    (sceneFrameBands==1&&fastPushDesired&&Cyd32FramePush::enabled())?1u:0u,
    bandFrameReady?(sceneFrameBands==1?"FULL8":"HALF8"):"STATIC",
    fastPushLastUsed?"FAST":"STANDARD",
    (unsigned long)fastPushLastRows,(unsigned long)fastPushLastHashUs,
    (unsigned long)fastPushLastWireUs,(unsigned long)fastPushFallbacks);
}
void cyd32UiSetFramePush(bool enabled){
  fastPushDesired=enabled&&Cyd32FramePush::available();
  Cyd32FramePush::setEnabled(fastPushDesired);
  // A/B transitions must never use stale row hashes after ordinary
  // TFT_eSPI pushes, especially when switching mid-animation.
  Cyd32FramePush::invalidate();
  fastPushLastUsed=false;
  fastPushLastRows=0;fastPushLastHashUs=0;fastPushLastWireUs=0;
  Serial.printf("CYD32,PUSH,%s\n",fastPushDesired?"ON":"OFF");
  cyd32UiPrintFramePushStatus();
}
void cyd32UiSetTouchTrace(bool enabled){
  portraitTouchTrace=enabled;
  lastPortraitTouchTraceAt=0;
  Serial.printf("CYD32,TOUCH,TRACE,%s\n",enabled?"ON":"OFF");
}
void cyd32UiPrintTouchStatus(){
  Serial.printf("CYD32,TOUCH,STATUS,orientation=%s,portrait_saved=%u,landscape_saved=%u,trace=%u,rescues=%lu,cal=%u:%u:%u:%u:%u,affine5=%u\n",
    portraitMode?"PORTRAIT":"LANDSCAPE",portraitCalibrated?1u:0u,
    calibrated?1u:0u,portraitTouchTrace?1u:0u,
    (unsigned long)portraitMenuRawRescues,
    (unsigned)portraitTouchData[0],(unsigned)portraitTouchData[1],
    (unsigned)portraitTouchData[2],(unsigned)portraitTouchData[3],
    (unsigned)portraitTouchData[4],portraitFitLoaded?1u:0u);
}
void cyd32UiCalibratePortraitTouch(){
  if(!ready||!calibrated||!portraitMode){
    Serial.println("ERROR,CALTOUCH_PORTRAIT_REQUIRES_PORTRAIT");
    return;
  }
  // The original four-corner TFT_eSPI wizard can consume the same finger
  // at two adjacent targets and save a broken calibration. Use the exact
  // SquachWatch five-target held-sample + release + proof-dot workflow.
  Serial.println("CYD32,TOUCH,PORTRAIT_CAL_START");
  const bool ok=runPortraitFivePointWizard();
  Serial.println(ok?"CYD32,TOUCH,PORTRAIT_CAL_DONE":
                    "CYD32,TOUCH,PORTRAIT_CAL_FAILED_OLD_FIT_PRESERVED");
  portraitRawHoldStarted=0;
  if(!ok)redraw=true;
  cyd32UiPrintTouchStatus();
}
void cyd32UiResetCalibration(){
  if(prefs.begin("sqemit32",false)){
    prefs.remove("touchland");
    prefs.end();
  }
  calibrated=false;
  wasPressed=false;
  redraw=true;
  Serial.println("CYD32,TOUCH,RESET_LANDSCAPE;TAP_TO_CALIBRATE");
}
Cyd32UiAction cyd32UiTick(const char* active,uint32_t remaining,bool suite){
  if(!ready)return Cyd32UiAction::None;
  const uint32_t now=millis();
  const uint32_t priorBatterySample=lastBatterySample;
  const int8_t priorBattery=batteryPercent;
  const int prevBatteryVoltageStep=batteryDisplayTick();
  sampleBattery();
#if defined(ARDUINO_ARCH_ESP32)
  const time_t clockNow=time(nullptr);
  if(clockNow>1704067200){
    const uint32_t minute=(uint32_t)(clockNow/60);
    if(lastClockMinute!=minute){lastClockMinute=minute;redraw=true;}
  }
#endif
  const int nextBatteryVoltageStep=batteryDisplayTick();
  if(priorBattery!=batteryPercent ||
     prevBatteryVoltageStep!=nextBatteryVoltageStep ||
     (screen==Screen::BatteryCalibration&&priorBatterySample!=lastBatterySample))
    redraw=true;
  if(colorBarsUntil){
    if((int32_t)(now-colorBarsUntil)>=0 || (active&&*active)){
      colorBarsUntil=0;redraw=true;
    }else{
      uint16_t x=0,y=0;
      if(lcd.getTouch(&x,&y,450)){colorBarsUntil=0;redraw=true;}
      else return Cyd32UiAction::None;
    }
  }
  const char* name=active?active:"";
  // Landscape keeps the established auto-return-to-HOME during TX.
  // Portrait has an always-visible STOP footer, so don't force users out
  // of the menu every tick: that made the menu appear unresponsive.
  if(((active&&*active)||suite)&&screen!=Screen::Home&&!portraitMode){
    screen=Screen::Home;
    redraw=true;
  }
  if(onAirIndicator!=(active&&*active)) {onAirIndicator=active&&*active;redraw=true;}
  if(strcmp(previousActive,name)!=0||previousSuite!=suite){
    strncpy(previousActive,name,sizeof(previousActive)-1);
    previousActive[sizeof(previousActive)-1]='\0';
    previousSuite=suite;
    redraw=true;
  }
  // Advance state once per logical UI loop, never per sprite band.
  advanceLandscapeBubble(now,active,suite);
  if(redraw){
    // Non-HOME screens and calibration draw directly on the TFT, outside
    // FramePush's hash ledger. Force a full refresh on return to HOME.
    if(!(calibrated&&screen==Screen::Home&&bandFrameReady))
      Cyd32FramePush::invalidate();
    if(!calibrated)calibrationScreen();
    else if(screen==Screen::Home&&bandFrameReady)
      renderBufferedHome(active,remaining,suite);
    else if(portraitMode)portraitPage(active,remaining,suite);
    else if(screen==Screen::Home)homeScreen(active,remaining,suite);
    else if(screen==Screen::Categories)categoriesScreen();
    else if(screen==Screen::Duration)durationScreen();
    else if(screen==Screen::Details)detailScreen();
    else if(screen==Screen::Diagnostics)diagnosticsScreen();
    else if(screen==Screen::Favorites)favoritesScreen();
    else if(screen==Screen::History)historyScreen();
    else if(screen==Screen::Settings)settingsScreen();
    else if(screen==Screen::BatteryCalibration)batteryCalibrationScreen();
    else if(screen==Screen::Help)helpScreen();
    else if(screen==Screen::Identity)identityScreen();
    else if(screen==Screen::Recovery)recoveryScreen();
    else if(screen==Screen::Activity)activityScreen();
    else if(screen==Screen::Evidence)evidenceScreen();
    else if(screen==Screen::QuickFilter)quickFilterScreen();
    else if(screen==Screen::Portrait)portraitScreen(active,remaining,suite);
    else if(screen==Screen::MainMenu)mainMenuScreen();
    else if(screen==Screen::Outfits)outfitScreen();
    else if(screen==Screen::Backgrounds)backgroundsScreen();
    else if(screen==Screen::OtaInfo)otaInfoScreen();
    else libraryScreen(active,suite);
    lastCount=(active&&*active)?((remaining+999)/1000):UINT32_MAX;
    portraitLastCount=(active&&*active)?((remaining+999)/1000):UINT32_MAX;
    redraw=false;
  }else if(calibrated&&screen==Screen::Home&&active&&*active&&
           !bandFrameReady&&!portraitMode){
    // STATIC_FALLBACK only. Buffered FULL8/HALF8 renders countdown
    // text together with the scene; an additional direct TFT countdown
    // scribbles over that frame and causes portrait/landscape flicker.
    const uint32_t count=(remaining+999)/1000;
    if(count!=lastCount){
      Cyd32FramePush::invalidate();
      countdownOnly(count);lastCount=count;
    }
  }
  if(calibrated&&screen==Screen::Diagnostics&&now-lastDiagAt>=1000u){
    Cyd32FramePush::invalidate();diagValues();lastDiagAt=now;
  }
  // A timed, random bubble is assembled WITH the complete buffered frame.
  // Never paint it directly to the ST7798 over a running FramePush frame.
  if(calibrated&&portraitMode&&screen==Screen::Home&&
     active&&*active&&!bandFrameReady){
    // No sprite available: update only the portrait TX text, in exactly
    // the same position/font as portraitPage(). When FULL8/HALF8 is
    // available portraitPage() owns every animation and countdown pixel.
    const uint32_t count=(remaining+999)/1000;
    if(count!=portraitLastCount){
      Cyd32FramePush::invalidate();
      lcd.fillRect(10,265,225,19,kBg);
      char status[42];
      snprintf(status,sizeof(status),"TX %lus LEFT",(unsigned long)count);
      fit(12,267,status,216,kGreen);
      portraitLastCount=count;
    }
  }
  if(screenTimeout&&!pcControl&&!(active&&*active)&&!suite && !screenDimmed &&
      now-lastTouchActivity>=(screenTimeout==1?30000UL:60000UL)){
    screenDimmed=true;applyBrightness();
  }
  const uint32_t animDelay=animationLevel==0?200u:animationLevel==1?120u:80u;
  // R13: all motion is rendered into SquachWatch-style half-height 8-bit
  // bands first; only completed bands are submitted to the LCD.
  // If memory pressure prevents band allocation, retain R12's static
  // no-flicker fallback instead of direct-to-panel erase/redraw.
  // SquachWatch's original scene engine aims at a ~50ms logical frame;
  // the R14.2 130ms NORMAL throttle alone limited visible smoothness.
  // Actual throughput depends on paint and SPI time (logged below).
  // Match SquachWatch's 33 ms visual pacing in NORMAL mode.
  // Faster rendering is opportunistic: a two-band draw/push may take longer.
  const uint32_t frameCadence=animationLevel==0?100u:animationLevel==1?33u:25u;
  if(calibrated&&screen==Screen::Home&&bandFrameReady&&
     now-lastSceneFrame>=frameCadence&&colorBarsUntil==0){
    renderBufferedHome(active,remaining,suite);
  }
  if(now-lastPoll<55)return Cyd32UiAction::None;
  lastPoll=now;
  if(!calibrated){
    // TFT_eSPI calibration wizard blocks. Never allow while RF is active.
    if((active&&*active)||suite)return Cyd32UiAction::None;
    uint16_t rx=0,ry=0;
    const bool pressed=lcd.getTouchRaw(&rx,&ry);
    if(pressed&&!wasPressed&&now-lastPress>350){
      lastPress=now;startCal();
    }
    wasPressed=pressed;
    return Cyd32UiAction::None;
  }
  uint16_t x=0,y=0;
  bool pressed=false;
  if(portraitMode&&portraitFitLoaded){
    // Match SquachWatch CYD32 precisely: use pressure-gated raw ADC
    // values + a saved affine mapping, rather than TFT_eSPI's rotating
    // min/span mapping (which silently drops bezel coordinates).
    int16_t a=0,b=0;
    if(readPortraitRaw(a,b)){
      float sx=0,sy=0;
      TouchFit::toScreen(portraitFit,a,b,0,sx,sy);
      // Same bounded extrapolation and edge clipping as SquachWatch.
      const int32_t px=(int32_t)sx,py=(int32_t)sy;
      if(isfinite(sx)&&isfinite(sy)&&px>-240&&px<480&&py>-320&&py<640){
        x=(uint16_t)(px<0?0:px>=240?239:px);
        y=(uint16_t)(py<0?0:py>=320?319:py);
        pressed=true;
      }
    }
#if !defined(ARDUINO_ARCH_ESP32)
    // Native fixture injects calibrated touch coordinates directly. On
    // real CYD3.2 hardware only raw XPT2046 affine mapping is accepted.
    if(!pressed)pressed=lcd.getTouch(&x,&y,260);
#endif
  }else{
    // Preserve the physically validated landscape touch mapping.
    pressed=lcd.getTouch(&x,&y,450);
    if(portraitMode&&!pressed)pressed=recoverPortraitMenuTouch(x,y);
  }
  // Raw-hold recovery never trusts an incorrect calibrated x/y. Crucial:
  // wait for the initiating finger to LIFT before starting any wizard,
  // otherwise the old press may be consumed as a target (original bug).
  const bool rfBusy=onAirIndicator||previousSuite||playlistRunning||txDriverActive;
  if(portraitCalibrationPending){
    if(rfBusy||!portraitMode){portraitCalibrationPending=false;return Cyd32UiAction::None;}
    if(panel.getTouchRawZ()<230){
      portraitCalibrationPending=false;
      Serial.println("CYD32,TOUCH,RAW_HOLD_RELEASED_BEGIN_5_POINT");
      cyd32UiCalibratePortraitTouch();
    }
    return Cyd32UiAction::None;
  }
  if(portraitMode&&!remoteDown&&!rfBusy&&panel.getTouchRawZ()>=230){
    if(!portraitRawHoldStarted)portraitRawHoldStarted=now?now:1u;
    else if(now-portraitRawHoldStarted>=5000u){
      portraitRawHoldStarted=0;
      portraitCalibrationPending=true;
      Serial.println("CYD32,TOUCH,RAW_HOLD_CAL;RELEASE_TO_START_5_POINT");
      return Cyd32UiAction::None;
    }
  }else portraitRawHoldStarted=0;
  if(portraitTouchTrace&&now-lastPortraitTouchTraceAt>=180u){
    const uint16_t z=panel.getTouchRawZ();
    if(pressed||z>=160){
      uint16_t rx=0,ry=0;
      panel.getTouchRaw(&rx,&ry);
      Serial.printf("CYD32,TOUCH,TRACE,rot=%s,accepted=%u,x=%u,y=%u,raw=%u:%u,z=%u,menu=%u\n",
         portraitMode?"PORTRAIT":"LANDSCAPE",pressed?1u:0u,
         (unsigned)x,(unsigned)y,(unsigned)rx,(unsigned)ry,
         (unsigned)z,(pressed&&portraitMode&&x<90&&y<70)?1u:0u);
      lastPortraitTouchTraceAt=now;
    }
  }
  if(remoteDown){
    if(now-remoteAt>4000u)remoteDown=false;
    else {x=remoteX;y=remoteY;pressed=true;}
  }
  Cyd32UiAction action=Cyd32UiAction::None;
  if(pressed){
    lastTouchActivity=now;
    if(screenDimmed){screenDimmed=false;applyBrightness();wasPressed=true;
      return Cyd32UiAction::None;}
  }
  if(pressed&&!wasPressed&&now-lastPress>=240&&
     x<(portraitMode?240:kW)&&y<(portraitMode?320:kH)){
    if(portraitMode){
      action=portraitTap(x,y,active&&*active,suite);
    }else if(y<23&&x>=275){
      rotateCurrentScreen();
    }else if(y<23&&x<48){
      screen=Screen::MainMenu;menuPage=0;openedFromMainMenu=false;quickFromCategories=false;settingsSubmenuFlow=false;redraw=true;
    }else if(screen==Screen::MainMenu){
      if(y>=kFooterY){
        if(x<108)screen=Screen::Home;
        else if(x<213){if(menuPage)--menuPage;}
        else if(menuPage<2)++menuPage;
        redraw=true;
      }else if(y>=46&&y<202){
        const uint8_t n=menuPage*4+(uint8_t)((y-46)/39);
        if(n<kMenuEntries){
          const Screen targets[]={Screen::Categories,Screen::QuickFilter,
            Screen::Favorites,Screen::History,Screen::Diagnostics,
            Screen::Settings,Screen::Outfits,Screen::OtaInfo,
            Screen::Activity,Screen::Help,Screen::Recovery,Screen::Backgrounds};
          openedFromMainMenu=true;quickFromCategories=false;settingsSubmenuFlow=false;screen=targets[n];redraw=true;
          if(screen==Screen::Favorites)favoritesPage=0;
          if(screen==Screen::History)historyPage=0;
          if(screen==Screen::QuickFilter)filterPage=0;
        }
      }
    }else if(screen==Screen::Outfits){
      if(y>=kFooterY){
        if(x<108)screen=Screen::MainMenu;
        else if(x<213){if(outfitPage)--outfitPage;}
        else if(outfitPage<5)++outfitPage;
        redraw=true;
      }else if(y>=46&&y<202){
        const uint8_t n=outfitPage*4+(uint8_t)((y-46)/39);
        if(n<kOutfitCount){
          outfitLevel=n;
          if(prefs.begin("sqemit32",false)){
            prefs.putUChar("outfit",outfitLevel);prefs.end();
          }
          redraw=true;
        }
      }
    }else if(screen==Screen::Backgrounds){
      if(y>=kFooterY){
        if(x<108)screen=Screen::MainMenu;
        else if(x<213){if(backgroundPage)--backgroundPage;}
        else if(backgroundPage<2)++backgroundPage;
        redraw=true;
      }else if(y>=46&&y<202){
        const uint8_t n=backgroundPage*4+(uint8_t)((y-46)/39);
        if(n<10){
          selectBackground(n);
        }
      }
    }else if(screen==Screen::OtaInfo){
      if(y>=kFooterY){
        screen=x<108?Screen::MainMenu:x<213?Screen::Help:Screen::Home;
        redraw=true;
      }
    }else if(screen==Screen::Home)action=touchHome(x,y,active&&*active,suite);
    else if(screen==Screen::Categories)action=touchCategories(x,y);
    else if(screen==Screen::Duration)action=touchDuration(x,y);
    else if(screen==Screen::Details)action=touchDetails(x,y,active&&*active);
    else if(screen==Screen::Diagnostics)action=touchDiagnostics(x,y);
    else if(screen==Screen::Favorites)action=touchFavorites(x,y);
    else if(screen==Screen::History)action=touchHistory(x,y);
    else if(screen==Screen::Settings)action=touchSettings(x,y);
    else if(screen==Screen::BatteryCalibration)action=touchBatteryCalibration(x,y);
    else if(screen==Screen::Help)action=touchHelp(x,y);
    else if(screen==Screen::Identity)action=touchIdentity(x,y);
    else if(screen==Screen::Recovery)action=touchRecovery(x,y);
    else if(screen==Screen::Activity)action=touchActivity(x,y);
    else if(screen==Screen::Evidence)action=touchEvidence(x,y);
    else if(screen==Screen::QuickFilter)action=touchQuickFilter(x,y);
    else if(screen==Screen::Portrait)action=touchPortrait(x,y,active&&*active,suite);
    else action=touchLibrary(x,y,active&&*active);
    lastPress=now;
    // Only navigation/state changes request a full repaint. Every other
    // touch (including Squachy and duration +/-) stays flicker-free.
  }
  wasPressed=pressed;
  return action;
}

// Small view-model protocol; the Windows app draws its own canvas from
// the same logical navigation state as the physical TFT UI. This function
// NEVER reads pixels, invokes readRect, or transmits a screenshot.
void cyd32UiStreamState(const char* activeId,uint32_t remainingMs,
                        bool suiteRunning,bool force){
  if(!ready)return;
  static uint32_t sentAt=0;
  const uint32_t now=millis();
  if(!force&&now-sentAt<180u)return;
  sentAt=now;
  const char* screenName=
      !calibrated?"CALIBRATION":
      colorBarsUntil?"COLORS":
      screen==Screen::Categories?"CATEGORIES":
      screen==Screen::Library?"LIBRARY":
      screen==Screen::Duration?"DURATION":
      screen==Screen::Details?"DETAILS":
      screen==Screen::Diagnostics?"DIAGNOSTICS":
      screen==Screen::Favorites?"FAVORITES":
      screen==Screen::History?"HISTORY":
      screen==Screen::Settings?"SETTINGS":
       screen==Screen::BatteryCalibration?"BATCAL":
      screen==Screen::Help?"HELP":
      screen==Screen::Identity?"IDENTITY":
      screen==Screen::Recovery?"RECOVERY":
      screen==Screen::Activity?"ACTIVITY":
      screen==Screen::Evidence?"EVIDENCE":
      screen==Screen::QuickFilter?"QUICKFILTER":
      screen==Screen::Portrait?"PORTRAIT":
       screen==Screen::MainMenu?"MENU":
       screen==Screen::Outfits?"OUTFITS":
       screen==Screen::OtaInfo?"OTA":
       screen==Screen::Backgrounds?"BACKGROUNDS":"HOME";
  const Scenario* chosen=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  const uint32_t petMs=(int32_t)(petUntil-now)>0?petUntil-now:0;
  Serial.printf("UI STATE 1 %s %u %u %u %s %u %s %lu %u %u %s %s %lu %lu\n",
    screenName,(unsigned)static_cast<uint8_t>(category),
    (unsigned)(screen==Screen::Favorites?favoritesPage:screen==Screen::History?historyPage:page),
    (unsigned)categoryPage,
    chosen?chosen->id:"NONE",(unsigned)durationSeconds,
    activeId&&*activeId?activeId:"IDLE",
    (unsigned long)remainingMs,suiteRunning?1u:0u,calibrated?1u:0u,
    radioStartingProfile[0]?radioStartingProfile:"NONE",
    radioStartError[0]?radioStartError:"NONE",
    (unsigned long)petMs,(unsigned long)now);
  if((force||now-lastListsAt>=1000u)&&playlistRunning){
    Serial.printf("UI PLAYLIST 1 %u %u %u %u\n",
      playlistRunning?1u:0u,playlistPaused?1u:0u,playlistIndex,playlistTotal);
  }
  if(screen==Screen::Details&&(force||now-lastListsAt>=1000u)){
    lastListsAt=now;
    const bool fav=chosen&&((favBits[selected/8]>>(selected%8))&1u);
    Serial.printf("UI FAVORITE 1 %s %u\n",chosen?chosen->id:"NONE",fav?1u:0u);
  }
  if(screen==Screen::Favorites&& (force||now-lastListsAt>=1000u)){
    lastListsAt=now;
    Serial.printf("UI FAVS BEGIN %u %u\n",(unsigned)favoriteCount(),(unsigned)favoritesPage);
    for(size_t i=0;i<4;++i){
      const Scenario* c=favoriteAt(favoritesPage*4+i);
      if(c)Serial.printf("UI FAVS ROW %u %s\n",(unsigned)i,c->id);
    }
    Serial.println("UI FAVS END");
  }
  if(screen==Screen::History&&(force||now-lastListsAt>=1000u)){
    lastListsAt=now;
    Serial.printf("UI HISTORY BEGIN %u %u\n",(unsigned)historyCount,(unsigned)historyPage);
    for(size_t i=0;i<4;++i){
      const HistoryEntry* h=historyAt(historyPage*4+i);
      if(h)Serial.printf("UI HISTORY ROW %u %s %s %lu\n",(unsigned)i,h->id,h->status,(unsigned long)h->uptime);
    }
    Serial.println("UI HISTORY END");
  }
  if((screen==Screen::Settings||screen==Screen::Help||screen==Screen::Recovery)&&
     (force||now-lastDeviceAt>=1000u)){
    lastDeviceAt=now;
    Serial.printf("UI DEVICE 1 %u %u %u %u %u %u %u %u %u %u\n",
      brightnessLevel,animationLevel,screenTimeout,panelInverted?1u:0u,
      pcControl?1u:0u,resetReasonCode,(unsigned)helpPage,
      (unsigned)themeLevel,(unsigned)fontMode,settingsMore?1u:0u);
  }
  if(screen==Screen::Identity&&(force||now-lastDeviceAt>=1000u)){
    lastDeviceAt=now;
    const bool matches=chosen&&strcmp(identityCase,chosen->id)==0&&identityMac[0];
    Serial.printf("UI IDENTITY 1 %s %s\n",chosen?chosen->id:"NONE",
      matches?identityMac:"UNKNOWN");
  }
  if((screen==Screen::Home||screen==Screen::Activity)&&(force||now-lastActivityAt>=1000u)){
    lastActivityAt=now;
    Serial.printf("UI TXACT 1 %lu %lu %lu %u %lu\n",
      (unsigned long)txBegins,(unsigned long)txEnds,(unsigned long)txErrors,
      txDriverActive?1u:0u,(unsigned long)actualTxDuration);
  }
  if(screen==Screen::Evidence&&(force||now-lastEvidenceAt>=1000u)){
    lastEvidenceAt=now;
    Serial.printf("UI RXREPORT 1 %s %s %d %s\n",
      rxId[0]?rxId:"NONE",rxLabel[0]?rxLabel:"NONE",rxRssi,
      rxQuality[0]?rxQuality:"UNVERIFIED");
  }
  if(screen==Screen::QuickFilter&&(force||now-lastFilterAt>=1000u)){
    lastFilterAt=now;
    Serial.printf("UI FILTER BEGIN %u %u %u\n",(unsigned)filterLetter,(unsigned)filterPage,(unsigned)filterCount());
    for(size_t i=0;i<4;++i){
      const Scenario* item=filterAt(filterPage*4+i);
      if(item)Serial.printf("UI FILTER ROW %u %s\n",(unsigned)i,item->id);
    }
    Serial.println("UI FILTER END");
  }
  if(screen==Screen::Diagnostics){
    static uint32_t lastHealth=0;
    if(force||now-lastHealth>=1000u){
      lastHealth=now;
      Serial.printf("UI HEALTH 1 %lu %lu %u %lu %u\n",
        (unsigned long)ESP.getFreeHeap(),(unsigned long)ESP.getMaxAllocHeap(),
        (unsigned)Serial.availableForWrite(),(unsigned long)(now/1000UL),
        previousActive[0]?1u:0u);
    }
  }
}
size_t cyd32UiFavoriteCount(){return favoriteCount();}
const Scenario* cyd32UiFavoriteAt(size_t rank){return favoriteAt(rank);}
void cyd32UiSetPlaylistState(bool running,bool paused,unsigned index,unsigned total){
  if(playlistRunning!=running||playlistPaused!=paused||playlistIndex!=index||playlistTotal!=total){
    playlistRunning=running;playlistPaused=paused;playlistIndex=index;playlistTotal=total;
    if(screen==Screen::Home)redraw=true;
  }
}
void cyd32UiRecordTxEvent(const char* event,const char* status,uint32_t duration){
  if(!event)return;
  if(strcmp(event,"BEGIN")==0){
    ++txBegins;txDriverActive=true;actualTxDuration=duration;
    rxId[0]=rxLabel[0]=rxQuality[0]=0;rxRssi=0;
  }else if(strcmp(event,"END")==0){
    ++txEnds;txDriverActive=false;
    if(status&&strcmp(status,"ok")&&strcmp(status,"stopped"))++txErrors;
  }else if(strcmp(event,"ERROR")==0){++txErrors;txDriverActive=false;}
  if(screen==Screen::Activity||screen==Screen::Evidence)redraw=true;
}
void cyd32UiRecordReceiverEvidence(const char* id,const char* classification,int rssi,const char* quality){
  if(!id||!findScenario(id)||!classification||!quality)return;
  snprintf(rxId,sizeof(rxId),"%s",id);
  snprintf(rxLabel,sizeof(rxLabel),"%s",classification);
  snprintf(rxQuality,sizeof(rxQuality),"%s",quality);
  rxRssi=rssi;
  if(screen==Screen::Evidence)redraw=true;
}
void cyd32UiSetPcControl(bool enabled){if(pcControl!=enabled){pcControl=enabled;redraw=true;lastTouchActivity=millis();}}
void cyd32UiSetResetReason(unsigned reason){resetReasonCode=reason;}
void cyd32UiRecordIdentity(const char* id,const char* mac){
  snprintf(identityCase,sizeof(identityCase),"%s",id?id:"");
  snprintf(identityMac,sizeof(identityMac),"%s",mac?mac:"");
}
void cyd32UiRecordResult(const char* id,const char* status,uint32_t atMs){
  if(!id||!*id||!status)return;
  HistoryEntry& h=history[historyHead];
  snprintf(h.id,sizeof(h.id),"%s",id);
  snprintf(h.status,sizeof(h.status),"%s",status);
  h.uptime=atMs/1000UL;
  historyHead=(uint8_t)((historyHead+1)%8);
  if(historyCount<8)++historyCount;
  if(screen==Screen::History)redraw=true;
}
bool cyd32UiRemoteTouch(uint16_t x,uint16_t y,const char* phase){
  if(!ready||!calibrated||!phase)return false;
  if(strcmp(phase,"UP")==0){remoteDown=false;return true;}
  if(x>=lcd.width()||y>=lcd.height())return false;
  if(strcmp(phase,"DOWN")!=0&&strcmp(phase,"MOVE")!=0)return false;
  remoteX=x;remoteY=y;remoteAt=millis();remoteDown=true;
  return true;
}
#endif
