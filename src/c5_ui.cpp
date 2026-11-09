#if defined(SQE_WAVESHARE_C5)
#include "c5_ui.h"
#include "scenarios.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <esp_heap_caps.h>
#include <string.h>
#include "Board_IO.h"
#include "Display_ST7789.h"
#include "Touch_CST3530.h"

// Native portrait 240x320 Waveshare C5 LCD. Framebuffer is in PSRAM.
// The vendor BSP owns reset, touch and PWM backlight through CH32V003.
namespace {
constexpr int16_t WIDTH=240, HEIGHT=320, FIRST_ROW=85, ROW_HEIGHT=31;
constexpr size_t PAGE_SIZE=5;
constexpr uint16_t BG=0x0842, PANEL=0x10A3, SELECTED=0x21CC;
constexpr uint16_t WHITE=0xFFFF, CYAN=0x07FF, DIM=0xA514, RED=0xA800, GREEN=0x07E0;
uint16_t* image=nullptr;
bool panelReady=false, touchReady=false, dirty=true, wasPressed=false;
size_t selected=0, page=0;
uint32_t lastDraw=0, lastTouchPoll=0, lastPress=0;

class Frame : public Adafruit_GFX {
public:
  Frame():Adafruit_GFX(WIDTH,HEIGHT){}
  void drawPixel(int16_t x,int16_t y,uint16_t c) override {
    if(image && x>=0 && y>=0 && x<WIDTH && y<HEIGHT)
      image[(size_t)y*WIDTH+x]=c;
  }
  void fillRect(int16_t x,int16_t y,int16_t w,int16_t h,uint16_t c) override {
    if(!image || w<=0 || h<=0) return;
    int32_t x0=x<0?0:x, y0=y<0?0:y;
    int32_t x1=(int32_t)x+w, y1=(int32_t)y+h;
    if(x1>WIDTH)x1=WIDTH;
    if(y1>HEIGHT)y1=HEIGHT;
    if(x0>=x1 || y0>=y1)return;
    for(int32_t row=y0;row<y1;++row){
      uint16_t* out=image+row*WIDTH+x0;
      for(int32_t col=x0;col<x1;++col)*out++=c;
    }
  }
  void fillScreen(uint16_t c) override {fillRect(0,0,WIDTH,HEIGHT,c);}
  void flush(){
    if(!image)return;
    for(int16_t y=0;y<HEIGHT;++y)
      LCD_addWindow(0,y,WIDTH-1,y,image+(size_t)y*WIDTH);
  }
} display;

void label(int16_t x,int16_t y,const char* s,uint16_t color,uint8_t size=1){
  display.setCursor(x,y);
  display.setTextColor(color);
  display.setTextSize(size);
  display.print(s);
}
void button(int x,int y,int w,int h,const char* s,uint16_t bg){
  display.fillRoundRect(x,y,w,h,4,bg);
  display.drawRoundRect(x,y,w,h,4,WHITE);
  label(x+(w-(int)strlen(s)*6)/2,y+(h-8)/2,s,WHITE);
}
void render(const char* active,uint32_t remaining,bool suite){
  display.fillScreen(BG);
  display.fillRect(0,0,WIDTH,35,SELECTED);
  label(8,6,"SQUACHEMIT",CYAN,2);
  label(9,28,"C5 2.8 / RF LAB",WHITE);
  label(7,43,active?"ON AIR":(suite?"SUITE":"READY - NO RF"),active?RED:GREEN);
  if(active){
    label(7,58,active,WHITE);
    char timer[24];
    snprintf(timer,sizeof(timer),"%lu seconds left",(unsigned long)((remaining+999)/1000));
    label(7,70,timer,DIM);
  } else label(7,60,"Tap a test then RUN",DIM);
  for(size_t i=0;i<PAGE_SIZE;++i){
    size_t idx=page*PAGE_SIZE+i;
    if(idx>=kScenarioCount)break;
    int y=FIRST_ROW+(int)i*ROW_HEIGHT;
    display.fillRoundRect(5,y,230,28,4,idx==selected?SELECTED:PANEL);
    display.drawRoundRect(5,y,230,28,4,idx==selected?CYAN:DIM);
    label(11,y+5,kScenarios[idx].id,WHITE);
    label(11,y+17,kScenarios[idx].expected,idx==selected?CYAN:DIM);
  }
  char pageLabel[30];
  snprintf(pageLabel,sizeof(pageLabel),"%u / %u",
           (unsigned)(page+1),(unsigned)((kScenarioCount+PAGE_SIZE-1)/PAGE_SIZE));
  label(104,245,pageLabel,DIM);
  button(5,258,112,27,"PREV",PANEL);
  button(123,258,112,27,"NEXT",PANEL);
  button(5,289,73,28,"RUN",SELECTED);
  button(83,289,73,28,"STOP",RED);
  button(161,289,74,28,"SUITE",0x29C8);
  display.flush();
}
C5UiAction touch(int16_t x,int16_t y){
  if(y>=FIRST_ROW && y<FIRST_ROW+(int)PAGE_SIZE*ROW_HEIGHT){
    size_t index=page*PAGE_SIZE+(size_t)((y-FIRST_ROW)/ROW_HEIGHT);
    if(index<kScenarioCount)selected=index;
    return C5UiAction::None;
  }
  if(y>=258 && y<285){
    if(x<120){if(page>0)--page;}
    else if((page+1)*PAGE_SIZE<kScenarioCount)++page;
    selected=page*PAGE_SIZE;
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
  const bool io=Board_IO_Init(Wire);
  if(!io)Serial.println("WARN,C5_CH32_INIT_FAILED");
  Board_SetBacklight(0);
  LCD_Init();
  touchReady=TOUCH2_Init();
  image=(uint16_t*)heap_caps_malloc((size_t)WIDTH*HEIGHT*2,
                                    MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
  panelReady=image!=nullptr;
  if(!panelReady)Serial.println("WARN,C5_FRAMEBUFFER_UNAVAILABLE;SERIAL_MODE_ACTIVE");
  if(!touchReady)Serial.println("WARN,C5_TOUCH_UNAVAILABLE;SERIAL_MODE_ACTIVE");
  if(panelReady)Backlight_Init();
  dirty=true;
}
const Scenario* c5UiSelection(){
  return selected<kScenarioCount?&kScenarios[selected]:nullptr;
}
C5UiAction c5UiTick(const char* active,uint32_t remaining,bool suite){
  if(!panelReady)return C5UiAction::None;
  const uint32_t now=millis();
  if(dirty || now-lastDraw>=1000){
    render(active,remaining,suite);
    lastDraw=now;
    dirty=false;
  }
  if(!touchReady || now-lastTouchPoll<50)return C5UiAction::None;
  lastTouchPoll=now;
  uint16_t x[5]={0},y[5]={0},strength[5]={0};
  uint8_t n=0;
  Touch2_Read_Data();
  const bool pressed=Touch2_Get_XY(x,y,strength,&n,5)&&n>0;
  C5UiAction action=C5UiAction::None;
  if(pressed && !wasPressed && now-lastPress>=240 && x[0]<WIDTH && y[0]<HEIGHT){
    action=touch((int16_t)x[0],(int16_t)y[0]);
    lastPress=now;
    dirty=true;
  }
  wasPressed=pressed;
  return action;
}
#endif
