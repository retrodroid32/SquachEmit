#if defined(SQE_CYD32)
#include "cyd32_ui.h"
#include "scenarios.h"
#include "scenario_library.h"
#include "cyd32_squachy_port.h"
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
constexpr size_t kPageRows=4;

TFT_eSPI lcd;
TFT_eSprite squachSprite(&lcd);
bool squachSpriteReady=false;
uint32_t lastSquachFrame=0, petUntil=0;
Preferences prefs;
Screen screen=Screen::Home;
DeviceCategory category=DeviceCategory::Axon;
size_t selected=0,page=0,categoryPage=0;
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
void retroBackdrop(){
  // Synthwave night-scape: matches SquachWatch's actual dark, detailed
  // backgrounds rather than leaving a flat near-black void.
  // Designed independently in TFT primitives, no bitmap/PSRAM required.
  lcd.fillRect(0,15,320,164,lcd.color565(4,4,18));
  for (int y=16; y<155; y+=8){
    const uint8_t shift=(uint8_t)((y-16)/8);
    const uint16_t dusk=lcd.color565((uint8_t)(4+shift/3),
                                     (uint8_t)(5+shift/4),
                                     (uint8_t)(25+shift));
    lcd.drawFastHLine(0,y,320,dusk);
  }
  // Stars use deterministic fixed coordinates: no animated background
  // repaints during countdown, unlike the old flickering 3.2 render.
  static const uint16_t stars[][2]={
    {13,27},{48,42},{81,23},{114,37},{154,25},{203,39},{243,26},
    {290,33},{306,47},{27,66},{65,79},{94,55},{137,86},{181,63},
    {236,81},{277,68},{311,94},{17,116},{73,124},{222,116},{296,130}};
  for(const auto& st:stars){
    lcd.drawPixel(st[0],st[1],kCyan);
    if((st[0]%3)==0)lcd.drawPixel(st[0]+1,st[1],kVaporPurple);
  }
  // Distant neon mountains and city buildings form a readable silhouette.
  lcd.fillTriangle(0,146,53,87,115,146,lcd.color565(17,3,44));
  lcd.fillTriangle(62,146,138,93,201,146,lcd.color565(15,2,36));
  lcd.fillTriangle(166,146,252,83,320,146,lcd.color565(20,3,48));
  const struct {uint16_t x,y,w;} buildings[]={
    {0,132,19},{22,116,15},{42,128,14},{61,100,19},
    {86,123,18},{111,116,12},{215,116,20},{240,99,16},
    {263,128,17},{284,107,20},{308,123,12}};
  for(const auto& b:buildings){
    lcd.fillRect(b.x,b.y,b.w,168-b.y,lcd.color565(5,2,24));
    lcd.drawFastVLine(b.x,b.y,168-b.y,kVaporPurple);
    for(int y=b.y+5;y<163;y+=10){
      for(int x=b.x+4;x<(int)b.x+b.w-2;x+=7)
        lcd.fillRect(x,y,2,3,(x+y)%3==0?kPink:kCyan);
    }
  }
  // Retro ground grid, a signature SquachWatch scene motif.
  lcd.fillRect(0,168,320,13,lcd.color565(3,1,13));
  lcd.drawFastHLine(0,167,320,kVaporPink);
  for(int x=0;x<320;x+=28)
    lcd.drawLine(160,167,x,181,kPurple);
  lcd.drawFastHLine(0,174,320,kPurple);
  lcd.drawFastHLine(0,180,320,kCyan);
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
  if(!squachSpriteReady)return;
  SquachyPort::render(squachSprite,now,emitting,suite,(int32_t)(petUntil-now)>0);
  squachSprite.pushSprite(104,70);
  lastSquachFrame=now;
}
void bigSquachy(bool emitting,bool suite){
  if(squachSpriteReady)drawSquachyFrame(emitting,suite,millis());
  else oldSquachy(emitting,suite);
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

void speechBubble(const char* active,bool suite){
  lcd.fillRoundRect(7,20,306,44,4,kBg);
  lcd.drawRoundRect(7,20,306,44,4,kVaporPink);
  lcd.drawRoundRect(9,22,302,40,3,kPurple);
  const Scenario* chosen=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  const char* phrase=(active&&*active)?"RF LAB TRANSMITTING":suite?"TEST SUITE RUNNING":
      "READY TO TEST. PICK A PROFILE.";
  fit(16,25,phrase,285,kWhite,2);
  fit(16,47,active&&*active?active:chosen?chosen->id:"NO PROFILE",
      288,active&&*active?kPink:kCyan,1);
}

void statusStrip(const char* active,bool suite){
  lcd.fillRect(0,183,320,29,kBg);
  lcd.drawFastHLine(6,182,308,kPurple);
  label(12,186,active&&*active?"ON AIR":suite?"SUITE":"LAB READY",
        active&&*active?kPink:kGreen);
  label(97,186,"BLE / WIFI / RID",kCyan);
  if(!(active&&*active))rightStatus(false,0);
  lcd.drawFastHLine(6,198,308,kPurple);
  label(12,201,"PROFILE",kVaporPink);
  const Scenario* chosen=selected<kScenarioCount?&kScenarios[selected]:nullptr;
  fit(66,200,chosen?chosen->id:"NONE",164,kWhite,1);
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
  if(emitting)countdownOnly((remaining+999)/1000);
  footer("TESTS",emitting?"STOP":"RUN","SUITE");
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
  label(8,21,"TEST LIBRARY / DEVICE TYPE",kVaporPink,2);
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
    char title[55]={0},hint[45]={0};
    if(s){
      const bool djiBle = startsWith(s->id,"RID-DJI-");
      const bool djiWifi = startsWith(s->id,"RID-WIFI-DJI-");
      if(djiBle || djiWifi) {
        // Clearly identify the model name and transport. These are lab labels,
        // not a signature proven to identify an actual DJI aircraft.
        const char* end = strstr(s->description," | ");
        if(end) snprintf(title,sizeof(title),"%02u  %.*s",
                         (unsigned)(page*4+row+1),
                         (int)(end-s->description),s->description);
        else snprintf(title,sizeof(title),"%02u  %.42s",
                      (unsigned)(page*4+row+1),s->id);
        snprintf(hint,sizeof(hint),"%s",
                 djiWifi?"SYNTHETIC WIFI BEACON / BASIC ID":
                          "SYNTHETIC BLE4 / EXPECT DRONE");
      } else {
        snprintf(title,sizeof(title),"%02u  %.42s",
                 (unsigned)(page*4+row+1),s->id);
        snprintf(hint,sizeof(hint),"EXPECTED: %.24s",s->expected);
      }
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
  if(x>=116&&x<=204&&y>=70&&y<183){
    petUntil=millis()+1500;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchCategories(int x,int y){
  if(y>=kFooterY){
    if(x<108){screen=Screen::Home;return Cyd32UiAction::None;}
    if(x<213){if(categoryPage>0)--categoryPage;return Cyd32UiAction::None;}
    if((categoryPage+1)*kPageRows<kDeviceCategoryCount)++categoryPage;
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
  panelInverted=true; // physically observed E32R32P board requires inversion ON
  if(prefs.begin("sqemit32",true)){
    // New key avoids inheriting R3's incorrect "invert=false" default.
    panelInverted=prefs.getBool("invert2",true);
    prefs.end();
  }
  lcd.invertDisplay(panelInverted);
  lcd.fillScreen(kBg);
  squachSprite.setColorDepth(16);
  squachSpriteReady=(squachSprite.createSprite(112,111)!=nullptr);
  if(!squachSpriteReady)Serial.println("WARN,CYD32_SPRITE_ALLOC;STATIC_FALLBACK");
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
    prefs.putBool("invert2",enabled);
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
  if(calibrated&&screen==Screen::Home&&squachSpriteReady&&
     now-lastSquachFrame>=120u && colorBarsUntil==0)
    drawSquachyFrame(active&&*active,suite,now);
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
