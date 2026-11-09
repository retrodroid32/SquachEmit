#if defined(SQE_CYD32)
#include "cyd32_ui.h"
#include "scenarios.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <string.h>

// For the owner's ESP32-2432S032R 3.2" / ST7798-compatible panel,
// all output uses the shared HSPI interface from SquachWatch's verified
// GPIO pin map. No 150KB framebuffer on the WROOM-32 (usually no PSRAM).
namespace {
constexpr int16_t W=240, H=320, FIRST=88, ROW=30;
constexpr size_t PAGE=5;
constexpr uint16_t BG=0x0842, PANEL=0x10A3, SELECT=0x21CC;
constexpr uint16_t WHITE=0xFFFF, CYAN=0x07FF, DIM=0xA514;
constexpr uint16_t RED=0xA800, GREEN=0x07E0;
TFT_eSPI tft;
Preferences prefs;
bool ready=false, calibrated=false, dirty=true, wasPressed=false;
size_t selected=0,page=0;
uint32_t lastDraw=0,lastPoll=0,lastPress=0;
uint32_t lastTimerSeconds=UINT32_MAX;
char lastActiveId[54]={0};
bool lastSuite=false;

void textAt(int x, int y, const char* str, uint16_t color, int fontSize=1) {
  tft.setTextFont(1);
  tft.setTextSize(fontSize);
  tft.setTextColor(color);
  tft.setCursor(x,y);
  if(str) tft.print(str);
  tft.setTextSize(1);
}

void button(int x, int y, int w, int h, const char* str, uint16_t bg) {
  tft.fillRect(x,y,w,h,bg);
  tft.drawRect(x,y,w,h,WHITE);
  const int len=(int)strlen(str);
  textAt(x+(w-len*6)/2,y+(h-8)/2,str,WHITE);
}

void renderCalibration() {
  tft.fillScreen(BG);
  tft.fillRect(0,0,W,39,SELECT);
  textAt(10,9,"SQUACHEMIT",CYAN,2);
  textAt(9,49,"CYD 3.2 / ST7798 LAB",WHITE);
  textAt(9,83,"TOUCH NOT CALIBRATED",RED);
  textAt(9,116,"TAP SCREEN TO START",WHITE);
  textAt(9,136,"5-POINT CALIBRATION",WHITE);
  textAt(9,180,"FOLLOW EACH TARGET",DIM);
  textAt(9,197,"THEN USE RUN / STOP",DIM);
  textAt(9,240,"SERIAL STILL WORKS",GREEN);
  textAt(9,258,"115200 BAUD",DIM);
}

void render(const char* active,uint32_t remaining,bool suite) {
  tft.fillScreen(BG);
  tft.fillRect(0,0,W,37,SELECT);
  textAt(8,7,"SQUACHEMIT",CYAN,2);
  textAt(7,30,"CYD 3.2 / RF LAB",WHITE);
  textAt(8,44,active?"ON AIR":suite?"SUITE":"READY - NO RF",
         active?RED:GREEN);
  if(active) {
    char label[34]; snprintf(label,sizeof(label),"%.29s",active);
    textAt(7,59,label,WHITE);
    char timer[24];
    snprintf(timer,sizeof(timer),"%lu SEC LEFT",
             (unsigned long)((remaining+999)/1000));
    textAt(7,71,timer,DIM);
  } else textAt(7,62,"SELECT TEST THEN RUN",DIM);

  for(size_t i=0;i<PAGE;++i) {
    const size_t index=page*PAGE+i;
    if(index>=kScenarioCount)break;
    const int y=FIRST+(int)i*ROW;
    tft.fillRect(5,y,230,27,index==selected?SELECT:PANEL);
    tft.drawRect(5,y,230,27,index==selected?CYAN:DIM);
    char id[36];
    snprintf(id,sizeof(id),"%.35s",kScenarios[index].id);
    textAt(10,y+4,id,WHITE);
    textAt(10,y+16,kScenarios[index].expected,index==selected?CYAN:DIM);
  }
  char pages[22];
  snprintf(pages,sizeof(pages),"%u / %u",(unsigned)(page+1),
           (unsigned)((kScenarioCount+PAGE-1)/PAGE));
  textAt(106,243,pages,DIM);
  button(5,257,112,27,"PREV",PANEL);
  button(123,257,112,27,"NEXT",PANEL);
  button(5,289,73,28,"RUN",SELECT);
  button(83,289,73,28,"STOP",RED);
  button(161,289,74,28,"SUITE",0x29C8);
}

Cyd32UiAction handleTap(int x,int y) {
  if (y>=FIRST && y<FIRST+(int)PAGE*ROW) {
    const size_t idx=page*PAGE+(size_t)((y-FIRST)/ROW);
    if(idx<kScenarioCount)selected=idx;
    return Cyd32UiAction::None;
  }
  if(y>=257 && y<285){
    if(x<120){if(page>0)--page;}
    else if((page+1)*PAGE<kScenarioCount)++page;
    selected=page*PAGE;
    return Cyd32UiAction::None;
  }
  if(y>=289){
    if(x<80)return Cyd32UiAction::Run;
    if(x<159)return Cyd32UiAction::Stop;
    return Cyd32UiAction::Suite;
  }
  return Cyd32UiAction::None;
}

void saveCal(const uint16_t data[5]){
  if(!prefs.begin("sqemit32",false)){
    Serial.println("WARN,CYD32_TOUCH_NVS_WRITE_FAILED");
    return;
  }
  if(prefs.putBytes("touch",data,10)!=10)
    Serial.println("WARN,CYD32_TOUCH_NVS_WRITE_FAILED");
  prefs.end();
}

bool loadCal() {
  if(!prefs.begin("sqemit32",true))return false;
  uint16_t data[5]={0};
  const bool found=prefs.getBytesLength("touch")==10 &&
                   prefs.getBytes("touch",data,10)==10;
  prefs.end();
  if(!found)return false;
  tft.setTouch(data);
  return true;
}

void startCalibration() {
  // TFT_eSPI calibrateTouch drives the resistive XPT2046 through the
  // shared HSPI bus. It is intentionally user-triggered, never on boot.
  uint16_t data[5]={0};
  tft.fillScreen(BG);
  textAt(8,5,"TOUCH CALIBRATION",CYAN);
  tft.calibrateTouch(data,WHITE,BG,15);
  tft.setTouch(data);
  saveCal(data);
  calibrated=true;
  wasPressed=true; // wait for finger release to prevent phantom RUN
  dirty=true;
  Serial.println("CYD32,TOUCH,CALIBRATED");
}
}  // namespace

