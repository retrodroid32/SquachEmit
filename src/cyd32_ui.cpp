#if defined(SQE_CYD32)
#include "cyd32_ui.h"
#include "scenarios.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <string.h>

// SquachEmit's original LCD renderer, independently inspired by the
// SquachWatch-CYD SQUACHWARE aesthetic and Pwnagotchi's state-driven faces.
// No GPLv3 Pwnagotchi implementation or asset is copied.
//
// The ESP32-2432S032R has a 320x240 landscape TFT and classic ESP32 RAM.
// Avoid full-frame buffers: repaint the chrome only for real UI changes,
// and patch only the seconds field while emitting.
namespace {
constexpr int16_t W=320, H=240;
constexpr int16_t TAB_Y=82, TAB_H=21, LIST_Y=108, LIST_H=25;
constexpr int16_t ROW_GAP=2, ROWS=3, NAV_Y=190, NAV_H=20;
constexpr int16_t BUTTON_Y=214, BUTTON_H=25;
constexpr uint16_t BG=0x0801, TASKBAR=0x0803, PURPLE=0xAC1F;
constexpr uint16_t CYAN=0x07FF, PINK=0xF96F, VAPOR_PINK=0xFB99;
constexpr uint16_t VAPOR_PURPLE=0xBB5F, GREEN=0x07E0, AMBER=0xFD20;
constexpr uint16_t WHITE=0xFFFF, MUTED=0x73CC, FACE=0x1003;
constexpr uint16_t FUR_DARK=0x38C0, FUR_MAIN=0x5941, FUR_LIGHT=0x9326;
constexpr uint16_t SKIN=0xF60F, RED=0xF800;
constexpr uint8_t kFilterTabCount=4;

enum class Filter : uint8_t { All, Drone, Ble, Wifi };
constexpr const char* TAB_NAMES[kFilterTabCount] = {"ALL","DRONE","BLE","WIFI"};
constexpr const char* MASCOT_LINES[] = {
  "LAB MODE. NO SECRETS.",
  "49 CASES. PICK ONE.",
  "LET'S TEST THAT.",
  "VERIFY THE RECEIVER."
};

TFT_eSPI tft;
Preferences prefs;
bool ready=false,calibrated=false,dirty=true,wasPressed=false,oldSuite=false;
size_t selected=0,page=0;
Filter filter=Filter::All;
uint32_t lastPoll=0,lastPress=0;
uint32_t lastTimerSeconds=UINT32_MAX;
uint8_t mascotLine=0;
char lastActiveId[54]={0};

uint16_t mix565(uint16_t a,uint16_t b,uint16_t t){
  const uint16_t ar=(a>>11)&31,ag=(a>>5)&63,ab=a&31;
  const uint16_t br=(b>>11)&31,bg=(b>>5)&63,bb=b&31;
  return ((ar+((int)br-(int)ar)*t/255)<<11) |
         ((ag+((int)bg-(int)ag)*t/255)<<5) |
          (ab+((int)bb-(int)ab)*t/255);
}

void text(int16_t x,int16_t y,const char* s,uint16_t fg,uint8_t font=1){
  tft.setTextFont(font);
  tft.setTextSize(1);
  tft.setTextColor(fg);
  tft.setCursor(x,y);
  if(s)tft.print(s);
}

void fitText(int16_t x,int16_t y,const char* value,int16_t width,
             uint16_t fg,uint8_t font=1){
  if(!value)return;
  tft.setTextFont(font);
  char buf[70]={0};
  strncpy(buf,value,sizeof(buf)-1);
  while(buf[0] && tft.textWidth(buf)>width){
    const size_t len=strlen(buf);
    buf[len-1]='\0';
  }
  if(buf[0])text(x,y,buf,fg,font);
}

void gradientTitle(){
  // SquachWatch's vaporwave purple -> cyan -> pink accent ribbon.
  for(int16_t x=0;x<W;x++){
    const uint16_t color=x<W/2
      ?mix565(PURPLE,CYAN,(uint16_t)(x*255/(W/2)))
      :mix565(CYAN,PINK,(uint16_t)((x-W/2)*255/(W/2)));
    tft.drawFastVLine(x,0,3,color);
  }
  tft.fillRect(0,3,W,18,TASKBAR);
  tft.drawFastHLine(0,20,W,PURPLE);
  text(9,4,"SQUACH",VAPOR_PINK,2);
  text(79,4,"EMIT",CYAN,2);
  text(220,7,"RF / LAB  v0.4",MUTED);
}

void mascot(int16_t x,int16_t y,bool onAir,bool suite){
  // Original tiny vector Squachy; no sprites/full-frame allocations.
  const uint16_t mood=onAir?PINK:suite?AMBER:CYAN;
  tft.fillRoundRect(x,y+17,48,36,12,FUR_DARK);
  tft.fillRoundRect(x+5,y+17,38,37,12,FUR_MAIN);
  tft.drawRoundRect(x+4,y+16,40,38,12,FUR_LIGHT);
  tft.fillCircle(x+4,y+17,8,FUR_DARK);
  tft.fillCircle(x+44,y+17,8,FUR_DARK);
  tft.fillRoundRect(x+8,y,33,36,14,FUR_LIGHT);
  tft.fillRoundRect(x+11,y+3,27,30,12,FUR_MAIN);
  tft.fillRoundRect(x+13,y+13,23,18,8,SKIN);
  tft.fillCircle(x+18,y+15,3,WHITE);
  tft.fillCircle(x+31,y+15,3,WHITE);
  tft.fillCircle(x+18,y+15,1,FUR_DARK);
  tft.fillCircle(x+31,y+15,1,FUR_DARK);
  tft.drawFastHLine(x+21,y+21,7,FUR_DARK);
  if(onAir){
    tft.drawLine(x+21,y+26,x+28,y+26,mood);
    tft.drawLine(x+21,y+26,x+24,y+29,mood);
    tft.drawLine(x+24,y+29,x+28,y+26,mood);
  }else if(suite){
    tft.drawFastHLine(x+20,y+26,9,mood);
    tft.drawPixel(x+20,y+27,mood);
    tft.drawPixel(x+28,y+27,mood);
  }else{
    tft.drawLine(x+20,y+25,x+25,y+28,FUR_DARK);
    tft.drawLine(x+25,y+28,x+30,y+25,FUR_DARK);
  }
  tft.drawCircle(x+24,y+24,21,mood);
}

const Scenario* activeCase(const char* id){
  return (id && *id) ? findScenario(id) : nullptr;
}

bool matches(size_t index){
  const Scenario& s=kScenarios[index];
  const bool wifi=s.kind==Kind::WifiAp || s.kind==Kind::WifiRemoteId;
  switch(filter){
    case Filter::All:return true;
    case Filter::Wifi:return wifi;
    case Filter::Ble:return !wifi;
    case Filter::Drone:
      return strcmp(s.expected,"DRONE")==0 ||
             strncmp(s.id,"RID-",4)==0 ||
             strstr(s.id,"DRONE")!=nullptr;
  }
  return true;
}

size_t matchCount(){
  size_t count=0;
  for(size_t i=0;i<kScenarioCount;++i)if(matches(i))++count;
  return count;
}

const Scenario* filteredAt(size_t rank,size_t* rawIndex=nullptr){
  for(size_t i=0;i<kScenarioCount;++i){
    if(!matches(i))continue;
    if(rank==0){
      if(rawIndex)*rawIndex=i;
      return &kScenarios[i];
    }
    --rank;
  }
  return nullptr;
}

void selectFirstPageRow(){
  size_t idx=0;
  if(filteredAt(page*ROWS,&idx))selected=idx;
}

void statusBubble(const char* active,uint32_t remaining,bool suite){
  const bool onAir=active && active[0];
  tft.fillRoundRect(77,25,237,53,8,FACE);
  tft.drawRoundRect(77,25,237,53,8,onAir?PINK:VAPOR_PURPLE);
  tft.fillTriangle(77,53,67,61,77,65,FACE);
  tft.drawLine(77,53,67,61,onAir?PINK:VAPOR_PURPLE);
  tft.drawLine(67,61,77,65,onAir?PINK:VAPOR_PURPLE);
  mascot(11,26,onAir,suite);
  text(87,30,onAir?"SQUACHY / ON AIR":suite?"SQUACHY / SUITE":"SQUACHY / IDLE",
       onAir?PINK:suite?AMBER:CYAN);
  tft.drawFastHLine(87,44,216,PURPLE);
  if(onAir){
    fitText(87,49,active,172,WHITE,2);
    text(87,67,"KEEP YOUR RECEIVER READY",MUTED);
  }else if(suite){
    text(87,50,"RUNNING THE LAB SUITE",WHITE,2);
    text(87,67,"WAIT FOR NEXT CASE...",MUTED);
  }else{
    text(87,49,MASCOT_LINES[mascotLine%4],WHITE,2);
    const Scenario* s=&kScenarios[selected];
    fitText(87,67,s->description,210,MUTED);
  }
  if(onAir){
    tft.fillRect(262,48,46,18,FACE);
    char timer[16];
    snprintf(timer,sizeof(timer),"%lus",(unsigned long)((remaining+999)/1000));
    text(270,51,timer,PINK,2);
  }
}

void timerPatch(uint32_t seconds){
  // Patch just the countdown text so the LCD does not blink every second.
  tft.fillRect(262,48,46,18,FACE);
  char timer[16];
  snprintf(timer,sizeof(timer),"%lus",(unsigned long)seconds);
  text(270,51,timer,PINK,2);
}

void drawTabs(){
  for(uint8_t i=0;i<kFilterTabCount;i++){
    const int x=5+i*79, width=74;
    const bool chosen=(uint8_t)filter==i;
    tft.fillRoundRect(x,TAB_Y,width,TAB_H,4,chosen?PURPLE:TASKBAR);
    tft.drawRoundRect(x,TAB_Y,width,TAB_H,4,chosen?CYAN:VAPOR_PURPLE);
    const int len=strlen(TAB_NAMES[i]);
    text(x+(width-len*6)/2,TAB_Y+7,TAB_NAMES[i],chosen?WHITE:CYAN);
  }
}

void drawList(){
  for(int row=0;row<ROWS;row++){
    const int y=LIST_Y+row*(LIST_H+ROW_GAP);
    const size_t rank=page*ROWS+(size_t)row;
    size_t raw=0;
    const Scenario* s=filteredAt(rank,&raw);
    const bool chosen=s && raw==selected;
    tft.fillRoundRect(6,y,W-12,LIST_H,4,chosen?0x2806:FACE);
    tft.drawRoundRect(6,y,W-12,LIST_H,4,chosen?PINK:0x5A8C);
    if(!s)continue;
    if(chosen)tft.fillRect(8,y+5,3,LIST_H-10,CYAN);
    char no[9];
    snprintf(no,sizeof(no),"%02u",(unsigned)(rank+1));
    text(15,y+8,no,chosen?VAPOR_PINK:MUTED);
    fitText(36,y+4,s->id,213,chosen?WHITE:CYAN,2);
    const uint16_t accent=!strcmp(s->expected,"NONE")?GREEN:
                          !strcmp(s->expected,"DRONE")?VAPOR_PURPLE:AMBER;
    text(256,y+8,s->expected,accent);
  }
}

void button(int x,int y,int width,int height,const char* label,
            uint16_t edge,bool emphasized=false){
  const uint16_t fill=emphasized?0x2827:TASKBAR;
  tft.fillRoundRect(x,y,width,height,5,fill);
  tft.drawRoundRect(x,y,width,height,5,edge);
  const int16_t tw=(int16_t)strlen(label)*6;
  text(x+(width-tw)/2,y+(height-8)/2,label,emphasized?WHITE:edge);
}

void drawNav(){
  const size_t total=matchCount();
  const size_t pages=(total+ROWS-1)/ROWS;
  button(6,NAV_Y,85,NAV_H,"< PREV",VAPOR_PURPLE);
  char middle[35];
  snprintf(middle,sizeof(middle),"%u / %u  (%u CASES)",
           (unsigned)(page+1),(unsigned)(pages?pages:1),(unsigned)total);
  text(104,NAV_Y+7,middle,MUTED);
  button(230,NAV_Y,84,NAV_H,"NEXT >",VAPOR_PURPLE);
}

void drawActions(){
  button(6,BUTTON_Y,100,BUTTON_H,"RUN",CYAN,true);
  button(110,BUTTON_Y,100,BUTTON_H,"STOP",PINK);
  button(214,BUTTON_Y,100,BUTTON_H,"SUITE",VAPOR_PURPLE);
}

void render(const char* active,uint32_t remaining,bool suite){
  tft.fillScreen(BG);
  gradientTitle();
  statusBubble(active,remaining,suite);
  drawTabs();
  drawList();
  drawNav();
  drawActions();
}

void calibrationScreen(){
  tft.fillScreen(BG);
  gradientTitle();
  mascot(18,59,false,false);
  text(91,38,"WELCOME TO THE LAB",CYAN,2);
  text(91,60,"TOUCH SETUP REQUIRED",PINK);
  tft.drawRoundRect(12,110,296,85,6,PURPLE);
  text(28,123,"TAP TO START CALIBRATION",WHITE,2);
  text(28,149,"FOLLOW THE 5 TOUCH TARGETS",CYAN);
  text(28,169,"SERIAL: 115200 BAUD",MUTED);
  text(28,208,"NO RF ON BOOT",GREEN);
}

Cyd32UiAction tap(int x,int y){
  if(y>=TAB_Y && y<TAB_Y+TAB_H){
    uint8_t tab=(uint8_t)(x/79);
    if(tab<kFilterTabCount){
      filter=(Filter)tab;
      page=0;
      selectFirstPageRow();
    }
    return Cyd32UiAction::None;
  }
  if(y>=LIST_Y && y<LIST_Y+ROWS*(LIST_H+ROW_GAP)){
    const size_t row=(size_t)((y-LIST_Y)/(LIST_H+ROW_GAP));
    if(row<ROWS){
      size_t raw=0;
      if(filteredAt(page*ROWS+row,&raw))selected=raw;
    }
    return Cyd32UiAction::None;
  }
  if(y>=NAV_Y && y<NAV_Y+NAV_H){
    const size_t pages=(matchCount()+ROWS-1)/ROWS;
    if(x<94 && page>0)--page;
    else if(x>=228 && page+1<pages)++page;
    selectFirstPageRow();
    return Cyd32UiAction::None;
  }
  if(y>=BUTTON_Y){
    if(x<108)return Cyd32UiAction::Run;
    if(x<212)return Cyd32UiAction::Stop;
    return Cyd32UiAction::Suite;
  }
  if(y>=24 && y<=80 && x<76){
    mascotLine=(mascotLine+1)%4;
    return Cyd32UiAction::None;
  }
  return Cyd32UiAction::None;
}

bool saveCal(const uint16_t data[5]){
  if(!prefs.begin("sqemit32",false)){
    Serial.println("WARN,CYD32_TOUCH_NVS_WRITE_FAILED");
    return false;
  }
  const bool ok=prefs.putBytes("touchland",data,10)==10;
  prefs.end();
  if(!ok)Serial.println("WARN,CYD32_TOUCH_NVS_WRITE_FAILED");
  return ok;
}

bool loadCal(){
  if(!prefs.begin("sqemit32",true))return false;
  uint16_t data[5]={0};
  const bool ok=prefs.getBytesLength("touchland")==10 &&
                prefs.getBytes("touchland",data,10)==10;
  prefs.end();
  if(ok)tft.setTouch(data);
  return ok;
}

void startCal(){
  // Blocking TFT_eSPI wizard is entered only from idle, never during an
  // active transmission or a running suite.
  uint16_t data[5]={0};
  tft.fillScreen(BG);
  text(22,16,"SQUACHWARE / TOUCH SETUP",CYAN,2);
  tft.calibrateTouch(data,WHITE,BG,15);
  tft.setTouch(data);
  saveCal(data);
  calibrated=true;
  wasPressed=true;
  dirty=true;
  Serial.println("CYD32,TOUCH,CALIBRATED_LANDSCAPE");
}

} // namespace

