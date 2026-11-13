/* SquachEmit CYD 3.2 native desktop UI renderer.
 * No bitmap capture, TFT GRAM reads, MIRROR FRAME, or screenshot decoding.
 * PC draws from firmware's logical UI STATE model using local Canvas2D.
 * 320x240 logical coordinates and palette copied from SquachEmit's own
 * CYD32 C++ renderer; navigation is controlled by native firmware state.
 */
(function(root,factory){
  'use strict';
  const library=factory();
  if(typeof module!=='undefined'&&module.exports)module.exports=library;
  if(root)root.SQECydNative=library;
})(typeof window==='undefined'?null:window,function(){
  'use strict';
  const W=320,H=240;
  const C={bg:0x0801,taskbar:0x0803,purple:0xAC1F,cyan:0x07FF,
    pink:0xF96F,vaporPink:0xFB99,vaporPurple:0xBB5F,yellow:0xFFD2,
    green:0x07E0,white:0xFFFF,muted:0x7393,ink:0x1801,
    furDark:0x38C0,furMain:0x5941,furLight:0x9326,skin:0xF60F,skinDark:0xCB88};
  const CATEGORIES=['AXON','DRONES / REMOTE ID','FLOCK','GENERAL / CONTROLS',
    'HACKER DEVICES','IBEACON','RAVEN','SERIAL MODULES','SMART GLASSES','TRACKERS'];
  function hex565(w){
    w=Number(w)&65535;
    const r=(w>>11)&31,g=(w>>5)&63,b=w&31;
    return '#'+[Math.round(r*255/31),Math.round(g*255/63),
      Math.round(b*255/31)].map(x=>x.toString(16).padStart(2,'0')).join('');
  }
  function parse(line){
    const a=String(line||'').trim().split(/ +/);
    if(a.length!==17||a[0]!=='UI'||a[1]!=='STATE'||a[2]!=='1')
      return null;
    const screens=['HOME','CATEGORIES','LIBRARY','DURATION','CALIBRATION','COLORS'];
    if(!screens.includes(a[3]))return null;
    const num=(i,max)=>/^\d+$/.test(a[i])&&Number.isSafeInteger(Number(a[i]))
      &&Number(a[i])<=max?Number(a[i]):null;
    const cat=num(4,9),page=num(5,100),catPage=num(6,5);
    const duration=num(8,30),remaining=num(10,60000);
    const suite=num(11,1),calibrated=num(12,1),petMs=num(15,2000),uptime=num(16,4294967295);
    if([cat,page,catPage,duration,remaining,suite,calibrated,petMs,uptime].some(x=>x===null)
      ||duration<3||!['NONE','IDLE'].includes(a[7])&&!/^[A-Z0-9-]{1,60}$/.test(a[7])
      ||!['IDLE','NONE'].includes(a[9])&&!/^[A-Z0-9-]{1,60}$/.test(a[9])
      ||![13,14].every(i=>/^[A-Z0-9_-]{1,60}$/.test(a[i])))return null;
    return {screen:a[3],category:cat,page,categoryPage:catPage,
      selected:a[7],duration,active:a[9],remaining,suite:!!suite,
      calibrated:!!calibrated,starting:a[13],error:a[14],petMs,uptime};
  }
  function categoryFor(s){
    const id=s.id||'';
    if(/^RID-|^OUI-|^BLE-DRONE$|^BLE-NEG-DRONE-UUID$/.test(id))return 1;
    if(/^BLE-AXON|^WIFI-AXON|^WIFI-NEG-AXON-NOHYPHEN$/.test(id))return 0;
    if(/^BLE-FLOCK|^WIFI-FLOCK|^BLE-NEG-PENGUIN-9$|^BLE-NEG-BARE9$|^WIFI-NEG-FLOCKHART$/.test(id))return 2;
    if(/^BLE-META|^BLE-NEG-META-ADJ$|^BLE-NEG-SNAP-ADJ$/.test(id))return 8;
    if(/^BLE-RAVEN|^BLE-NEG-RAVEN-3600$/.test(id))return 6;
    if(/^BLE-SKIM|^BLE-NEG-JDY16-EXTRA$/.test(id))return 7;
    if(/^BLE-HACKER|^WIFI-HACKER|^BLE-NEG-FLIP-MFG$|^WIFI-NEG-PAGER-/.test(id))return 4;
    if(/^BLE-IBEACON|^BLE-NEG-IBEACON-SHORT$/.test(id))return 5;
    if(/^BLE-AIRTAG|^BLE-TILE|^BLE-SAMSUNG|^BLE-GOOGLE|^BLE-NEG-APPLE$/.test(id))return 9;
    return 3;
  }
  function prepare(cases){
    const all=Array.isArray(cases)?cases:[];
    const byId=new Map(all.map(c=>[c.id,c])),groups=CATEGORIES.map(()=>[]);
    for(const item of all)groups[categoryFor(item)].push(item);
    for(const group of groups)group.sort((a,b)=>a.id.localeCompare(b.id,'en'));
    return {byId,groups,count:all.length};
  }
  function P(ctx){
    function color(w){return typeof w==='number'?hex565(w):w;}
    function fill(x,y,w,h,c){ctx.fillStyle=color(c);ctx.fillRect(Math.round(x),Math.round(y),Math.max(0,Math.round(w)),Math.max(0,Math.round(h)));}
    function line(x0,y0,x1,y1,c){ctx.strokeStyle=color(c);ctx.lineWidth=1;ctx.beginPath();
      ctx.moveTo(Math.round(x0)+0.5,Math.round(y0)+0.5);
      ctx.lineTo(Math.round(x1)+0.5,Math.round(y1)+0.5);ctx.stroke();}
    function outline(x,y,w,h,c){line(x,y,x+w-1,y,c);line(x,y,x,y+h-1,c);
      line(x+w-1,y,x+w-1,y+h-1,c);line(x,y+h-1,x+w-1,y+h-1,c);}
    function tri(x1,y1,x2,y2,x3,y3,c){ctx.fillStyle=color(c);ctx.beginPath();
      ctx.moveTo(x1,y1);ctx.lineTo(x2,y2);ctx.lineTo(x3,y3);ctx.closePath();ctx.fill();}
    function round(x,y,w,h,r,c){ctx.fillStyle=color(c);ctx.beginPath();
      if(ctx.roundRect)ctx.roundRect(Math.round(x),Math.round(y),Math.max(0,w),Math.max(0,h),Math.max(0,Math.min(r,w/2,h/2)));
      else ctx.rect(Math.round(x),Math.round(y),Math.max(0,w),Math.max(0,h));
      ctx.fill();}
    function ring(x,y,w,h,r,c){ctx.strokeStyle=color(c);ctx.lineWidth=1;ctx.beginPath();
      if(ctx.roundRect)ctx.roundRect(x+.5,y+.5,w-1,h-1,r);
      else ctx.rect(x+.5,y+.5,w-1,h-1);
      ctx.stroke();}
    function circle(x,y,r,c){ctx.fillStyle=color(c);ctx.beginPath();ctx.arc(x,y,Math.max(0,r),0,Math.PI*2);ctx.fill();}
    function ellipse(x,y,rx,ry,c){ctx.fillStyle=color(c);ctx.beginPath();ctx.ellipse(x,y,Math.max(.01,rx),Math.max(.01,ry),0,0,Math.PI*2);ctx.fill();}
    function text(x,y,str,c,font=1,maxWidth){
      const str0=String(str??'');
      ctx.textBaseline='top';ctx.textAlign='left';ctx.fillStyle=color(c);
      ctx.font=font===2?'bold 13px monospace':'8px monospace';
      let output=str0;
      if(maxWidth){while(output.length&&ctx.measureText(output).width>maxWidth)output=output.slice(0,-1);}
      ctx.fillText(output,x,y);
    }
    function blend(a,b,t){const ar=(a>>11)&31,ag=(a>>5)&63,ab=a&31;
      const br=(b>>11)&31,bg=(b>>5)&63,bb=b&31;
      return ((ar+Math.trunc((br-ar)*t/255))<<11)|
        ((ag+Math.trunc((bg-ag)*t/255))<<5)|(ab+Math.trunc((bb-ab)*t/255));
    }
    return {color,fill,line,outline,tri,round,ring,circle,ellipse,text,blend};
  }
  function title(p){
    for(let x=0;x<W;x++){const c=x<160?p.blend(C.purple,C.cyan,Math.trunc(x*255/160)):
      p.blend(C.cyan,C.pink,Math.trunc((x-160)*255/160));p.fill(x,0,1,14,c);}
    p.fill(0,14,320,1,C.purple);p.fill(94,2,132,10,C.taskbar);
    p.text(123,3,'SQUACHEMIT',C.white);
  }
  function button(p,x,y,w,h,s){
    p.fill(x,y,w,h,C.bg);p.outline(x,y,w,h,C.purple);
    p.text(x+(w-s.length*4.7)/2,y+(h-8)/2,s,C.cyan);
  }
  function footer(p,a,b,c){button(p,8,214,96,23,a);button(p,112,214,96,23,b);button(p,216,214,96,23,c);}
  function backdrop(p){
    p.fill(0,15,320,164,'#040412');
    for(let y=16;y<155;y+=8){const sh=Math.trunc((y-16)/8);
      p.fill(0,y,320,1,rgb(4+Math.trunc(sh/3),5+Math.trunc(sh/4),25+sh));}
    const stars=[[13,27],[48,42],[81,23],[114,37],[154,25],[203,39],[243,26],
      [290,33],[306,47],[27,66],[65,79],[94,55],[137,86],[181,63],
      [236,81],[277,68],[311,94],[17,116],[73,124],[222,116],[296,130]];
    for(const [x,y] of stars){p.fill(x,y,1,1,C.cyan);if(x%3===0)p.fill(x+1,y,1,1,C.vaporPurple);}
    p.tri(0,146,53,87,115,146,rgb(17,3,44));
    p.tri(62,146,138,93,201,146,rgb(15,2,36));
    p.tri(166,146,252,83,320,146,rgb(20,3,48));
    const buildings=[[0,132,19],[22,116,15],[42,128,14],[61,100,19],
      [86,123,18],[111,116,12],[215,116,20],[240,99,16],
      [263,128,17],[284,107,20],[308,123,12]];
    for(const [x,y,w] of buildings){
      p.fill(x,y,w,168-y,rgb(5,2,24));p.fill(x,y,1,168-y,C.vaporPurple);
      for(let yy=y+5;yy<163;yy+=10)for(let xx=x+4;xx<x+w-2;xx+=7)
        p.fill(xx,yy,2,3,(xx+yy)%3===0?C.pink:C.cyan);
    }
    p.fill(0,168,320,13,rgb(3,1,13));p.fill(0,167,320,1,C.vaporPink);
    for(let x=0;x<320;x+=28)p.line(160,167,x,181,C.purple);
    p.fill(0,174,320,1,C.purple);p.fill(0,180,320,1,C.cyan);
  }
  function rgb(r,g,b){return '#'+[r,g,b].map(x=>Math.max(0,Math.min(255,x)).toString(16).padStart(2,'0')).join('');}
  function squachy(ctx,p,now,onAir,suite,pet){
    ctx.save();ctx.beginPath();ctx.rect(104,70,112,111);ctx.clip();
    ctx.translate(104,70);
    p=P(ctx);p.fill(0,0,112,111,rgb(4,4,18));
    for(let y=72;y<155;y+=8){const sh=Math.trunc((y-16)/8);
      p.fill(0,y-70,112,1,rgb(4+Math.trunc(sh/3),5+Math.trunc(sh/4),25+sh));}
    p.tri(-42,76,34,23,97,76,rgb(15,2,36));
    p.tri(62,76,148,13,216,76,rgb(20,3,48));
    p.fill(7,46,12,52,rgb(5,2,24));p.fill(7,46,1,52,C.vaporPurple);
    for(let y=51;y<93;y+=10)p.fill(11,y,2,3,C.cyan);
    p.fill(0,98,112,13,rgb(3,1,13));p.fill(0,97,112,1,C.vaporPink);
    for(let x=0;x<320;x+=28)p.line(56,97,x-104,111,C.purple);
    p.fill(0,104,112,1,C.purple);p.fill(0,110,112,1,C.cyan);
    const K=1.5,S=n=>Math.trunc(n*K),phase=now%22000;
    const wave=pet||(!onAir&&!suite&&phase>12000&&phase<13400);
    const sleep=!onAir&&!suite&&phase>19000&&!pet;
    const walk=!onAir&&!suite&&phase>16200&&phase<18800;
    const bounce=!onAir&&!suite&&phase>6500&&phase<7400;
    const wink=!onAir&&!suite&&phase>9600&&phase<10300;
    const speed=bounce?220:sleep?2200:1100;
    let bob=Math.sin((now%speed)/speed*2*Math.PI)*(bounce?9:sleep?1.5:3)*K;
    if(bounce)bob=-Math.abs(bob);
    const cx=56+(walk?Math.trunc(Math.sin(now/700)*S(5)):0),hy=22+Math.trunc(bob);
    const hh=hy-Math.trunc(bob*.16),leg=walk?Math.trunc(Math.sin(now%400/400*2*Math.PI)*S(3)):0;
    const black=0;
    for(const side of [-1,1]){
      const lx=cx+(side<0?-S(10):S(2)),foot=cx+(side<0?-S(13):S(1));
      const shift=side<0?leg:-leg;
      p.fill(lx-1,hy+S(40)-1,S(8)+2,S(10)+shift+2,black);
      p.fill(lx,hy+S(40),S(8),S(10)+shift,C.furMain);
      p.round(foot-1,hy+S(49)+shift-1,S(12)+2,S(6)+2,2,black);
      p.round(foot,hy+S(49)+shift,S(12),S(6),2,C.furLight);
    }
    p.round(cx-S(12),hy+S(22),S(24),S(20),S(5),black);
    p.round(cx-S(11),hy+S(23),S(22),S(18),S(5),C.furMain);
    function wedge(x0,y0,x1,y1,w,col){
      const dx=x1-x0,dy=y1-y0,len=Math.hypot(dx,dy)||1,r=Math.trunc(w/2);
      const nx=-dy/len*w/2,ny=dx/len*w/2;
      p.tri(Math.trunc(x0+nx),Math.trunc(y0+ny),Math.trunc(x0-nx),Math.trunc(y0-ny),Math.trunc(x1+nx),Math.trunc(y1+ny),col);
      p.tri(Math.trunc(x1+nx),Math.trunc(y1+ny),Math.trunc(x1-nx),Math.trunc(y1-ny),Math.trunc(x0-nx),Math.trunc(y0-ny),col);
      p.circle(x0,y0,r,col);p.circle(x1,y1,r,col);
    }
    const limb=(x0,y0,x1,y1)=>{wedge(x0,y0,x1,y1,S(7)+2,black);wedge(x0,y0,x1,y1,S(7),C.furLight);};
    if(wave){
      const a=-1+Math.sin(now%400/400*2*Math.PI)*.5;
      limb(cx+S(11),hy+S(28),cx+S(13)+Math.trunc(Math.cos(a)*18*K),
        hy+S(28)+Math.trunc(Math.sin(a)*18*K));
      limb(cx-S(14),hy+S(24),cx-S(14),hy+S(44));
    }else if(onAir){
      const sh=Math.trunc(Math.sin(now%140/140*2*Math.PI)*S(2));
      limb(cx-S(11),hy+S(26),cx-S(23)+sh,hy+S(10));
      limb(cx+S(11),hy+S(26),cx+S(23)+sh,hy+S(10));
    }else if(suite){
      const dx=Math.trunc(Math.sin(now%500/500*2*Math.PI)*S(11));
      limb(cx-S(14),hy+S(20),cx-S(14)+dx,hy+S(44));
      limb(cx+S(14),hy+S(20),cx+S(14)+dx,hy+S(44));
    }else{
      const sway=Math.trunc(Math.sin(now%1800/1800*2*Math.PI)*S(3));
      for(const [x,shift] of [[cx-S(18),walk?leg:sway],[cx+S(10),walk?-leg:-sway]]){
        p.round(x-1,hy+S(22)+shift-1,S(8)+2,S(22)+2,S(3),black);
        p.round(x,hy+S(22)+shift,S(8),S(22),S(3),C.furLight);
      }
    }
    p.round(cx-S(16),hh-1,S(32),S(26),S(8),black);
    p.round(cx-S(15),hh,S(30),S(24),S(7),C.furLight);
    p.round(cx-S(12),hh+S(2),S(24),S(19),S(5),C.furMain);
    p.round(cx-S(9),hh+S(7),S(18),S(11),S(4),C.skin);
    for(const a of [[-7,2,-4,-5,-1,2],[-2,2,6,-7,6,2],[-13,3,-9,-4,-5,3],[5,3,9,-4,13,3]])
      p.tri(cx+S(a[0]),hh+S(a[1]),cx+S(a[2]),hh+S(a[3]),cx+S(a[4]),hh+S(a[5]),C.furLight);
    for(const side of [-1,1]){
      p.circle(cx+side*S(15),hh+S(13),S(3),C.furMain);
      p.circle(cx+side*S(15),hh+S(13),S(1),C.skinDark);
      p.circle(cx+side*S(8),hh+S(15),S(2),C.vaporPink);
    }
    if(onAir){
      for(const side of [-1,1]){
        p.ellipse(cx+side*S(5),hh+S(9),S(3),S(4),C.white);
        p.circle(cx+side*S(5),hh+S(9),S(1),black);
      }
      p.ellipse(cx,hh+S(18),S(4),S(5),black);ctx.restore();return;
    }
    if(sleep){
      p.line(cx-S(9),hh+S(9),cx-S(2),hh+S(11),C.furLight);
      p.line(cx+S(2),hh+S(11),cx+S(9),hh+S(9),C.furLight);
      p.circle(cx,hh+S(18),S(2),black);ctx.restore();return;
    }
    const slot=Math.floor(now/2600),hashed=(()=>{let h=Math.imul(slot,2654435761)>>>0;
      h^=h>>>15;h=Math.imul(h,2246822519)>>>0;h^=h>>>13;return h>>>0;})();
    const t0=slot*2600+300+hashed%1700,kind=(hashed>>>20)&7;
    const blink=kind===0?now>=t0&&now<t0+440:kind===1?
      ((now>=t0&&now<t0+140)||(now>=t0+230&&now<t0+370)):
      (now>=t0&&now<t0+140);
    const tint=0x0113,lensL=blink||wink?black:tint,lensR=blink?black:tint;
    p.round(cx-S(12),hh+S(6),S(10),S(7),2,black);
    p.round(cx+S(2),hh+S(6),S(10),S(7),2,black);
    p.fill(cx-S(12)+S(10)-1,hh+S(8),cx+S(2)-(cx-S(12)+S(10)-1)+1,S(2),black);
    p.round(cx-S(11),hh+S(7),S(8),S(5),1,lensL);
    p.round(cx+S(3),hh+S(7),S(8),S(5),1,lensR);
    const gx=Math.trunc(now%2400/2400*6);
    if(lensL!==black)p.fill(cx-S(11)+S(1+gx),hh+S(7),1,S(4),C.white);
    if(lensR!==black)p.fill(cx+S(3)+S(1+gx),hh+S(7),1,S(4),C.white);
    p.round(cx-S(8),hh+S(16),S(16),S(7),S(3),black);
    p.fill(cx-S(6),hh+S(17),S(12),S(2),C.white);
    p.fill(cx-S(6),hh+S(19),S(12),S(3),C.pink);
    ctx.restore();
  }
  function deviceName(c){
    if(!c)return 'NO PROFILE';
    const id=c.id;
    if(/^OUI-|^RID-DJI-|^RID-WIFI-DJI-/.test(id))
      return String(c.description||'').split(' | ')[0];
    if(/^RID-AUTEL-/.test(id))return id.includes('EVOMAX')?'Autel EVO Max':
      id.includes('EVOLITE')?'Autel EVO Lite':'Autel EVO II';
    if(/^RID-PARROT-|^RID-WIFI-PARROT$/.test(id))
      return id.includes('ANAFIUSA')?'Parrot ANAFI USA':'Parrot ANAFI';
    if(/^RID-|^BLE-DRONE$/.test(id))return 'OpenDroneID';
    if(/^BLE-HACKER-|^BLE-NEG-FLIP-MFG$/.test(id))return 'Flipper Zero';
    if(/^WIFI-HACKER-PAGER|^WIFI-NEG-PAGER/.test(id))return 'Wi-Fi Pineapple Pager';
    if(id==='WIFI-HACKER-PINE')return 'Wi-Fi Pineapple';
    if(id==='WIFI-HACKER-PWN')return 'ESP Deauther';
    if(/^BLE-FLOCK-|^WIFI-FLOCK|^BLE-NEG-BARE9$/.test(id))return 'Flock Safety';
    if(id==='BLE-NEG-PENGUIN-9')return 'Flock Penguin';
    if(id==='WIFI-NEG-FLOCKHART')return 'Flock Safety control';
    if(/^BLE-META-/.test(id))return id.includes('LUXOTTICA')?'Ray-Ban / Luxottica':
      id.includes('SNAP')?'Snap Spectacles':id.includes('UUID')?'Ray-Ban Meta':'Meta';
    if(/^BLE-RAVEN|^BLE-NEG-RAVEN/.test(id))return 'Raven';
    if(/^BLE-AIRTAG/.test(id))return 'Apple AirTag';
    if(/^BLE-SAMSUNG/.test(id))return 'Samsung SmartTag';
    if(/^BLE-TILE/.test(id))return 'Tile Tracker';
    if(/^BLE-IBEACON|^BLE-NEG-IBEACON/.test(id))return 'Apple iBeacon';
    if(/^BLE-AXON|^WIFI-AXON|^WIFI-NEG-AXON/.test(id))return 'Axon Body Camera';
    if(/^BLE-SKIM/.test(id))return c.text||'Bluetooth Serial Module';
    if(/^BLE-NEG-DRONE-UUID$/.test(id))return 'OpenDroneID control';
    if(/^BLE-NEG-META-ADJ$/.test(id))return 'Ray-Ban Meta control';
    if(/^BLE-NEG-SNAP-ADJ$/.test(id))return 'Snap Spectacles control';
    if(/^BLE-NEG/.test(id))return 'Generic BLE device';
    if(/^WIFI-NEG/.test(id))return 'Generic Wi-Fi AP';
    return 'UNMAPPED DEVICE';
  }
  function menuRow(p,y,name,sub,selected=false){
    p.fill(4,y+1,310,34,selected?0x2825:C.bg);
    p.outline(4,y+1,310,34,selected?C.pink:C.purple);
    p.text(15,y+4,name,selected?C.white:C.cyan,2,280);
    if(sub)p.text(15,y+23,sub,C.muted,1,282);
  }
  function render(ctx,state,lib,now){
    if(!ctx||!state)return false;
    const p=P(ctx),catalog=lib||prepare([]);
    if(ctx.canvas){ctx.canvas.width=W;ctx.canvas.height=H;}
    ctx.imageSmoothingEnabled=false;
    p.fill(0,0,W,H,C.bg);
    const screen=state.calibrated?state.screen:'CALIBRATION';
    if(screen==='CALIBRATION'){
      title(p);p.text(28,35,'SQUACHEMIT TOUCH SETUP',C.cyan,2);
      menuRow(p,87,'TAP SCREEN TO CALIBRATE','FOLLOW THE 5 CROSSHAIRS');
      p.text(12,179,'NO RF ON BOOT  /  115200 BAUD',C.green);
      return true;
    }
    if(screen==='COLORS'){
      title(p);p.text(24,25,'ST7798 RGB565 TEST',C.cyan,2);
      const colors=['#ff0000','#00ff00','#0000ff','#ffffff','#000000'];
      const names=['RED','GREEN','BLUE','WHITE','BLACK'];
      for(let i=0;i<5;i++){const x=7+62*i;p.fill(x,62,56,84,colors[i]);
        p.outline(x,62,56,84,C.purple);p.text(x,157,names[i],C.white);}
      p.text(11,193,'TAP TO EXIT / 7S AUTO',C.vaporPink);
      return true;
    }
    if(screen==='HOME'){
      const active=state.active!=='IDLE'&&state.active!=='NONE';
      const chosen=catalog.byId.get(active?state.active:state.starting!=='NONE'?state.starting:state.selected);
      backdrop(p);
      squachy(ctx,p,now,active,state.suite,state.petMs>0);
      title(p);
      p.round(7,20,306,44,4,C.bg);p.ring(7,20,306,44,4,C.vaporPink);
      p.ring(9,22,302,40,3,C.purple);
      const error=state.error!=='NONE',starting=state.starting!=='NONE';
      const phrase=error?'WIFI START FAILED':starting?'STARTING WIFI TRANSMISSION':
        active?'RF LAB TRANSMITTING':state.suite?'TEST SUITE RUNNING':'READY TO TEST. PICK A PROFILE.';
      p.text(16,25,phrase,error?C.pink:starting?C.yellow:C.white,2,285);
      p.text(16,47,error?state.error:deviceName(chosen),error?C.yellow:
        starting?C.yellow:active?C.pink:C.cyan,1,288);
      p.fill(0,183,320,29,C.bg);p.fill(6,182,308,1,C.purple);
      p.text(12,186,active?'ON AIR':starting?'STARTING':state.suite?'SUITE':'LAB READY',
        active?C.pink:starting?C.yellow:C.green);
      p.outline(93,183,137,15,C.purple);
      p.text(99,186,'TX TIME',C.cyan);p.text(150,186,state.duration+'s [SET]',C.yellow);
      p.fill(236,183,84,15,C.bg);
      p.text(238,186,active?Math.ceil(state.remaining/1000)+'s LEFT':
        catalog.count+' CASES',active?C.pink:C.vaporPink);
      p.fill(6,198,308,1,C.purple);p.text(12,201,'PROFILE',C.vaporPink);
      p.text(66,200,deviceName(catalog.byId.get(state.selected)),C.white,1,164);
      footer(p,'TESTS',active?'STOP':'RUN','SUITE');
      return true;
    }
    title(p);
    if(screen==='CATEGORIES'){
      p.text(8,21,'TEST LIBRARY / DEVICE TYPE',C.vaporPink,2);
      for(let row=0;row<4;row++){
        const index=state.categoryPage*4+row;
        if(index>=CATEGORIES.length)break;
        menuRow(p,44+row*39,CATEGORIES[index],catalog.groups[index].length+' TEST CASES');
      }
      p.text(12,199,'PAGE '+(state.categoryPage+1)+'/'+Math.ceil(CATEGORIES.length/4),C.muted);
      footer(p,'BACK','< PREV','NEXT >');
      return true;
    }
    if(screen==='LIBRARY'){
      const list=catalog.groups[state.category]||[];
      p.text(8,23,CATEGORIES[state.category]+'  '+list.length+' CASES',C.vaporPink,2,301);
      for(let row=0;row<4;row++){
        const i=state.page*4+row,item=list[i];
        if(!item){menuRow(p,48+row*36,'','');continue;}
        const hint=item.id.startsWith('OUI-')?'NO REMOTE ID | '+item.id:
          item.id+'  |  '+item.expected;
        menuRow(p,48+row*36,String(i+1).padStart(2,'0')+'  '+deviceName(item),
          hint,item.id===state.selected);
      }
      p.text(12,198,'PAGE '+(state.page+1)+'/'+Math.max(1,Math.ceil(list.length/4)),C.muted);
      if(state.active!=='IDLE')p.text(233,198,'ON AIR',C.pink);
      footer(p,'BACK','< PREV',state.active!=='IDLE'?'STOP':'NEXT >');
      return true;
    }
    if(screen==='DURATION'){
      p.text(20,27,'TRANSMIT TIME',C.vaporPink,2);
      p.text(22,57,'FOR EACH RUN AND SUITE CASE',C.cyan);
      button(p,12,85,83,54,'- 1 SEC');button(p,225,85,83,54,'+ 1 SEC');
      p.fill(96,82,128,65,C.bg);p.text(145,95,state.duration,C.white,2);
      p.text(139,126,'SECONDS',C.muted);
      p.text(21,164,'AUTO STOP / 3 TO 30 SECONDS',C.green);
      p.text(26,191,'SAVED FOR THE NEXT TEST',C.muted);
      footer(p,'BACK','RESET 12','DONE');
      return true;
    }
    return false;
  }
  return {parse,prepare,render,categoryFor,deviceName,hex565,CATEGORIES,W,H};
});
