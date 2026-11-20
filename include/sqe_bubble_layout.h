// SquachEmit CYD32: pixel-bounded, centered, content-hugging speech box.
// Geometry is pure so native tests can catch clipping before flashing.
#pragma once

namespace SqeBubbleLayout {
struct Rect { int x,y,w,h; };
inline Rect bounds(int primaryWidth,int detailWidth,bool hasDetail,float scale){
  // Font 2 is 16px high; keep 11-13px of horizontal breathing room
  // and 7px vertical breathing room for a single line.
  int textWidth=primaryWidth>detailWidth?primaryWidth:detailWidth;
  if(textWidth<0)textWidth=0;
  if(textWidth>282)textWidth=282;
  int baseW=textWidth+26;
  if(baseW<74)baseW=74;
  if(baseW>306)baseW=306;
  const int baseH=hasDetail?44:30;
  int w=(int)(baseW*scale+0.5f);
  int h=(int)(baseH*scale+0.5f);
  if(w<5)w=5;
  if(h<4)h=4;
  // 220ms pop can overshoot 3%; stay inside the 320x240 panel.
  if(w>310)w=310;
  if(h>46)h=46;
  return {(320-w)/2,20+(44-h)/2,w,h};
}
inline int centeredTextX(Rect box,int textWidth){
  return box.x+(box.w-textWidth)/2;
}
}