void cyd32UiBegin(){
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,LOW);
  tft.init();
  tft.setRotation(1); // 320x240 landscape, separate touchland calibration
  tft.fillScreen(BG);
  digitalWrite(TFT_BL,HIGH);
  calibrated=loadCal();
  ready=true;
  dirty=true;
  Serial.println(calibrated?"CYD32,SQUACHWARE_UI,LANDSCAPE_TOUCH_SAVED":
                             "CYD32,SQUACHWARE_UI,LANDSCAPE_TOUCH_SETUP");
}

const Scenario* cyd32UiSelection(){
  return selected<kScenarioCount?&kScenarios[selected]:nullptr;
}

void cyd32UiResetCalibration(){
  if(prefs.begin("sqemit32",false)){
    prefs.remove("touchland");
    prefs.end();
  }
  calibrated=false;
  wasPressed=false;
  dirty=true;
  Serial.println("CYD32,TOUCH,RESET_LANDSCAPE;TAP_TO_CALIBRATE");
}

Cyd32UiAction cyd32UiTick(const char* active,uint32_t remaining,bool suite){
  if(!ready)return Cyd32UiAction::None;
  const uint32_t now=millis();
  const char* name=active?active:"";
  if(strcmp(name,lastActiveId) || suite!=oldSuite){
    strncpy(lastActiveId,name,sizeof(lastActiveId)-1);
    lastActiveId[sizeof(lastActiveId)-1]='\0';
    oldSuite=suite;
    dirty=true;
  }
  if(dirty){
    if(calibrated)render(active,remaining,suite);
    else calibrationScreen();
    lastTimerSeconds=(active&&*active)?((remaining+999)/1000):UINT32_MAX;
    dirty=false;
  }else if(calibrated&&active&&*active){
    const uint32_t seconds=(remaining+999)/1000;
    if(seconds!=lastTimerSeconds){
      timerPatch(seconds);
      lastTimerSeconds=seconds;
    }
  }

  if(now-lastPoll<55)return Cyd32UiAction::None;
  lastPoll=now;
  if(!calibrated){
    // Don't start a blocking touch calibration that would extend RF airtime.
    if((active&&*active)||suite)return Cyd32UiAction::None;
    uint16_t rx=0,ry=0;
    const bool pressed=tft.getTouchRaw(&rx,&ry);
    if(pressed && !wasPressed && now-lastPress>400){
      lastPress=now;
      startCal();
    }
    wasPressed=pressed;
    return Cyd32UiAction::None;
  }
  uint16_t x=0,y=0;
  const bool pressed=tft.getTouch(&x,&y,450);
  Cyd32UiAction action=Cyd32UiAction::None;
  if(pressed && !wasPressed && now-lastPress>=240 && x<W && y<H){
    action=tap(x,y);
    lastPress=now;
    dirty=true;
  }
  wasPressed=pressed;
  return action;
}
#endif
