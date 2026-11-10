#if defined(SQE_CYD32)
#include "cyd32_ui.h"
#include "scenarios.h"
#include <Arduino.h>
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <string.h>

// This screen structure follows the owner's SquachWatch-CYD CLEAR and
// SETTINGS screens: centered large Squachy, fine 14px vapor title,
 // square soft buttons and full-width outlined list rows.
 // Original reduced drawing; no SquachWatch dependency or GPL assets.
namespace {
constexpr int kW=320,kH=240;
constexpr int kFooterY=214,kFooterH=23;
constexpr int kMenuTop=43,kMenuStep=39,kCasesTop=51,kCasesStep=36;
constexpr uint16_t kBg=0x0801,kTaskbar=0x0803,kPurple=0xAC1F;
constexpr uint16_t kCyan=0x07FF,kPink=0xF96F,kVaporPink=0xFB99;
constexpr uint16_t kVaporPurple=0xBB5F,kYellow=0xFFD2,kGreen=0x07E0;
constexpr uint16_t kWhite=0xFFFF,kMuted=0x7393,kInk=0x1801;
constexpr uint16_t kFurDark=0x38C0,kFurMain=0x5941,kFurLight=0x9326;
constexpr uint16_t kSkin=0xF60F,kSkinDark=0xCB88;

enum class Screen : uint8_t { Home, Categories, Library };
enum class Filter : uint8_t { All, Drone, Ble, Wifi };
const char* const kCategoryName[] = {"ALL TEST CASES","DRONE / REMOTE ID",
                                      "BLUETOOTH / BLE","WIFI / BEACONS"};
const char* const kCategoryId[] = {"ALL","DRONE","BLE","WIFI"};
constexpr size_t kPageRows=4;

TFT_eSPI lcd;
Preferences prefs;
Screen screen=Screen::Home;
Filter filter=Filter::All;
size_t selected=0,page=0;
bool ready=false,calibrated=false,redraw=true,wasPressed=false,previousSuite=false;
bool panelInverted=false;
uint32_t lastPoll=0,lastPress=0,lastCount=UINT32_MAX,colorBarsUntil=0;
uint8_t talkIndex=0;
char previousActive[56]={0};

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
void titleBar(){
  for(int x=0;x<kW;++x){
    const uint16_t c=x<160?blend(kPurple,kCyan,(uint8_t)(x*255/160)):
                         blend(kCyan,kPink,(uint8_t)((x-160)*255/160));
    lcd.drawFastVLine(x,0,14,c);
  }
  lcd.drawFastHLine(0,14,kW,kPurple);
  lcd.fillRect(94,2,132,10,kTaskbar);
  label(123,3,"SQUACHEMIT",kWhite);
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
  button(112,kFooterY,96,kFooterH,b);
  button(216,kFooterY,96,kFooterH,c);
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
void bigSquachy(bool emitting,bool suite){
  // Proportions follow SquachWatch's central standing character: long arms,
  // squat fur-covered body, wider head, tan muzzle. Distinct original art.
  const int x=159,y=92;
  lcd.drawFastHLine(95,194,130,kPurple);
  lcd.drawFastHLine(112,195,96,kVaporPurple);
  // Legs/feet
  wedge(x-13,y+49,x-19,y+90,12,kInk);
  wedge(x+13,y+49,x+18,y+90,12,kInk);
  wedge(x-13,y+49,x-19,y+89,9,kFurDark);
  wedge(x+13,y+49,x+18,y+89,9,kFurDark);
  lcd.fillRoundRect(x-30,y+86,28,12,5,kFurMain);
  lcd.fillRoundRect(x+3,y+86,28,12,5,kFurMain);
  // Torso + long angled arms, strong keyline.
  lcd.fillRoundRect(x-31,y+20,62,63,20,kInk);
  lcd.fillRoundRect(x-28,y+20,56,60,18,kFurMain);
  lcd.fillRoundRect(x-13,y+38,26,28,10,kFurDark);
  wedge(x-27,y+26,x-43,y+74,12,kInk);
  wedge(x+27,y+26,x+43,y+74,12,kInk);
  wedge(x-27,y+26,x-43,y+74,9,kFurMain);
  wedge(x+27,y+26,x+43,y+74,9,kFurMain);
  // Head dark outline and layered fur (same silhouette geometry family).
  lcd.fillRoundRect(x-38,y-39,76,65,23,kInk);
  lcd.fillRoundRect(x-35,y-37,70,60,22,kFurLight);
  lcd.fillRoundRect(x-32,y-34,64,56,20,kFurMain);
  lcd.fillTriangle(x-20,y-33,x-9,y-52,x+3,y-31,kFurLight);
  lcd.fillTriangle(x-15,y-35,x-8,y-46,x+3,y-32,kFurMain);
  lcd.fillCircle(x-34,y-8,10,kInk);
  lcd.fillCircle(x+34,y-8,10,kInk);
  lcd.fillCircle(x-34,y-8,7,kFurDark);
  lcd.fillCircle(x+34,y-8,7,kFurDark);
  // Muzzle and eyes. No white "bear-face" mask.
  lcd.fillRoundRect(x-23,y-15,46,30,11,kSkin);
  lcd.fillRoundRect(x-20,y-18,40,9,4,kFurLight);
  lcd.drawFastHLine(x-19,y-18,14,kFurDark);
  lcd.drawFastHLine(x+5,y-18,14,kFurDark);
  lcd.fillCircle(x-11,y-10,4,kWhite);
  lcd.fillCircle(x+11,y-10,4,kWhite);
  lcd.fillCircle(x-10,y-9,2,kInk);
  lcd.fillCircle(x+10,y-9,2,kInk);
  lcd.fillRoundRect(x-5,y+2,10,5,2,kFurDark);
  lcd.fillCircle(x-17,y+4,3,kSkinDark);
  lcd.fillCircle(x+17,y+4,3,kSkinDark);
  if(emitting){
    lcd.fillCircle(x,y+12,5,kInk);
    lcd.fillCircle(x,y+12,2,kPink);
  }else if(suite)lcd.drawFastHLine(x-10,y+12,20,kInk);
  else{
    lcd.drawLine(x-10,y+10,x,y+15,kInk);
    lcd.drawLine(x,y+15,x+10,y+10,kInk);
  }
}
void countdownOnly(uint32_t count){
  // Entire home stays untouched; repaint timer digits only.
  lcd.fillRect(238,62,65,17,kBg);
  char buf[20];
  snprintf(buf,sizeof(buf),"%lus LEFT",(unsigned long)count);
  label(238,65,buf,kPink);
}
void homeScreen(const char* active,uint32_t remaining,bool suite){
  const bool emitting=active&&*active;
  lcd.fillScreen(kBg);
  titleBar();
  fit(8,22,emitting?"!! EMITTING !!":suite?">> LAB SUITE <<":">> ALL CLEAR <<",
      303,emitting?kPink:kGreen,2);
  bigSquachy(emitting,suite);
  const char* lines[]={"READY TO TEST","CHOOSE AN RF PROFILE","RECEIVER ON A 2ND CYD"};
  fit(203,40,emitting?active:suite?"RUNNING SUITE":lines[talkIndex%3],
      109,emitting?kPink:kCyan,1);
  lcd.drawFastHLine(202,57,108,kPurple);
  if(emitting)countdownOnly((remaining+999)/1000);
  else label(218,65,suite?"SUITE ON":"RF: OFF",suite?kYellow:kGreen);
  const Scenario* chosen=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  label(6,176,"SELECTED:",kVaporPink);
  fit(6,189,chosen?chosen->id:"NONE",99,kWhite,1);
  label(211,176,"CASES:",kVaporPink);
  label(256,176,"49",kWhite,2);
  label(211,192,"2.4 GHZ LAB",kMuted);
  footer("TESTS",emitting?"STOP":"RUN","SUITE");
}
bool matches(size_t index){
  const Scenario& s=kScenarios[index];
  const bool wifi=s.kind==Kind::WifiAp||s.kind==Kind::WifiRemoteId;
  switch(filter){
    case Filter::All:return true;
    case Filter::Wifi:return wifi;
    case Filter::Ble:return !wifi;
    case Filter::Drone:return strcmp(s.expected,"DRONE")==0 ||
          strstr(s.id,"DRONE")!=nullptr||strncmp(s.id,"RID-",4)==0;
  }
  return true;
}
size_t totalMatches(){
  size_t count=0;
  for(size_t i=0;i<kScenarioCount;++i)if(matches(i))++count;
  return count;
}
const Scenario* atRank(size_t rank,size_t* raw=nullptr){
  for(size_t i=0;i<kScenarioCount;++i){
    if(!matches(i))continue;
    if(!rank){if(raw)*raw=i;return &kScenarios[i];}
    --rank;
  }
  return nullptr;
}
void firstSelected(){
  size_t raw=0;
  if(atRank(page*4,&raw))selected=raw;
}
void menuRow(int y,const char* title,const char* sub,bool chosen=false){
  // Native SquachWatch settings row: squared panel, thin purple outline,
  // no rounded glossy cards and no crowded filter pills.
  lcd.fillRect(4,y+1,kW-10,34,chosen?0x2825:kBg);
  lcd.drawRect(4,y+1,kW-10,34,chosen?kPink:kPurple);
  fit(15,y+4,title,280,chosen?kWhite:kCyan,2);
  if(sub)fit(15,y+23,sub,282,kMuted,1);
}
void categoriesScreen(){
  lcd.fillScreen(kBg);
  titleBar();
  label(8,21,"TEST LIBRARY",kVaporPink,2);
  for(size_t i=0;i<4;++i){
    char sub[50];
    if(i==0)snprintf(sub,sizeof(sub),"ALL %u TEST CASES",(unsigned)kScenarioCount);
    else if(i==1)snprintf(sub,sizeof(sub),"SYNTHETIC REMOTE ID / DRONE");
    else if(i==2)snprintf(sub,sizeof(sub),"BLUETOOTH ADVERTISING");
    else snprintf(sub,sizeof(sub),"2.4 GHZ WIFI BEACONS");
    const char* titles[]={"ALL TEST CASES","DRONE / REMOTE ID",
                          "BLUETOOTH / BLE","WIFI / BEACONS"};
    menuRow(44+(int)i*39,titles[i],sub);
  }
  pinnedBack();
}
void libraryScreen(const char* active,bool suite){
  lcd.fillScreen(kBg);
  titleBar();
  const char* names[]={"ALL","DRONE","BLE","WIFI"};
  char header[44];
  snprintf(header,sizeof(header),"%s  %u CASES",names[(int)filter],
           (unsigned)totalMatches());
  fit(8,23,header,301,kVaporPink,2);
  for(size_t row=0;row<4;++row){
    size_t raw=0;
    const Scenario* s=atRank(page*4+row,&raw);
    char title[55]={0},hint[45]={0};
    if(s){
      snprintf(title,sizeof(title),"%02u  %.42s",(unsigned)(page*4+row+1),s->id);
      snprintf(hint,sizeof(hint),"EXPECTED: %.24s",s->expected);
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
Cyd32UiAction touchHome(int x,int y,bool active){
  if(y>=kFooterY){
    if(x<108){screen=Screen::Categories;return Cyd32UiAction::None;}
    if(x<213)return active?Cyd32UiAction::Stop:Cyd32UiAction::Run;
    return Cyd32UiAction::Suite;
  }
  if(x<212&&y>=40&&y<195){
    talkIndex=(talkIndex+1)%3;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchCategories(int x,int y){
  if(y>=211){screen=Screen::Home;return Cyd32UiAction::None;}
  if(y<44||y>=44+4*39)return Cyd32UiAction::None;
  const size_t row=(size_t)((y-44)/39);
  if(row<4){
    filter=(Filter)row;
    page=0;
    firstSelected();
    screen=Screen::Library;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchLibrary(int x,int y,bool active){
  if(y>=kFooterY){
    if(x<108){screen=Screen::Categories;return Cyd32UiAction::None;}
    if(x<213){if(page>0){--page;firstSelected();}return Cyd32UiAction::None;}
    if(active)return Cyd32UiAction::Stop;
    if((page+1)*4<totalMatches()){++page;firstSelected();}
    return Cyd32UiAction::None;
  }
  if(y>=48&&y<48+4*36){
    const size_t row=(size_t)((y-48)/36);
    size_t idx=0;
    if(row<4&&atRank(page*4+row,&idx)){
      selected=idx;
      screen=Screen::Home;
    }
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

void cyd32UiBegin(){
  pinMode(TFT_BL,OUTPUT);
  digitalWrite(TFT_BL,LOW);
  lcd.init();
  lcd.setRotation(1); // 320x240 landscape
  // From SquachWatch's real CYD32: ST7798-compatible, BGR, inversion OFF.
  // TFT inversion needs an absolute setting after setRotation().
  panelInverted=false;
  if(prefs.begin("sqemit32",true)){
    panelInverted=prefs.getBool("invert",false);
    prefs.end();
  }
  lcd.invertDisplay(panelInverted);
  lcd.fillScreen(kBg);
  digitalWrite(TFT_BL,HIGH);
  calibrated=loadCal();
  ready=true;
  redraw=true;
  Serial.printf("CYD32,PANEL,BGR,INVERT_%s\n",panelInverted?"ON":"OFF");
  Serial.println(calibrated?"CYD32,UI,CLEAR_TOUCH_SAVED":"CYD32,UI,CLEAR_TOUCH_SETUP");
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
void cyd32UiColorBars(){if(ready)rgbBars();}
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
  if(strcmp(previousActive,name)!=0||previousSuite!=suite){
    strncpy(previousActive,name,sizeof(previousActive)-1);
    previousActive[sizeof(previousActive)-1]='\0';
    previousSuite=suite;
    redraw=true;
  }
  if(redraw){
    if(!calibrated)calibrationScreen();
    else if(screen==Screen::Home)homeScreen(active,remaining,suite);
    else if(screen==Screen::Categories)categoriesScreen();
    else libraryScreen(active,suite);
    lastCount=(active&&*active)?((remaining+999)/1000):UINT32_MAX;
    redraw=false;
  }else if(calibrated&&screen==Screen::Home&&active&&*active){
    const uint32_t count=(remaining+999)/1000;
    if(count!=lastCount){countdownOnly(count);lastCount=count;}
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
  const bool pressed=lcd.getTouch(&x,&y,450);
  Cyd32UiAction action=Cyd32UiAction::None;
  if(pressed&&!wasPressed&&now-lastPress>=240&&x<kW&&y<kH){
    if(screen==Screen::Home)action=touchHome(x,y,active&&*active);
    else if(screen==Screen::Categories)action=touchCategories(x,y);
    else action=touchLibrary(x,y,active&&*active);
    lastPress=now;
    redraw=true;
  }
  wasPressed=pressed;
  return action;
}
#endif
