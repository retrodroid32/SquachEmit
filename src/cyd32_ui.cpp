#if defined(SQE_CYD32)
#include "cyd32_ui.h"
#include "scenarios.h"
#include "scenario_library.h"
#include "scenario_display.h"
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
constexpr uint16_t kBg=0x0801,kTaskbar=0x0803;
uint16_t kPurple=0xAC1F;
uint16_t kCyan=0x07FF,kPink=0xF96F;
constexpr uint16_t kVaporPink=0xFB99;
constexpr uint16_t kVaporPurple=0xBB5F,kYellow=0xFFD2,kGreen=0x07E0;
constexpr uint16_t kWhite=0xFFFF,kMuted=0x7393,kInk=0x1801;
constexpr uint16_t kFurDark=0x38C0,kFurMain=0x5941,kFurLight=0x9326;
constexpr uint16_t kSkin=0xF60F,kSkinDark=0xCB88;

enum class Screen : uint8_t { Home, Categories, Library, Duration, Details, Diagnostics, Favorites, History, Settings, Help, Identity, Recovery, Activity, Evidence, QuickFilter, Portrait, MainMenu, Outfits, OtaInfo, Backgrounds };
constexpr size_t kPageRows=4;

TFT_eSPI lcd;
TFT_eSprite squachSprite(&lcd);
bool squachSpriteReady=false;
uint32_t lastSquachFrame=0, petUntil=0;
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
bool portraitCalibrated=false,portraitMode=false;
uint16_t portraitTouchData[5]={0};
uint32_t lastBubbleAt=0;
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
void titleBar(){
  for(int x=0;x<kW;++x){
    const uint16_t c=x<160?blend(kPurple,kCyan,(uint8_t)(x*255/160)):
                         blend(kCyan,kPink,(uint8_t)((x-160)*255/160));
    lcd.drawFastVLine(x,0,14,c);
  }
  lcd.drawFastHLine(0,14,kW,kPurple);
  // Black hamburger tile; the full 48x23 top-left area is tappable.
  lcd.fillRoundRect(5,1,27,13,3,kTaskbar);
  for(int y=4;y<=10;y+=3)lcd.drawFastHLine(11,y,15,kWhite);
  lcd.setTextFont(1);
  const int nameW=lcd.textWidth("SQUACHEMIT");
  const int nameX=(kW-nameW)/2;
  lcd.fillRect(nameX-8,1,nameW+16,12,kTaskbar);
  label(nameX,3,"SQUACHEMIT",kWhite);
  const uint16_t txColor=onAirIndicator?kGreen:kYellow;
  lcd.drawLine(292,11,298,3,txColor);
  lcd.drawLine(298,3,304,11,txColor);
  lcd.drawFastVLine(298,6,7,txColor);
  lcd.fillCircle(298,3,2,txColor);
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
  if(backgroundLevel!=0){
    lcd.fillRect(0,15,320,166,lcd.color565(3,4,16));
    if(backgroundLevel==1){ // SquachWatch Digital Rain motif; bounded static glyphs
      for(int x=7;x<320;x+=14){
        const int head=22+(x*17+(int)(millis()/220))%145;
        for(int j=0;j<8;++j){
          const int y=15+(head-15-j*10+166)%166;
          label(x,y,(j%3)==0?"1":"0",j==0?kWhite:blend(kGreen,kBg,(uint8_t)(j*25)));
        }
      }
    }else if(backgroundLevel==2){ // SquachWatch Starfield motif
      for(int i=0;i<95;++i){
        const int x=(i*157+29)%320, y=17+(i*83+11)%162;
        lcd.drawPixel(x,y,i%9==0?kWhite:(i%3==0?kCyan:kMuted));
      }
    }else if(backgroundLevel==3){ // Fireflies
      for(int i=0;i<38;++i){
        const int x=(i*79+31)%320,y=19+(i*67+9)%157;
        lcd.fillCircle(x,y,i%5==0?2:1,i%3==0?kYellow:kGreen);
      }
    }else if(backgroundLevel==4){ // FIRE: glowing embers and tongues
      lcd.fillRect(0,130,320,51,lcd.color565(49,6,2));
      for(int i=0;i<72;++i){
        const int x=(i*47+13)%320,y=50+(i*83+29)%120;
        const uint16_t glow=i%3==0?kYellow:i%3==1?kPink:lcd.color565(220,35,0);
        lcd.drawLine(x,180,x-((i%3)-1)*6,y,glow);
      }
    }else if(backgroundLevel==5){ // SNOWFALL: moon and drifting flakes
      lcd.fillCircle(275,43,18,kWhite);
      lcd.fillRect(0,152,320,29,lcd.color565(19,47,81));
      for(int i=0;i<100;++i)lcd.fillCircle((i*73+23)%320,
          18+(i*101+17)%151,i%7==0?2:1,kWhite);
    }else if(backgroundLevel==6){ // AQUARIUM: waves, plants and fish
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
    }else if(backgroundLevel==7){ // TERMINAL LOG: matrix console text
      for(int row=0;row<12;++row){
        char log[42];
        snprintf(log,sizeof(log),"%02d %04X:%04X TX_%s",row,
          (row*173+38)&0xFFFF,(row*119+22)&0xFFFF,
          row%3==0?"IDLE":"READY");
        label(7,20+row*13,log,row%3==0?kCyan:kGreen);
      }
    }else if(backgroundLevel==8){ // FLYING TOASTERS: procedural silhouettes
      for(int i=0;i<10;++i){
        const int x=12+(i*83)%287,y=28+(i*41)%127;
        lcd.fillRoundRect(x,y,21,19,4,kYellow);
        lcd.fillRect(x+4,y+4,13,8,lcd.color565(84,42,3));
        lcd.drawLine(x,y+7,x-9,y-4,kWhite);
        lcd.drawLine(x+21,y+7,x+30,y-4,kWhite);
      }
    }else if(backgroundLevel==9){ // THE GIBSON: circuit grid, dynamic activity
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
  SquachyPort::render(squachSprite,now,emitting,suite,(int32_t)(petUntil-now)>0,outfitLevel);
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
  const bool failed=!(active&&*active) && radioStartError[0];
  const bool starting=!(active&&*active) && !failed && radioStartingProfile[0];
  static const char* idleTalk[]={"READY FOR A SIGNAL CHECK!","PICK A PROFILE. I WILL EMIT!",
    "172 TESTS. ONE LITTLE SQUACH.","NO RF UNTIL YOU PRESS RUN.",
    "LET US TEST THAT DETECTOR!"};
  static const char* activeTalk[]={"SENDING SYNTHETIC SIGNAL...",
    "CHECK YOUR RECEIVER NOW!","TEST SIGNAL IS ON THE AIR!"};
  const char* phrase=failed?"WIFI START FAILED":
      starting?"STARTING WIFI TRANSMISSION":
      (active&&*active)?activeTalk[(millis()/5500u)%3u]:
      playlistRunning?"FAVORITES PLAYLIST":
      suite?"TEST SUITE RUNNING":idleTalk[(millis()/6500u)%5u];
  fit(16,25,phrase,285,failed?kPink:(starting?kYellow:kWhite),2);
  char friendly[58]={0};
  const Scenario* showing=(active&&*active)?findScenario(active):
    starting?findScenario(radioStartingProfile):chosen;
  if(showing)scenarioDeviceName(*showing,friendly,sizeof(friendly));
  fit(16,47,failed?radioStartError:
      showing?friendly:"NO PROFILE",
      288,failed?kYellow:(starting?kYellow:(active&&*active?kPink:kCyan)),1);
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
Cyd32UiAction touchPortrait(int x,int y,bool active,bool suite){
  if(y<278)return Cyd32UiAction::None;
  if(x<80){
    lcd.setRotation(1);
    if(calibrated)loadCal();
    wasPressed=true;screen=Screen::Home;redraw=true;
    return Cyd32UiAction::None;
  }
  if(x<160)
    return (active||suite||playlistRunning)?Cyd32UiAction::Stop:Cyd32UiAction::Run;
  return playlistRunning?(playlistPaused?Cyd32UiAction::PlaylistResume:
         Cyd32UiAction::PlaylistPause):Cyd32UiAction::Suite;
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
    menuRow(163,"PORTRAIT MODE / LAB","EXPERIMENTAL - RETURN TO LANDSCAPE");
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
  footer("BACK","MORE >","HELP");
}

constexpr uint8_t kMenuEntries=12;
const char* const menuTitles[kMenuEntries]={
 "TEST LIBRARY","QUICK FIND","FAVORITES","TX HISTORY",
 "HEALTH / DIAGNOSTICS","SETTINGS","SQUACHY OUTFITS",
 "OTA UPDATES","TX ACTIVITY","HELP","RECOVERY","BACKGROUNDS"};
const char* const menuHints[kMenuEntries]={
 "172 SYNTHETIC CASES","A-Z CASE FILTER","SAVED SIGNALS","LAST TRANSMISSIONS",
 "HEAP, RADIO, UPTIME","DISPLAY, ROTATION, THEME","COSTUMES AND ANIMATION",
 "FIRMWARE UPDATE STATUS","DRIVER EVENTS","USAGE AND SAFETY","BOOT AND TOUCH","FIVE SCENES"};
void mainMenuScreen(){
  lcd.fillScreen(kBg);titleBar();label(13,25,"SQUACHEMIT / MENU",kVaporPink,2);
  for(uint8_t i=0;i<4;++i){
    const uint8_t n=menuPage*4+i;
    if(n<kMenuEntries)menuRow(46+i*39,menuTitles[n],menuHints[n]);
  }
  char count[24];snprintf(count,sizeof(count),"PAGE %u/3",(unsigned)(menuPage+1));
  label(12,201,count,kMuted);footer("HOME","< PREV","NEXT >");
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
static const char* const kBackgroundNames[]={
  "SYNTHWAVE","DIGITAL RAIN","STARFIELD","FIREFLIES",
  "FIRE","SNOWFALL","AQUARIUM","TERMINAL LOG",
  "FLYING TOASTERS","THE GIBSON"};
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
  label(12,136,"IF TOUCH FAILS, USE CALTOUCH",kWhite);
  label(12,165,"VERIFY BOARD BEFORE FLASHING",kYellow);
  label(12,192,"This page does not erase settings",kMuted);
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
Cyd32UiAction touchHome(int x,int y,bool active,bool suite){
  if(y>=kFooterY){
    if(x<108){screen=Screen::Categories;redraw=true;return Cyd32UiAction::None;}
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
  if(x>=233&&y>=15&&y<43){screen=Screen::QuickFilter;filterPage=0;redraw=true;return Cyd32UiAction::None;}
  if(y>=kFooterY){
    if(x<108){screen=Screen::Home;redraw=true;return Cyd32UiAction::None;}
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
Cyd32UiAction touchSettings(int x,int y){
  if(y>=kFooterY){
    if(x<108){screen=Screen::MainMenu;settingsMore=false;}
    else if(x<213){settingsMore=!settingsMore;}
    else {screen=Screen::Help;settingsMore=false;}
    redraw=true;return Cyd32UiAction::None;
  }
  if(y>=46&&y<201){
    const int row=(y-46)/39;
    if(settingsMore){
      if(row==0){themeLevel=(themeLevel+1)%3;applyTheme();persistUiSettings();}
      else if(row==1){fontMode=(fontMode+1)%2;persistUiSettings();}
      else if(row==2)screen=Screen::Identity;
      else if(row==3){
         lcd.setRotation(0);
         if(!portraitCalibrated){
           if(prefs.begin("sqemit32",true)){
             portraitCalibrated=prefs.getBytesLength("touchport")==10 &&
                 prefs.getBytes("touchport",portraitTouchData,10)==10;
             prefs.end();
           }
           if(!portraitCalibrated){
             lcd.fillScreen(kBg);
             label(12,15,"CALIBRATE PORTRAIT TOUCH",kCyan,2);
             lcd.calibrateTouch(portraitTouchData,kWhite,kBg,15);
             portraitCalibrated=true;
             if(prefs.begin("sqemit32",false)){
               prefs.putBytes("touchport",portraitTouchData,10);prefs.end();
             }
           }
         }
         lcd.setTouch(portraitTouchData);
         wasPressed=true;portraitMode=true;portraitLastCount=UINT32_MAX;
       }
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
Cyd32UiAction touchHelp(int x,int y){
  if(y>=kFooterY){
    if(x<108)screen=Screen::MainMenu;
    else if(x<213)helpPage=(helpPage+1)%3;
    else screen=Screen::Recovery;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchIdentity(int x,int y){
  if(y>=kFooterY){
    if(x<108)screen=Screen::Settings;
    else if(x<213)screen=Screen::Help;
    else screen=Screen::Recovery;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchRecovery(int x,int y){
  if(y>=kFooterY){
    if(x<108)screen=Screen::Identity;
    else if(x<213)screen=Screen::Help;
    else screen=Screen::Home;
    redraw=true;
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchFavorites(int x,int y){
  if(x>=220&&y>=15&&y<44&&favoriteCount()){
    screen=Screen::Home;redraw=true;return Cyd32UiAction::PlaylistStart;
  }
  if(y>=kFooterY){
    if(x<108){screen=Screen::Details;redraw=true;}
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
    if(x<108){screen=Screen::Details;redraw=true;}
    else if(x<213){if(historyPage>0){--historyPage;redraw=true;}}
    else if((historyPage+1)*4<historyCount){++historyPage;redraw=true;}
  }
  return Cyd32UiAction::None;
}
Cyd32UiAction touchQuickFilter(int x,int y){
  if(y>=kFooterY){
    if(x<108){screen=Screen::Categories;redraw=true;}
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
  lcd.fillRect(0,0,240,26,kPurple);
  lcd.fillRoundRect(5,3,31,20,3,kTaskbar);
  for(int y=7;y<20;y+=5)lcd.drawFastHLine(12,y,17,kWhite);
  label(81,9,"SQUACHEMIT",kWhite);
  const uint16_t tx=active&&*active?kGreen:kYellow;
  lcd.drawLine(215,14,221,4,tx);lcd.drawLine(221,4,227,14,tx);
  lcd.drawFastVLine(221,9,12,tx);
  const char* name=screen==Screen::Home?"HOME":screen==Screen::MainMenu?"MENU":
    screen==Screen::Categories?"TEST LIBRARY":screen==Screen::Library?"SIGNALS":
    screen==Screen::Settings?"SETTINGS":screen==Screen::Outfits?"OUTFITS":
    screen==Screen::Backgrounds?"BACKGROUNDS":screen==Screen::Favorites?"FAVORITES":
    screen==Screen::QuickFilter?"QUICK FIND":screen==Screen::History?"HISTORY":
    screen==Screen::Duration?"TX TIME":screen==Screen::Details?"DETAILS":
    screen==Screen::Diagnostics?"HEALTH":screen==Screen::OtaInfo?"OTA":
    screen==Screen::Activity?"TX ACTIVITY":screen==Screen::Evidence?"EVIDENCE":
    screen==Screen::Help?"HELP":screen==Screen::Identity?"IDENTITY":"RECOVERY";
  label(12,37,name,kVaporPink,2);
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
      static const char* more[]={"THEME","FONT SIZE","IDENTITY","LANDSCAPE"};
      head=settingsMore?more[i]:basics[i];sub="TAP TO ADJUST";
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
  button(83,287,74,30,screen==Screen::Home?(active||suite?"STOP":"RUN"):"PREV");
  button(163,287,74,30,screen==Screen::Home?"SUITE":"NEXT");
}
Cyd32UiAction portraitTap(int x,int y,bool active,bool suite){
  if(y<27){
    if(x<45){screen=Screen::MainMenu;menuPage=0;redraw=true;}
    else if(x>=195){lcd.setRotation(1);loadCal();portraitMode=false;wasPressed=true;redraw=true;}
    return Cyd32UiAction::None;
  }
  if(screen==Screen::Home){
    if(y>=287){
      if(x<80){screen=Screen::Categories;redraw=true;return Cyd32UiAction::None;}
      if(x<160)return active||suite||playlistRunning?Cyd32UiAction::Stop:Cyd32UiAction::Run;
      return Cyd32UiAction::Suite;
    }
    if(y>=65&&y<261){
      const int n=(y-65)/49;
      if(n==0)screen=Screen::Categories;
      else if(n==1)screen=Screen::Duration;
      else if(n==3)screen=Screen::Details;
      redraw=true;
    }
    return Cyd32UiAction::None;
  }
  if(y>=287){
    if(x<80){screen=Screen::MainMenu;redraw=true;return Cyd32UiAction::None;}
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
      screen=targets[n];redraw=true;
    }
  }else if(screen==Screen::Categories)return touchCategories(150,48+i*39);
  else if(screen==Screen::Library)return touchLibrary(150,53+i*36,active);
  else if(screen==Screen::Favorites)return touchFavorites(150,50+i*37);
  else if(screen==Screen::QuickFilter)return touchQuickFilter(150,68+i*34);
  else if(screen==Screen::Settings){
    if(settingsMore&&i==3){
      lcd.setRotation(1);loadCal();portraitMode=false;wasPressed=true;redraw=true;
    }else return touchSettings(150,50+i*39);
  }else if(screen==Screen::Outfits){
    const int n=outfitPage*4+i;
    if(n<kOutfitCount){outfitLevel=(uint8_t)n;
      if(prefs.begin("sqemit32",false)){prefs.putUChar("outfit",outfitLevel);prefs.end();}
      redraw=true;}
  }else if(screen==Screen::Backgrounds){
    const int n=backgroundPage*4+i;
    if(n<10){backgroundLevel=(uint8_t)n;
      if(prefs.begin("sqemit32",false)){prefs.putUChar("bg",backgroundLevel);prefs.end();}
      redraw=true;}
  }else if(screen==Screen::Duration){
    setDuration(i==0?3:i==1?10:i==2?30:(durationSeconds<30?durationSeconds+1:30));
    redraw=true;
  }else if(screen==Screen::Details&&i==3){
    screen=Screen::Duration;redraw=true;
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
     outfitLevel=prefs.getUChar("outfit",0);
     backgroundLevel=prefs.getUChar("bg",0);
    prefs.end();
  }
  if(durationSeconds<3 || durationSeconds>30)durationSeconds=12;
  if(brightnessLevel>2)brightnessLevel=2;
  if(animationLevel>2)animationLevel=1;
  if(screenTimeout>2)screenTimeout=0;
  if(themeLevel>2)themeLevel=0;
  if(fontMode>1)fontMode=1;
  if(outfitLevel>21)outfitLevel=0;
  if(backgroundLevel>9)backgroundLevel=0;
  applyTheme();
  if(prefs.begin("sqemitfav",true)){
    if(prefs.getBytesLength("fav")==sizeof(favBits))prefs.getBytes("fav",favBits,sizeof(favBits));
    prefs.end();
  }
  lcd.invertDisplay(panelInverted);
  lcd.fillScreen(kBg);
  squachSprite.setColorDepth(16);
  squachSpriteReady=(squachSprite.createSprite(112,111)!=nullptr);
  if(!squachSpriteReady)Serial.println("WARN,CYD32_SPRITE_ALLOC;STATIC_FALLBACK");
  ledcSetup(6,5000,8);
  ledcAttachPin(TFT_BL,6);
  screenDimmed=false;applyBrightness();
  lastTouchActivity=millis();
  calibrated=loadCal();
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
  // A serial-initiated RUN or SUITE must not leave a hidden STOP button
  // behind the duration editor.
  if(((active&&*active)||suite)&&screen!=Screen::Home){
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
  if(redraw){
    if(!calibrated)calibrationScreen();
    else if(portraitMode)portraitPage(active,remaining,suite);
    else if(screen==Screen::Home)homeScreen(active,remaining,suite);
    else if(screen==Screen::Categories)categoriesScreen();
    else if(screen==Screen::Duration)durationScreen();
    else if(screen==Screen::Details)detailScreen();
    else if(screen==Screen::Diagnostics)diagnosticsScreen();
    else if(screen==Screen::Favorites)favoritesScreen();
    else if(screen==Screen::History)historyScreen();
    else if(screen==Screen::Settings)settingsScreen();
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
  }else if(calibrated&&screen==Screen::Home&&active&&*active){
    const uint32_t count=(remaining+999)/1000;
    if(count!=lastCount){countdownOnly(count);lastCount=count;}
  }
  if(calibrated&&screen==Screen::Diagnostics&&now-lastDiagAt>=1000u){diagValues();lastDiagAt=now;}
  if(calibrated&&screen==Screen::Home&&now-lastBubbleAt>6500u){
    speechBubble(active,suite);lastBubbleAt=now;
  }
  if(calibrated&&portraitMode){
    const uint32_t count=(active&&*active)?((remaining+999)/1000):UINT32_MAX;
    if(count!=portraitLastCount){portraitPage(active,remaining,suite);portraitLastCount=count;}
  }
  if(screenTimeout&&!pcControl&&!(active&&*active)&&!suite && !screenDimmed &&
      now-lastTouchActivity>=(screenTimeout==1?30000UL:60000UL)){
    screenDimmed=true;applyBrightness();
  }
  const uint32_t animDelay=animationLevel==0?200u:animationLevel==1?120u:80u;
  if(calibrated&&!portraitMode&&screen==Screen::Home&&squachSpriteReady&&
     now-lastSquachFrame>=animDelay && colorBarsUntil==0)
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
  bool pressed=lcd.getTouch(&x,&y,450);
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
    }else if(y<23&&x<48){
      screen=Screen::MainMenu;menuPage=0;redraw=true;
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
          screen=targets[n];redraw=true;
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
          backgroundLevel=n;
          if(prefs.begin("sqemit32",false)){prefs.putUChar("bg",backgroundLevel);prefs.end();}
          redraw=true;
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
