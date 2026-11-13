#if defined(SQE_WAVESHARE_C5)
#include "c5_ui.h"
#include "scenarios.h"
#include "scenario_display.h"
#include <Arduino.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "Board_IO.h"
#include "Display_ST7789.h"
#include "Touch_CST3530.h"

// Native portrait 240x320 Waveshare C5 LCD. All board control uses the
// supplier ST7789/CST3530/CH32V003 driver, not guessed GPIOs.
// Compact software font/framebuffer removes incompatible graphics libraries.
namespace {
constexpr int16_t W=240, H=320, FIRST=85, ROW=31;
constexpr size_t PAGE=5;
constexpr uint16_t BG=0x0842, PANEL=0x10A3, SELECT=0x21CC;
constexpr uint16_t WHITE=0xFFFF, CYAN=0x07FF, DIM=0xA514, RED=0xA800, GREEN=0x07E0;
uint16_t* pixels=nullptr;
bool displayReady=false,touchReady=false,dirty=true,wasPressed=false;
size_t selected=0,page=0;
uint32_t lastDraw=0,lastPoll=0,lastPress=0;

// Standard 5x7 glyphs for the exact uppercase UI vocabulary.
// Columns are stored low-bit first; the remaining characters use blank.
static const uint8_t font[42][5]={
 {0x7e,0x11,0x11,0x11,0x7e}, // A
 {0x7f,0x49,0x49,0x49,0x36}, // B
 {0x3e,0x41,0x41,0x41,0x22}, // C
 {0x7f,0x41,0x41,0x22,0x1c}, // D
 {0x7f,0x49,0x49,0x49,0x41}, // E
 {0x7f,0x09,0x09,0x09,0x01}, // F
 {0x3e,0x41,0x49,0x49,0x7a}, // G
 {0x7f,0x08,0x08,0x08,0x7f}, // H
 {0x00,0x41,0x7f,0x41,0x00}, // I
 {0x20,0x40,0x41,0x3f,0x01}, // J
 {0x7f,0x08,0x14,0x22,0x41}, // K
 {0x7f,0x40,0x40,0x40,0x40}, // L
 {0x7f,0x02,0x0c,0x02,0x7f}, // M
 {0x7f,0x04,0x08,0x10,0x7f}, // N
 {0x3e,0x41,0x41,0x41,0x3e}, // O
 {0x7f,0x09,0x09,0x09,0x06}, // P
 {0x3e,0x41,0x51,0x21,0x5e}, // Q
 {0x7f,0x09,0x19,0x29,0x46}, // R
 {0x46,0x49,0x49,0x49,0x31}, // S
 {0x01,0x01,0x7f,0x01,0x01}, // T
 {0x3f,0x40,0x40,0x40,0x3f}, // U
 {0x1f,0x20,0x40,0x20,0x1f}, // V
 {0x3f,0x40,0x38,0x40,0x3f}, // W
 {0x63,0x14,0x08,0x14,0x63}, // X
 {0x07,0x08,0x70,0x08,0x07}, // Y
 {0x61,0x51,0x49,0x45,0x43}, // Z
 {0x3e,0x51,0x49,0x45,0x3e}, // 0
 {0x00,0x42,0x7f,0x40,0x00}, // 1
 {0x42,0x61,0x51,0x49,0x46}, // 2
 {0x21,0x41,0x45,0x4b,0x31}, // 3
 {0x18,0x14,0x12,0x7f,0x10}, // 4
 {0x27,0x45,0x45,0x45,0x39}, // 5
 {0x3c,0x4a,0x49,0x49,0x30}, // 6
 {0x01,0x71,0x09,0x05,0x03}, // 7
 {0x36,0x49,0x49,0x49,0x36}, // 8
 {0x06,0x49,0x49,0x29,0x1e}, // 9
 {0x08,0x08,0x08,0x08,0x08}, // - index 36
 {0x20,0x10,0x08,0x04,0x02}, // / 37
 {0x40,0x40,0x40,0x40,0x40}, // _ 38
 {0x00,0x36,0x36,0x00,0x00}, // : 39
 {0x00,0x60,0x60,0x00,0x00}, // . 40
 {0x00,0x00,0x00,0x00,0x00}  // space 41
};
int glyphIndex(char ch) {
  if(ch>='A'&&ch<='Z')return ch-'A';
  if(ch>='0'&&ch<='9')return 26+ch-'0';
  switch(ch){
    case '-':return 36;case '/':return 37;case '_':return 38;
    case ':':return 39;case '.':return 40;default:return 41;
  }
}
void fill(int x,int y,int w,int h,uint16_t color) {
  if(!pixels || w<=0 || h<=0)return;
  int x0=x<0?0:x, y0=y<0?0:y;
  int x1=x+w>W?W:x+w, y1=y+h>H?H:y+h;
  if(x0>=x1 || y0>=y1)return;
  for(int row=y0;row<y1;++row){
    uint16_t* p=pixels+(size_t)row*W+x0;
    for(int col=x0;col<x1;++col)*p++=color;
  }
}
void text(int x,int y,const char* s,uint16_t color,int scale=1){
  if(!pixels || !s)return;
  for(;*s;s++,x+=6*scale){
    int g=glyphIndex(*s);
    for(int col=0;col<5;++col)
      for(int bit=0;bit<7;++bit)
        if(font[g][col]&(1<<bit))
          fill(x+col*scale,y+bit*scale,scale,scale,color);
  }
}
void rect(int x,int y,int w,int h,uint16_t color){
  fill(x,y,w,1,color);fill(x,y+h-1,w,1,color);
  fill(x,y,1,h,color);fill(x+w-1,y,1,h,color);
}
void button(int x,int y,int w,int h,const char* s,uint16_t color){
  fill(x,y,w,h,color);rect(x,y,w,h,WHITE);
  text(x+(w-(int)strlen(s)*6)/2,y+(h-7)/2,s,WHITE);
}
void deviceCaption(const Scenario& s,char* out,size_t cap){
  scenarioDeviceName(s,out,cap);
  // This board's lightweight 5x7 font contains uppercase glyphs only.
  for(size_t i=0;out[i];++i)
    if(out[i]>='a'&&out[i]<='z')out[i]=(char)(out[i]-'a'+'A');
  if(cap>37 && std::strlen(out)>36)out[36]='\0'; // 6px/character, 216px row
}
void render(const char* active,uint32_t remaining,bool suite){
  fill(0,0,W,H,BG);
  fill(0,0,W,35,SELECT);
  text(8,6,"SQUACHEMIT",CYAN,2);
  text(9,27,"C5 2.8 / RF LAB",WHITE);
  text(7,43,active?"ON AIR":suite?"SUITE":"READY - NO RF",active?RED:GREEN);
  if(active){
    const Scenario* running=findScenario(active);
    char runningName[48]={0};
    if(running)deviceCaption(*running,runningName,sizeof(runningName));
    text(7,58,running?runningName:active,WHITE);
    char timer[24];
    snprintf(timer,sizeof(timer),"%lu SEC LEFT",(unsigned long)((remaining+999)/1000));
    text(7,70,timer,DIM);
  } else text(7,60,"TAP TEST THEN RUN",DIM);
  for(size_t i=0;i<PAGE;++i){
    size_t index=page*PAGE+i;
    if(index>=kScenarioCount)break;
    int y=FIRST+(int)i*ROW;
    fill(5,y,230,28,index==selected?SELECT:PANEL);
    rect(5,y,230,28,index==selected?CYAN:DIM);
    char name[48]={0};
    deviceCaption(kScenarios[index],name,sizeof(name));
    text(11,y+5,name,WHITE);
    char id[37]={0};
    std::snprintf(id,sizeof(id),"%s",kScenarios[index].id);
    text(11,y+17,id,index==selected?CYAN:DIM);
  }
  char pg[24];
  snprintf(pg,sizeof(pg),"%u / %u",(unsigned)(page+1),
           (unsigned)((kScenarioCount+PAGE-1)/PAGE));
  text(104,245,pg,DIM);
  button(5,258,112,27,"PREV",PANEL);
  button(123,258,112,27,"NEXT",PANEL);
  button(5,289,73,28,"RUN",SELECT);
  button(83,289,73,28,"STOP",RED);
  button(161,289,74,28,"SUITE",0x29C8);
  for(int16_t y=0;y<H;++y)
    LCD_addWindow(0,y,W-1,y,pixels+(size_t)y*W);
}
C5UiAction handleTap(int x,int y){
  if(y>=FIRST && y<FIRST+(int)PAGE*ROW){
    size_t index=page*PAGE+(size_t)((y-FIRST)/ROW);
    if(index<kScenarioCount)selected=index;
    return C5UiAction::None;
  }
  if(y>=258 && y<285){
    if(x<120){if(page>0)--page;}
    else if((page+1)*PAGE<kScenarioCount)++page;
    selected=page*PAGE;
    return C5UiAction::None;
  }
  if(y>=289){
    if(x<80)return C5UiAction::Run;
    if(x<159)return C5UiAction::Stop;
    return C5UiAction::Suite;
  }
  return C5UiAction::None;
}
} // namespace