void cyd32UiBegin(){
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,LOW);
  tft.init();
  tft.setRotation(0); // 240x320 native portrait; calibration stored in this frame
  tft.fillScreen(BG);
  digitalWrite(TFT_BL,HIGH);
  calibrated=loadCal();
  ready=true;
  dirty=true;
  Serial.println(calibrated?"CYD32,DISPLAY_READY,TOUCH_SAVED":
                             "CYD32,DISPLAY_READY,TOUCH_NEEDS_CAL");
}

const Scenario* cyd32UiSelection(){
  return selected<kScenarioCount?&kScenarios[selected]:nullptr;
}

void cyd32UiResetCalibration(){
  // Never erase unrelated SquachWatch NVS keys.
  if(prefs.begin("sqemit32",false)){prefs.remove("touch");prefs.end();}
  calibrated=false;
  wasPressed=false;
  dirty=true;
  Serial.println("CYD32,TOUCH,RESET;TAP_TO_CALIBRATE");
}

Cyd32UiAction cyd32UiTick(const char* active,uint32_t remaining,bool suite){
  if(!ready)return Cyd32UiAction::None;
  const uint32_t now=millis();
  const char* id = active ? active : "";
  if(strcmp(lastActiveId,id)!=0 || lastSuite!=suite){
    strncpy(lastActiveId,id,sizeof(lastActiveId)-1);
    lastActiveId[sizeof(lastActiveId)-1]='\0';
    lastSuite=suite;
    dirty=true;
  }
  if(dirty){
    if(calibrated)render(active,remaining,suite);
    else renderCalibration();
    lastDraw=now;
    lastTimerSeconds=active?(remaining+999)/1000:UINT32_MAX;
    dirty=false;
  } else if(calibrated && active){
    // Only repaint countdown: a full-screen redraw every second caused
    // distracting flashing on the related SquachWatch 3.2 screen.
    const uint32_t seconds=(remaining+999)/1000;
    if(seconds!=lastTimerSeconds){
      tft.fillRect(7,70,200,11,BG);
      char timer[24];
      snprintf(timer,sizeof(timer),"%lu SEC LEFT",(unsigned long)seconds);
      textAt(7,71,timer,DIM);
      lastTimerSeconds=seconds;
    }
  }
  if(now-lastPoll<55)return Cyd32UiAction::None;
  lastPoll=now;
  if(!calibrated){
    // calibrateTouch() is a blocking wizard; never enter it while the
    // radio or the case suite is active, or a timed emission could overrun.
    if(active || suite)return Cyd32UiAction::None;
    uint16_t rx=0,ry=0;
    if(tft.getTouchRaw(&rx,&ry) && !wasPressed &&
       now-lastPress>400){
      lastPress=now;
      startCalibration();
    }
    return Cyd32UiAction::None;
  }
  uint16_t x=0,y=0;
  const bool pressed=tft.getTouch(&x,&y,450);
  Cyd32UiAction action=Cyd32UiAction::None;
  if(pressed && !wasPressed && now-lastPress>=240 && x<W && y<H) {
    action=handleTap((int)x,(int)y);
    lastPress=now;
    dirty=true;
  }
  wasPressed=pressed;
  return action;
}
#endif
