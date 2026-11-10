#if defined(SQE_CYD32)
#include "cyd32_ui.h"
#include "scenarios.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <string.h>

// SquachEmit CYD 3.2: a two-screen original UI based on the visual language
// of SquachWatch / SquachWare. No SquachWatch GPL drawing implementation
// or Pwnagotchi GPL artwork/source is included. All pixels drawn directly
// to the ST7798-compatible 320x240 TFT (no PSRAM/full-frame buffer).
namespace {
constexpr int16_t kWidth=320, kHeight=240;
constexpr int16_t kTitleHeight=17, kButtonsY=211, kButtonH=28;
constexpr int16_t kTabsY=48, kTabsH=26;
constexpr int16_t kRowsY=82, kRowH=26, kRowGap=3, kRowsPerPage=4;
constexpr uint16_t kBg=0x0801, kChrome=0x0803, kPurple=0xAC1F;
constexpr uint16_t kCyan=0x07FF, kPink=0xF96F, kVaporPink=0xFB99;
constexpr uint16_t kVaporPurple=0xBB5F, kGreen=0x07E0, kGold=0xFFD2;
constexpr uint16_t kWhite=0xFFFF, kMuted=0x73CC, kPanel=0x1003;
constexpr uint16_t kInk=0x1801, kFurDark=0x38C0, kFur=0x5941;
constexpr uint16_t kFurLight=0x9326, kSkin=0xF60F;
constexpr uint8_t kTabCount=4;
constexpr const char* kTabNames[kTabCount]={"ALL","DRONE","BLE","WIFI"};

enum class Screen : uint8_t { Home, Library };
enum class Filter : uint8_t { All, Drone, Ble, Wifi };
TFT_eSPI lcd;
Preferences prefs;
Screen screen=Screen::Home;
Filter filter=Filter::All;
size_t selected=0, page=0;
bool ready=false, calibrated=false, redraw=true, wasPressed=false, previousSuite=false;
bool panelInverted=false;
uint32_t colorBarsUntil=0;
uint32_t lastPoll=0, lastPress=0, lastCount=UINT32_MAX;
uint8_t idleLine=0;
char previousActive[56]={0};

// Color blending is only used for the 2-pixel title strip on page changes.
// Radio-driven countdown updates never recompute a screen gradient.
uint16_t blend(uint16_t a,uint16_t b,uint8_t fraction){
  const int ar=(a>>11)&31,ag=(a>>5)&63,ab=a&31;
  const int br=(b>>11)&31,bg=(b>>5)&63,bb=b&31;
  const int r=ar+(br-ar)*fraction/255;
  const int g=ag+(bg-ag)*fraction/255;
  const int bl=ab+(bb-ab)*fraction/255;
  return (uint16_t)((r<<11)|(g<<5)|bl);
}
void label(int x,int y,const char* s,uint16_t color,uint8_t font=1){
  lcd.setTextFont(font);
  lcd.setTextColor(color);
  lcd.setTextSize(1);
  lcd.setCursor(x,y);
  if(s)lcd.print(s);
}
void clipped(int x,int y,const char* s,int maxW,uint16_t color,uint8_t font=2){
  if(!s)return;
  lcd.setTextFont(font);
  char cut[58]={0};
  strncpy(cut,s,sizeof(cut)-1);
  while(cut[0] && lcd.textWidth(cut)>maxW)
    cut[strlen(cut)-1]='\0';
  label(x,y,cut,color,font);
}
void titleBar(){
  for(int x=0;x<kWidth;++x){
    const uint16_t c=(x<kWidth/2)
      ?blend(kPurple,kCyan,(uint8_t)(x*255/(kWidth/2)))
      :blend(kCyan,kPink,(uint8_t)((x-kWidth/2)*255/(kWidth/2)));
    lcd.drawFastVLine(x,0,3,c);
  }
  lcd.fillRect(0,3,kWidth,kTitleHeight-3,kChrome);
  lcd.drawFastHLine(0,kTitleHeight-1,kWidth,kPurple);
  label(9,5,"SQUACHEMIT",kWhite,2);
  label(226,6,"SQUACHWARE R3",kCyan);
}
void smallPill(int x,int y,int w,const char* s,uint16_t edge,bool filled=false){
  lcd.fillRoundRect(x,y,w,19,4,filled?0x2825:kBg);
  lcd.drawRoundRect(x,y,w,19,4,edge);
  const int tw=(int)strlen(s)*6;
  label(x+(w-tw)/2,y+6,s,filled?kWhite:edge);
}
void actionButton(int x,int y,int w,const char* s,uint16_t edge,bool highlighted=false){
  lcd.fillRoundRect(x,y,w,kButtonH,5,highlighted?0x3027:kBg);
  lcd.drawRoundRect(x,y,w,kButtonH,5,edge);
  const int length=(int)strlen(s)*6;
  label(x+(w-length)/2,y+10,s,highlighted?kWhite:edge);
}
void bigSquachy(bool emitting,bool suite){
  // Original character drawing deliberately scales up the face and body,
  // rather than placing a tiny generic bear icon in a cluttered toolbar.
  const int x=62, y=83;
  lcd.fillEllipse(x,y+72,58,8,0x2004); // ground shadow
  // Boots and body. Shaggy outline makes the silhouette visibly Sasquatch.
  lcd.fillRoundRect(x-25,y+12,50,69,18,kInk);
  lcd.fillRoundRect(x-22,y+11,44,66,16,kFur);
  lcd.fillRoundRect(x-35,y+28,18,49,9,kInk);
  lcd.fillRoundRect(x+17,y+28,18,49,9,kInk);
  lcd.fillRoundRect(x-33,y+27,14,46,7,kFurDark);
  lcd.fillRoundRect(x+19,y+27,14,46,7,kFurDark);
  lcd.fillRoundRect(x-20,y+62,18,31,8,kInk);
  lcd.fillRoundRect(x+2,y+62,18,31,8,kInk);
  lcd.fillRoundRect(x-18,y+63,14,27,6,kFurDark);
  lcd.fillRoundRect(x+4,y+63,14,27,6,kFurDark);
  lcd.fillRoundRect(x-24,y+85,22,10,5,kFur);
  lcd.fillRoundRect(x+2,y+85,22,10,5,kFur);
  // Raised hair tuft and outer head.
  lcd.fillTriangle(x-16,y-34,x-8,y-51,x+1,y-35,kInk);
  lcd.fillTriangle(x-12,y-31,x-7,y-45,x+1,y-29,kFurLight);
  lcd.fillRoundRect(x-29,y-37,58,61,20,kInk);
  lcd.fillRoundRect(x-26,y-35,52,55,18,kFur);
  lcd.fillCircle(x-26,y-9,9,kInk);
  lcd.fillCircle(x+26,y-9,9,kInk);
  lcd.fillCircle(x-26,y-9,6,kFurDark);
  lcd.fillCircle(x+26,y-9,6,kFurDark);
  lcd.fillRoundRect(x-20,y-18,40,31,12,kSkin);
  lcd.fillRoundRect(x-16,y-19,32,11,5,kFurLight);
  // Eyes and brows.
  lcd.drawFastHLine(x-17,y-15,13,kInk);
  lcd.drawFastHLine(x+4,y-15,13,kInk);
  lcd.fillCircle(x-10,y-10,4,kWhite);
  lcd.fillCircle(x+10,y-10,4,kWhite);
  lcd.fillCircle(x-10,y-9,2,kInk);
  lcd.fillCircle(x+10,y-9,2,kInk);
  lcd.fillRoundRect(x-5,y+1,10,4,2,kFurDark);
  const uint16_t mood=emitting?kPink:(suite?kGold:kCyan);
  if(emitting){
    lcd.drawCircle(x,y+8,5,kInk);
    lcd.fillCircle(x,y+8,2,mood);
    lcd.drawLine(x-18,y-20,x-6,y-18,mood);
    lcd.drawLine(x+6,y-18,x+18,y-20,mood);
  } else if(suite){
    lcd.drawFastHLine(x-10,y+9,20,kInk);
    lcd.drawPixel(x-10,y+8,mood);
    lcd.drawPixel(x+10,y+8,mood);
  } else {
    lcd.drawLine(x-10,y+7,x,y+11,kInk);
    lcd.drawLine(x,y+11,x+10,y+7,kInk);
  }
  // Purple ground with contrasting highlights.
  lcd.drawFastHLine(6,191,113,kPurple);
  lcd.drawFastHLine(20,192,84,kVaporPurple);
  label(21,196,emitting?"SQUACHY : ON AIR":suite?"SQUACHY : SUITE":"SQUACHY : READY",
        mood);
}
void bubble(int x,int y,int w,int h,uint16_t border){
  lcd.fillRoundRect(x,y,w,h,8,kPanel);
  lcd.drawRoundRect(x,y,w,h,8,border);
  lcd.fillTriangle(x,y+24,x-11,y+34,x,y+43,kPanel);
  lcd.drawLine(x,y+24,x-11,y+34,border);
  lcd.drawLine(x-11,y+34,x,y+43,border);
}
void countdownOnly(uint32_t count){
  // Small background patch only; no full-screen flicker every second.
  lcd.fillRect(262,111,42,21,kPanel);
  char buf[16];
  snprintf(buf,sizeof(buf),"%lus",(unsigned long)count);
  label(265,111,buf,kPink,2);
}
void homeScreen(const char* active,uint32_t remaining,bool suite){
  const bool emitting=active&&*active;
  lcd.fillScreen(kBg);
  titleBar();
  smallPill(9,23,89,emitting?"RF ON / TEST":suite?"SUITE ACTIVE":"RF OFF / IDLE",
            emitting?kPink: (suite?kGold:kGreen));
  smallPill(109,23,92,"49 LAB CASES",kVaporPurple);
  smallPill(214,23,97,"ESP32 / 2.4G",kCyan);
  bigSquachy(emitting,suite);
  bubble(132,47,179,102,emitting?kPink:kPurple);
  label(144,56,emitting?"TRANSMITTING":suite?"TEST SEQUENCE":"READY FOR A TEST",
        emitting?kPink: (suite?kGold:kCyan),2);
  lcd.drawFastHLine(144,77,157,kVaporPurple);
  if(emitting) {
    clipped(144,85,active,157,kWhite,2);
    label(144,115,"TIME LEFT",kMuted);
    countdownOnly((remaining+999)/1000);
  }else if(suite){
    label(144,85,"AUTOMATED SUITE",kWhite,2);
    label(144,115,"NEXT TEST LOADING",kMuted);
  }else{
    const Scenario* s=selected<kScenarioCount?&kScenarios[selected]:nullptr;
    clipped(144,85,s?s->id:"CHOOSE A CASE",157,kWhite,2);
    label(144,114,"TAP TO CHANGE TEST",kMuted);
  }
  lcd.fillRoundRect(132,156,179,43,5,kPanel);
  lcd.drawRoundRect(132,156,179,43,5,kPurple);
  label(143,164,"SELECTED LAB PROFILE",kVaporPink);
  const Scenario* chosen=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  clipped(142,177,chosen?chosen->description:"NO TEST",160,kWhite,1);
  actionButton(5,kButtonsY,74,"TESTS",kCyan,true);
  actionButton(84,kButtonsY,73,"RUN",kGreen);
  actionButton(162,kButtonsY,73,"STOP",kPink);
  actionButton(240,kButtonsY,75,"SUITE",kVaporPurple);
}
bool matches(size_t index){
  const Scenario& s=kScenarios[index];
  const bool wifi=s.kind==Kind::WifiAp || s.kind==Kind::WifiRemoteId;
  switch(filter){
    case Filter::All:return true;
    case Filter::Wifi:return wifi;
    case Filter::Ble:return !wifi;
    case Filter::Drone:return strcmp(s.expected,"DRONE")==0 ||
                             strstr(s.id,"DRONE")!=nullptr ||
                             strncmp(s.id,"RID-",4)==0;
  }
  return true;
}
size_t totalMatches(){
  size_t count=0;
  for(size_t i=0;i<kScenarioCount;++i)if(matches(i))++count;
  return count;
}
const Scenario* atRank(size_t rank,size_t* sourceIndex=nullptr){
  for(size_t i=0;i<kScenarioCount;++i){
    if(!matches(i))continue;
    if(!rank){
      if(sourceIndex)*sourceIndex=i;
      return &kScenarios[i];
    }
    --rank;
  }
  return nullptr;
}
void firstSelected(){
  size_t idx=0;
  if(atRank(page*kRowsPerPage,&idx))selected=idx;
}
void libraryScreen(const char* active,bool suite){
  lcd.fillScreen(kBg);
  titleBar();
  label(9,23,"TEST LIBRARY",kCyan,2);
  char count[28];
  snprintf(count,sizeof(count),"%u / 49", (unsigned)totalMatches());
  label(253,29,count,kVaporPink);
  for(uint8_t i=0;i<kTabCount;++i){
    const int x=5+79*i;
    const bool chosen=(uint8_t)filter==i;
    smallPill(x,kTabsY,74,kTabNames[i],chosen?kCyan:kPurple,chosen);
  }
  for(size_t row=0;row<kRowsPerPage;++row){
    const int y=kRowsY+(int)row*(kRowH+kRowGap);
    const size_t rank=page*kRowsPerPage+row;
    size_t rawIndex=0;
    const Scenario* s=atRank(rank,&rawIndex);
    const bool chosen=s&&rawIndex==selected;
    lcd.fillRoundRect(6,y,308,kRowH,4,chosen?0x2825:kPanel);
    lcd.drawRoundRect(6,y,308,kRowH,4,chosen?kPink:kPurple);
    if(!s)continue;
    if(chosen)lcd.fillRect(8,y+4,3,kRowH-8,kCyan);
    char number[10];
    snprintf(number,sizeof(number),"%02u",(unsigned)(rank+1));
    label(17,y+8,number,kMuted);
    clipped(39,y+4,s->id,208,chosen?kWhite:kCyan,2);
    const uint16_t kindColor=!strcmp(s->expected,"NONE")?kGreen:
                            !strcmp(s->expected,"DRONE")?kVaporPink:kGold;
    clipped(254,y+8,s->expected,53,kindColor,1);
  }
  const size_t countAll=totalMatches();
  const size_t pages=(countAll+kRowsPerPage-1)/kRowsPerPage;
  char pagesText[46];
  snprintf(pagesText,sizeof(pagesText),"PAGE %u/%u   %s",
           (unsigned)(page+1),(unsigned)(pages?pages:1),
           active?"ON AIR":suite?"SUITE":"RF OFF");
  label(13,200,pagesText,active?kPink:kMuted);
  actionButton(5,kButtonsY,74,"HOME",kVaporPurple);
  actionButton(84,kButtonsY,73,"< PREV",kCyan);
  actionButton(162,kButtonsY,73,"NEXT >",kCyan);
  actionButton(240,kButtonsY,75,(active&&*active)?"STOP":"RUN",
               (active&&*active)?kPink:kGreen,true);
}
void calibrationScreen(){
  lcd.fillScreen(kBg);
  titleBar();
  label(25,34,"WELCOME TO SQUACHEMIT",kCyan,2);
  lcd.drawRoundRect(14,70,292,113,9,kPurple);
  label(39,88,"FIRST BOOT / TOUCH SETUP",kVaporPink,2);
  label(40,123,"TAP TO CALIBRATE",kWhite,2);
  label(40,152,"FOLLOW THE FIVE TARGETS",kCyan);
  label(27,201,"115200 BAUD  /  RF OFF ON BOOT",kGreen);
}
bool storeCalibration(uint16_t data[5]){
  if(!prefs.begin("sqemit32",false)){
    Serial.println("WARN,CYD32_TOUCH_NVS_WRITE_FAILED");
    return false;
  }
  const bool saved=prefs.putBytes("touchland",data,10)==10;
  prefs.end();
  if(!saved)Serial.println("WARN,CYD32_TOUCH_NVS_WRITE_FAILED");
  return saved;
}
bool loadCalibration(){
  if(!prefs.begin("sqemit32",true))return false;
  uint16_t data[5]={0};
  const bool present=prefs.getBytesLength("touchland")==10 &&
                     prefs.getBytes("touchland",data,10)==10;
  prefs.end();
  if(present)lcd.setTouch(data);
  return present;
}
void calibrationWizard(){
  // TFT_eSPI wizard is blocking and NEVER entered during RF or suite.
  uint16_t data[5]={0};
  lcd.fillScreen(kBg);
  label(18,20,"SQUACHWARE / TOUCH CALIBRATION",kCyan,2);
  lcd.calibrateTouch(data,kWhite,kBg,15);
  lcd.setTouch(data);
  storeCalibration(data);
  calibrated=true;
  wasPressed=true;
  redraw=true;
  Serial.println("CYD32,TOUCH,CALIBRATED_LANDSCAPE");
}
Cyd32UiAction touchHome(int x,int y,bool active){
  if(y>=kButtonsY){
    if(x<82){screen=Screen::Library;page=0;firstSelected();return Cyd32UiAction::None;}
    if(x<160)return Cyd32UiAction::Run;
    if(x<239)return Cyd32UiAction::Stop;
    return Cyd32UiAction::Suite;
  }
  if((x>=125&&y>=47&&y<=200) || (x<125&&y>=48&&y<198)){
    screen=Screen::Library;
    page=0;firstSelected();
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchLibrary(int x,int y,bool active){
  if(y>=kTabsY&&y<kTabsY+kTabsH){
    uint8_t tab=(uint8_t)(x/79);
    if(tab<kTabCount){
      filter=(Filter)tab;page=0;firstSelected();
    }
    return Cyd32UiAction::None;
  }
  if(y>=kRowsY&&y<kRowsY+kRowsPerPage*(kRowH+kRowGap)){
    const size_t row=(size_t)((y-kRowsY)/(kRowH+kRowGap));
    size_t rawIndex=0;
    if(row<kRowsPerPage && atRank(page*kRowsPerPage+row,&rawIndex))
      selected=rawIndex;
    return Cyd32UiAction::None;
  }
  if(y>=kButtonsY){
    if(x<82){screen=Screen::Home;return Cyd32UiAction::None;}
    if(x<160){if(page>0){--page;firstSelected();}return Cyd32UiAction::None;}
    if(x<239){
      if((page+1)*kRowsPerPage<totalMatches()){++page;firstSelected();}
      return Cyd32UiAction::None;
    }
    return active?Cyd32UiAction::Stop:Cyd32UiAction::Run;
  }
  return Cyd32UiAction::None;
}
} // namespace

void cyd32UiBegin(){
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,LOW);
  lcd.init();
  lcd.setRotation(1); // 320x240 ST7798-compatible landscape
  // Match SquachWatch ST7798: explicit panel state AFTER setRotation.
  panelInverted=false;
  if(prefs.begin("sqemit32",true)){
    panelInverted=prefs.getBool("invert",false);
    prefs.end();
  }
  lcd.invertDisplay(panelInverted);
  Serial.printf("CYD32,PANEL,BGR,INVERT_%s\n",panelInverted?"ON":"OFF");
  lcd.fillScreen(kBg);
  digitalWrite(TFT_BL,HIGH);
  calibrated=loadCalibration();
  ready=true;redraw=true;
  Serial.println(calibrated?"CYD32,SQUACHWARE_HOME,LANDSCAPE_TOUCH_SAVED":
                             "CYD32,SQUACHWARE_HOME,LANDSCAPE_TOUCH_SETUP");
}
const Scenario* cyd32UiSelection(){
  return selected<kScenarioCount?&kScenarios[selected]:nullptr;
}
void cyd32UiSetInversion(bool enabled){
  panelInverted=enabled;
  lcd.invertDisplay(enabled);
  if(prefs.begin("sqemit32",false)){
    prefs.putBool("invert",enabled);
    prefs.end();
  }
  redraw=true;
  Serial.printf("CYD32,PANEL,BGR,INVERT_%s\n",enabled?"ON":"OFF");
}
bool cyd32UiInversion(){return panelInverted;}

void cyd32UiColorBars(){
  colorBarsUntil=millis()+7000;
  lcd.fillScreen(kBg);
  titleBar();
  label(16,26,"ST7798 RGB565 TEST",kCyan,2);
  const uint16_t cols[]={TFT_RED,TFT_GREEN,TFT_BLUE,TFT_WHITE,TFT_BLACK};
  const char* names[]={"RED","GREEN","BLUE","WHITE","BLACK"};
  for(uint8_t i=0;i<5;++i){
    const int x=7+i*62;
    lcd.fillRect(x,62,56,85,cols[i]);
    lcd.drawRect(x,62,56,85,kPurple);
    label(x,157,names[i],kWhite);
  }
  label(13,196,"TAP TO EXIT / 7S AUTO",kVaporPink);
}
void cyd32UiResetCalibration(){
  if(prefs.begin("sqemit32",false)){
    prefs.remove("touchland");
    prefs.end();
  }
  calibrated=false;wasPressed=false;redraw=true;
  Serial.println("CYD32,TOUCH,RESET_LANDSCAPE;TAP_TO_CALIBRATE");
}
Cyd32UiAction cyd32UiTick(const char* active,uint32_t remaining,bool suite){
  if(!ready)return Cyd32UiAction::None;
  const uint32_t now=millis();
  if(colorBarsUntil){
    if((int32_t)(now-colorBarsUntil)>=0){colorBarsUntil=0;redraw=true;}
    else {
      uint16_t tx=0,ty=0;
      if(lcd.getTouch(&tx,&ty,450)){colorBarsUntil=0;redraw=true;}
      else return Cyd32UiAction::None;
    }
  }
  const char* name=active?active:"";
  if(strcmp(name,previousActive)!=0 || suite!=previousSuite){
    strncpy(previousActive,name,sizeof(previousActive)-1);
    previousActive[sizeof(previousActive)-1]='\0';
    previousSuite=suite;
    redraw=true;
  }
  if(redraw){
    if(!calibrated)calibrationScreen();
    else if(screen==Screen::Home)homeScreen(active,remaining,suite);
    else libraryScreen(active,suite);
    lastCount=(active&&*active)?((remaining+999)/1000):UINT32_MAX;
    redraw=false;
  }else if(calibrated&&screen==Screen::Home&&active&&*active){
    const uint32_t count=(remaining+999)/1000;
    if(count!=lastCount){countdownOnly(count);lastCount=count;}
  }
  if(now-lastPoll<50)return Cyd32UiAction::None;
  lastPoll=now;
  if(!calibrated){
    if((active&&*active)||suite)return Cyd32UiAction::None;
    uint16_t rawX=0,rawY=0;
    const bool pressed=lcd.getTouchRaw(&rawX,&rawY);
    if(pressed&&!wasPressed&&now-lastPress>350){
      lastPress=now;calibrationWizard();
    }
    wasPressed=pressed;
    return Cyd32UiAction::None;
  }
  uint16_t x=0,y=0;
  const bool pressed=lcd.getTouch(&x,&y,450);
  Cyd32UiAction action=Cyd32UiAction::None;
  if(pressed&&!wasPressed&&now-lastPress>=220&&x<kWidth&&y<kHeight){
    action=screen==Screen::Home ? touchHome(x,y,active&&*active)
                                : touchLibrary(x,y,active&&*active);
    lastPress=now;redraw=true;
  }
  wasPressed=pressed;
  return action;
}
#endif