void c5UiBegin(){
  if(!Board_IO_Init(Wire))Serial.println("WARN,C5_CH32_INIT_FAILED");
  Board_SetBacklight(0);
  LCD_Init();
  touchReady=TOUCH2_Init();
  pixels=(uint16_t*)heap_caps_malloc((size_t)W*H*sizeof(uint16_t),
                                    MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  displayReady=(pixels!=nullptr);
  if(!displayReady)Serial.println("WARN,C5_FRAMEBUFFER_UNAVAILABLE;SERIAL_MODE_ACTIVE");
  if(!touchReady)Serial.println("WARN,C5_TOUCH_UNAVAILABLE;SERIAL_MODE_ACTIVE");
  if(displayReady)Backlight_Init();
  dirty=true;
}
const Scenario* c5UiSelection(){
  return selected<kScenarioCount?&kScenarios[selected]:nullptr;
}
C5UiAction c5UiTick(const char* active,uint32_t remaining,bool suite){
  if(!displayReady)return C5UiAction::None;
  const uint32_t now=millis();
  if(dirty || now-lastDraw>=1000){
    render(active,remaining,suite);
    lastDraw=now;
    dirty=false;
  }
  if(!touchReady || now-lastPoll<50)return C5UiAction::None;
  lastPoll=now;
  uint16_t x[5]={0},y[5]={0},strength[5]={0};
  uint8_t count=0;
  Touch2_Read_Data();
  bool pressed=Touch2_Get_XY(x,y,strength,&count,5) && count>0;
  C5UiAction action=C5UiAction::None;
  if(pressed && !wasPressed && now-lastPress>=240 && x[0]<W && y[0]<H){
    action=handleTap((int)x[0],(int)y[0]);
    lastPress=now;
    dirty=true;
  }
  wasPressed=pressed;
  return action;
}
#endif
