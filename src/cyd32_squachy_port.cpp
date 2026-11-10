// LAB source port from retrodroid32/SquachWatch-CYD/src/squachy.cpp.
// Character geometry and movement formulas adapted from GPLv3 code.
// SPDX-License-Identifier: GPL-3.0-only
#if defined(SQE_CYD32)
#include "cyd32_squachy_port.h"
#include <TFT_eSPI.h>
#include <Arduino.h>
#include <math.h>
namespace SquachyPort {
static constexpr uint16_t FUR=0x5941, LIGHT=0x9326, TAN=0xF60F, SKIN=0xCB88;
static constexpr uint16_t BLACK=0, WHITE=0xFFFF, CYAN=0x07FF, PINK=0xF96F;
static constexpr float K=1.5f;
static int S(int n){return (int)(n*K);}
static void wedge(TFT_eSprite& t,float ax,float ay,float bx,float by,float w,uint16_t c){
  // The source Squachy integer wedge: 2 triangles and 2 caps, not AA.
  const float dx=bx-ax,dy=by-ay,len=sqrtf(dx*dx+dy*dy);
  const int r=(int)(w/2);
  if(len<0.01f){t.fillCircle((int)ax,(int)ay,r,c);return;}
  const float nx=-dy/len*(w/2),ny=dx/len*(w/2);
  t.fillTriangle((int)(ax+nx),(int)(ay+ny),(int)(ax-nx),(int)(ay-ny),(int)(bx+nx),(int)(by+ny),c);
  t.fillTriangle((int)(bx+nx),(int)(by+ny),(int)(bx-nx),(int)(by-ny),(int)(ax-nx),(int)(ay-ny),c);
  t.fillCircle((int)ax,(int)ay,r,c); t.fillCircle((int)bx,(int)by,r,c);
}
static void backdrop(TFT_eSprite& t){
  // Patch coordinates: top-left at absolute LCD (104,70).
  t.fillSprite(t.color565(4,4,18));
  for(int y=72;y<155;y+=8){
    const uint8_t sh=(uint8_t)((y-16)/8);
    t.drawFastHLine(0,y-70,112,t.color565(4+sh/3,5+sh/4,25+sh));
  }
  t.fillTriangle(-42,76,34,23,97,76,t.color565(15,2,36));
  t.fillTriangle(62,76,148,13,216,76,t.color565(20,3,48));
  t.fillRect(7,46,12,52,t.color565(5,2,24));
  t.drawFastVLine(7,46,52,0xBB5F);
  for(int y=51;y<93;y+=10)t.fillRect(11,y,2,3,CYAN);
  t.fillRect(0,98,112,13,t.color565(3,1,13));
  t.drawFastHLine(0,97,112,0xFB99);
  for(int x=0;x<320;x+=28)t.drawLine(56,97,x-104,111,0xAC1F);
  t.drawFastHLine(0,104,112,0xAC1F);
  t.drawFastHLine(0,110,112,CYAN);
}
void render(TFT_eSprite& t,uint32_t now,bool onAir,bool suite,bool pet){
  if(t.width()!=112||t.height()!=111)return;
  backdrop(t);
  // Preserve SquachWatch's source body proportions, procedural eye timing,
  // arm movement, head bob, bounce, wave, shocked, dance, walking and nap.
  const uint32_t phase=now%22000u;
  const bool wave=pet||(!onAir&&!suite&&phase>12000&&phase<13400);
  const bool sleep=!onAir&&!suite&&phase>19000&&!pet;
  const bool walk=!onAir&&!suite&&phase>16200&&phase<18800;
  const bool bounce=!onAir&&!suite&&phase>6500&&phase<7400;
  const bool wink=!onAir&&!suite&&phase>9600&&phase<10300;
  const float speed=bounce?220.0f:(sleep?2200.0f:1100.0f);
  const float amount=(bounce?9.0f:(sleep?1.5f:3.0f))*K;
  float bob=sinf((float)(now%(uint32_t)speed)/speed*6.2831853f)*amount;
  if(bounce)bob=-fabsf(bob);
  const int cx=56+(walk?(int)(sinf(now/700.0f)*S(5)):0);
  const int hy=22+(int)bob, hh=hy-(int)(bob*0.16f);
  const float ph=(float)(now%400u)/400.0f*6.2831853f;
  const int leg=(walk?(int)(sinf(ph)*S(3)):0);
  for(int side=-1;side<=1;side+=2){
    const int lx=cx+(side<0?-S(10):S(2));
    const int foot=cx+(side<0?-S(13):S(1));
    const int shift=side<0?leg:-leg;
    t.fillRect(lx-1,hy+S(40)-1,S(8)+2,S(10)+shift+2,BLACK);
    t.fillRect(lx,hy+S(40),S(8),S(10)+shift,FUR);
    t.fillRoundRect(foot-1,hy+S(49)+shift-1,S(12)+2,S(6)+2,2,BLACK);
    t.fillRoundRect(foot,hy+S(49)+shift,S(12),S(6),2,LIGHT);
  }
  t.fillRoundRect(cx-S(12),hy+S(22),S(24),S(20),S(5),BLACK);
  t.fillRoundRect(cx-S(11),hy+S(23),S(22),S(18),S(5),FUR);
  auto limb=[&](int x0,int y0,int x1,int y1){
    wedge(t,x0,y0,x1,y1,S(7)+2,BLACK);
    wedge(t,x0,y0,x1,y1,S(7),LIGHT);
  };
  if(wave){
    const float a=-1.0f+sinf((float)(now%400u)/400.0f*6.2831853f)*0.5f;
    limb(cx+S(11),hy+S(28),cx+S(13)+(int)(cosf(a)*18*K),hy+S(28)+(int)(sinf(a)*18*K));
    limb(cx-S(14),hy+S(24),cx-S(14),hy+S(44));
  }else if(onAir){
    const int shake=(int)(sinf((float)(now%140u)/140.0f*6.2831853f)*S(2));
    limb(cx-S(11),hy+S(26),cx-S(23)+shake,hy+S(10));
    limb(cx+S(11),hy+S(26),cx+S(23)+shake,hy+S(10));
  }else if(suite){
    const int dx=(int)(sinf((float)(now%500u)/500.0f*6.2831853f)*S(11));
    limb(cx-S(14),hy+S(20),cx-S(14)+dx,hy+S(44));
    limb(cx+S(14),hy+S(20),cx+S(14)+dx,hy+S(44));
  }else{
    const float a=(float)(now%1800u)/1800.0f*6.2831853f;
    const int sway=(int)(sinf(a)*S(3));
    const int l=walk?leg:sway, r=walk?-leg:-sway;
    t.fillRoundRect(cx-S(18)-1,hy+S(22)+l-1,S(8)+2,S(22)+2,S(3),BLACK);
    t.fillRoundRect(cx+S(10)-1,hy+S(22)+r-1,S(8)+2,S(22)+2,S(3),BLACK);
    t.fillRoundRect(cx-S(18),hy+S(22)+l,S(8),S(22),S(3),LIGHT);
    t.fillRoundRect(cx+S(10),hy+S(22)+r,S(8),S(22),S(3),LIGHT);
  }
  t.fillRoundRect(cx-S(16),hh-1,S(32),S(26),S(8),BLACK);
  t.fillRoundRect(cx-S(15),hh,S(30),S(24),S(7),LIGHT);
  t.fillRoundRect(cx-S(12),hh+S(2),S(24),S(19),S(5),FUR);
  t.fillRoundRect(cx-S(9),hh+S(7),S(18),S(11),S(4),TAN);
  // Authentic asymmetric cowlick/fringe from SquachWatch default Squachy.
  const int tuft[4][6]={{-7,2,-4,-5,-1,2},{-2,2,6,-7,6,2},{-13,3,-9,-4,-5,3},{5,3,9,-4,13,3}};
  for(const auto& p:tuft)
    t.fillTriangle(cx+S(p[0]),hh+S(p[1]),cx+S(p[2]),hh+S(p[3]),
                   cx+S(p[4]),hh+S(p[5]),LIGHT);
  for(int side=-1;side<=1;side+=2){
    t.fillCircle(cx+side*S(15),hh+S(13),S(3),FUR);
    t.fillCircle(cx+side*S(15),hh+S(13),S(1),SKIN);
    t.fillCircle(cx+side*S(8),hh+S(15),S(2),0xFB99);
  }
  if(onAir){
    for(int side=-1;side<=1;side+=2){
      t.fillEllipse(cx+side*S(5),hh+S(9),S(3),S(4),WHITE);
      t.fillCircle(cx+side*S(5),hh+S(9),S(1),BLACK);
    }
    t.fillEllipse(cx,hh+S(18),S(4),S(5),BLACK);
    return;
  }
  if(sleep){
    t.drawLine(cx-S(9),hh+S(9),cx-S(2),hh+S(11),LIGHT);
    t.drawLine(cx+S(2),hh+S(11),cx+S(9),hh+S(9),LIGHT);
    t.fillCircle(cx,hh+S(18),S(2),BLACK);
    return;
  }
  // SquachWatch's deterministic varying single/double/slow blinks.
  const uint32_t slot=now/2600u;
  uint32_t h=slot*2654435761u;
  h^=h>>15;h*=2246822519u;h^=h>>13;
  const uint32_t t0=slot*2600u+300u+(h%1700u);
  const uint8_t kind=(uint8_t)((h>>20)&7u);
  const bool blink=kind==0?(now>=t0&&now<t0+440u):
    kind==1?((now>=t0&&now<t0+140u)||(now>=t0+230u&&now<t0+370u)):
    (now>=t0&&now<t0+140u);
  const uint16_t tint=0x0113;
  const uint16_t lensL=blink||wink?BLACK:tint, lensR=blink?BLACK:tint;
  t.fillRoundRect(cx-S(12),hh+S(6),S(10),S(7),2,BLACK);
  t.fillRoundRect(cx+S(2),hh+S(6),S(10),S(7),2,BLACK);
  const int bx0=cx-S(12)+S(10)-1,bx1=cx+S(2);
  t.fillRect(bx0,hh+S(8),bx1-bx0+1,S(2),BLACK);
  t.fillRoundRect(cx-S(11),hh+S(7),S(8),S(5),1,lensL);
  t.fillRoundRect(cx+S(3),hh+S(7),S(8),S(5),1,lensR);
  const int gx=(int)(((float)(now%2400u)/2400.0f)*6.0f);
  if(lensL!=BLACK)t.drawFastVLine(cx-S(11)+S(1+gx),hh+S(7),S(4),WHITE);
  if(lensR!=BLACK)t.drawFastVLine(cx+S(3)+S(1+gx),hh+S(7),S(4),WHITE);
  t.fillRoundRect(cx-S(8),hh+S(16),S(16),S(7),S(3),BLACK);
  t.fillRect(cx-S(6),hh+S(17),S(12),S(2),WHITE);
  t.fillRect(cx-S(6),hh+S(19),S(12),S(3),PINK);
}
} // namespace SquachyPort
#endif
