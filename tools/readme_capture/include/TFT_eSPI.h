#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>
#include "tft_espi_fonts.h"
namespace MockDisplay {
  class TFTMarker;
  inline bool touch=false;
  inline uint16_t x=0,y=0;
}
class TFT_eSPI;
namespace MockDisplay { inline TFT_eSPI* screen=nullptr; }
class TFT_eSPI {
public:
 int w=320,h=240;
 std::vector<uint16_t> pixel;
 uint16_t fg=0xffff;
 unsigned fillScreenCalls=0; // test-only counter: detect visible full-screen clears
 int cx=0,cy=0,font=1,size=1;
 explicit TFT_eSPI(int width=320,int height=240,bool root=true)
 :w(width),h(height),pixel(size_t(width)*height,0){
   if(root)MockDisplay::screen=this;
 }
 void init(){}
 void setRotation(int){}
 void invertDisplay(bool){} // physical panel inversion not applied to RGB buffer
 void setTouch(uint16_t*){}
 void calibrateTouch(uint16_t*,uint16_t,uint16_t,uint8_t){}
 bool getTouch(uint16_t* xx,uint16_t* yy,uint16_t){
  if(MockDisplay::touch){*xx=MockDisplay::x;*yy=MockDisplay::y;return true;}
  return false;
 }
 bool getTouchRaw(uint16_t* xx,uint16_t* yy){return getTouch(xx,yy,0);}
 static uint16_t color565(int r,int g,int b){
  return (uint16_t)(((r>>3)<<11)|((g>>2)<<5)|(b>>3));
 }
 int width() const{return w;}
 int height() const{return h;}
 // Return the actual drawn RGB565 raster to mirror regression tests.
 void readRect(int x,int y,int ww,int hh,uint16_t* out) const{
  if(!out||ww<0||hh<0)return;
  for(int yy=0;yy<hh;yy++)
   for(int xx=0;xx<ww;xx++){
    const int sx=x+xx,sy=y+yy;
    out[size_t(yy)*ww+xx]=(sx>=0&&sy>=0&&sx<w&&sy<h)?
      pixel[size_t(sy)*w+sx]:0;
   }
 }
 void drawPixel(int x,int y,uint16_t c){
  if(x>=0&&x<w&&y>=0&&y<h)pixel[size_t(y)*w+x]=c;
 }
 void fillScreen(uint16_t c){++fillScreenCalls;std::fill(pixel.begin(),pixel.end(),c);}
 void fillRect(int x,int y,int ww,int hh,uint16_t c){
  int x0=std::max(0,x),y0=std::max(0,y);
  int x1=std::min(w,x+ww),y1=std::min(h,y+hh);
  for(int yy=y0;yy<y1;yy++)
    for(int xx=x0;xx<x1;xx++)pixel[size_t(yy)*w+xx]=c;
 }
 void drawFastHLine(int x,int y,int ww,uint16_t c){fillRect(x,y,ww,1,c);}
 void drawFastVLine(int x,int y,int hh,uint16_t c){fillRect(x,y,1,hh,c);}
 void drawRect(int x,int y,int ww,int hh,uint16_t c){
  drawFastHLine(x,y,ww,c);drawFastHLine(x,y+hh-1,ww,c);
  drawFastVLine(x,y,hh,c);drawFastVLine(x+ww-1,y,hh,c);
 }
 void drawLine(int x0,int y0,int x1,int y1,uint16_t c){
  int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
  while(true){drawPixel(x0,y0,c);if(x0==x1&&y0==y1)break;
   int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}
  }
 }
 void fillCircle(int cx0,int cy0,int r,uint16_t c){
  for(int y=cy0-r;y<=cy0+r;y++)
   for(int x=cx0-r;x<=cx0+r;x++)
    if((x-cx0)*(x-cx0)+(y-cy0)*(y-cy0)<=r*r)drawPixel(x,y,c);
 }
 void fillEllipse(int cx0,int cy0,int rx,int ry,uint16_t c){
  if(rx<1||ry<1)return;
  for(int y=cy0-ry;y<=cy0+ry;y++)
   for(int x=cx0-rx;x<=cx0+rx;x++)
    if((x-cx0)*(x-cx0)*ry*ry+(y-cy0)*(y-cy0)*rx*rx<=rx*rx*ry*ry)
     drawPixel(x,y,c);
 }
 bool roundInside(int xx,int yy,int x,int y,int ww,int hh,int r){
  if(xx<x||xx>=x+ww||yy<y||yy>=y+hh)return false;
  r=std::min(r,std::min(ww/2,hh/2));
  int nearestX=std::clamp(xx,x+r,x+ww-r-1);
  int nearestY=std::clamp(yy,y+r,y+hh-r-1);
  int dx=xx-nearestX,dy=yy-nearestY;
  return dx*dx+dy*dy<=r*r;
 }
 void fillRoundRect(int x,int y,int ww,int hh,int r,uint16_t c){
  for(int yy=std::max(0,y);yy<std::min(h,y+hh);yy++)
   for(int xx=std::max(0,x);xx<std::min(w,x+ww);xx++)
    if(roundInside(xx,yy,x,y,ww,hh,r))drawPixel(xx,yy,c);
 }
 void drawRoundRect(int x,int y,int ww,int hh,int r,uint16_t c){
  for(int yy=std::max(0,y);yy<std::min(h,y+hh);yy++)
   for(int xx=std::max(0,x);xx<std::min(w,x+ww);xx++)
    if(roundInside(xx,yy,x,y,ww,hh,r)&&
       !roundInside(xx,yy,x+1,y+1,ww-2,hh-2,std::max(0,r-1)))
      drawPixel(xx,yy,c);
 }
 static int cross(int x0,int y0,int x1,int y1,int x,int y){
  return (x-x0)*(y1-y0)-(y-y0)*(x1-x0);
 }
 void fillTriangle(int x0,int y0,int x1,int y1,int x2,int y2,uint16_t c){
  int xmin=std::max(0,std::min({x0,x1,x2})),xmax=std::min(w-1,std::max({x0,x1,x2}));
  int ymin=std::max(0,std::min({y0,y1,y2})),ymax=std::min(h-1,std::max({y0,y1,y2}));
  for(int yy=ymin;yy<=ymax;yy++)
   for(int xx=xmin;xx<=xmax;xx++){
    int a=cross(x0,y0,x1,y1,xx,yy);
    int b=cross(x1,y1,x2,y2,xx,yy);
    int d=cross(x2,y2,x0,y0,xx,yy);
    if((a>=0&&b>=0&&d>=0)||(a<=0&&b<=0&&d<=0))drawPixel(xx,yy,c);
   }
 }
 void setTextFont(uint8_t f){font=(f==2?2:1);}
 void setTextSize(uint8_t s){size=(s?s:1);}
 void setTextColor(uint16_t c){fg=c;}
 void setCursor(int x,int y){cx=x;cy=y;}
 int charWidth(unsigned ch) const {
   if(font==1)return 6*size;
   if(ch<32||ch>127)ch=' ';
   return int(FontPixels::font2_width[ch-32])*size;
 }
 int textWidth(const char* s) const {
   if(!s)return 0;
   int width=0;
   for(const unsigned char* c=(const unsigned char*)s;*c;c++)
     width+=charWidth(*c);
   return width;
 }
 // Match the actual TFT_eSPI GLCD 5x8 bit-columns (font 1) and
 // Font16.c row-byte glyphs + proportional advance widths (font 2).
 void drawGlyph(unsigned ch,int x,int y) {
   if(ch<32||ch>127)ch='?';
   if(font==1){
     for(int gx=0;gx<5;gx++){
       uint8_t col=FontPixels::glcd[ch*5+gx];
       for(int gy=0;gy<8;gy++)
         if(col&(1u<<gy))fillRect(x+gx*size,y+gy*size,size,size,fg);
     }
   }else{
     const int gw=int(FontPixels::font2_width[ch-32]);
     const int bytes=(gw+6)/8; // TFT_eSPI Font 2's special byte rounding
     for(int yy=0;yy<16;yy++)
       for(int xb=0;xb<bytes;xb++){
         const uint8_t data=FontPixels::font2_bitmap[ch-32][yy*bytes+xb];
         for(int bit=0;bit<8;bit++)
           if(data&(0x80u>>bit))
             fillRect(x+(xb*8+bit)*size,y+yy*size,size,size,fg);
       }
   }
 }
 void print(const char* s){
   if(!s)return;
   for(const unsigned char* c=(const unsigned char*)s;*c;c++){
     drawGlyph(*c,cx,cy);
     cx+=charWidth(*c);
   }
 }
};
class TFT_eSprite : public TFT_eSPI {
 TFT_eSPI* target;
public:
 explicit TFT_eSprite(TFT_eSPI* parent):TFT_eSPI(112,111,false),target(parent){}
 void setColorDepth(int){}
 void* createSprite(int width,int height){
  w=width;h=height;pixel.assign(size_t(w)*h,0);
  return pixel.data();
 }
 void fillSprite(uint16_t c){fillScreen(c);}
 void pushSprite(int x,int y){
  for(int yy=0;yy<h;yy++)for(int xx=0;xx<w;xx++)
   target->drawPixel(x+xx,y+yy,pixel[size_t(yy)*w+xx]);
 }
};
constexpr uint16_t TFT_RED=0xF800,TFT_GREEN=0x07E0,TFT_BLUE=0x001F,TFT_WHITE=0xFFFF,TFT_BLACK=0;
