'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const mirrorCodec=require('../public/mirror.js');
const native=require('../public/native-emitter.js');

test('Windows rail renders authentic USB frame and translates canvas pointer to MIRROR TOUCH',async()=>{
  const elements=new Map(),windowEvents=new Map(),timers=new Map();
  const commands=[];
  let nextTimer=1;
  function element(id){
    if(elements.has(id))return elements.get(id);
    const e={id,hidden:false,value:'',textContent:'',style:{},listeners:{},
      classList:{add(){},toggle(){}},
      addEventListener(event,callback){this.listeners[event]=callback;},
      appendChild(){},replaceChildren(){},
      setAttribute(){},pause(){},play(){return Promise.resolve();},
      setPointerCapture(){},
      getBoundingClientRect(){return {left:0,top:0,width:this.width,height:this.height};},
      getContext(){
        const el=this;
        return {canvas:el,putImageData:(image)=>{el.lastImage=image;},
          fillRect(){el.drawCount=(el.drawCount||0)+1;},beginPath(){},
          moveTo(){},lineTo(){},stroke(){},fill(){},arc(){},rect(){},
          roundRect(){},ellipse(){},closePath(){},save(){},restore(){},
          translate(){},clip(){},fillText(){},measureText(x){return {width:String(x).length*6};}
        };
      },
      querySelectorAll(){return [];},
      width:320,height:240
    };
    Object.defineProperty(e,'innerHTML',{set(value){
      this._html=value;
      for(const m of value.matchAll(/id="([^"]+)"/g))element(m[1]);
      // The real browser honors the canvas "hidden" attribute. The
      // minimal DOM fake must honor it too before any USB frame arrives.
      for(const m of value.matchAll(/<canvas[^>]*id="([^"]+)"[^>]*>/g))
        element(m[1]).hidden=/\shidden(?:\s|>)/.test(m[0]);
    },get(){return this._html||'';}});
    elements.set(id,e);
    return e;
  }
  const main=element('main');
  main.firstChild=null;
  const state={emitterConnected:true,receiverConnected:true,
    emitterBaud:2000000,batchActive:false};
  const api={
    getState:()=>state,
    send:(role,cmd)=>{commands.push([role,cmd]);return Promise.resolve(true);},
    connect:()=>Promise.resolve(),disconnect:()=>Promise.resolve(),
    setEmitterBaud:()=>true,runSelected:()=>Promise.resolve(),exportCsv:()=>{}
  };
  const win={
    SQEWorkbenchDesktop:api,SQEMirrorCodec:mirrorCodec,SQECydNative:native,
    addEventListener:(name,callback)=>windowEvents.set(name,callback)
  };
  const context={
    window:win,
    document:{querySelector:()=>main,createElement:tag=>element('created-'+tag+'-'+elements.size),
      getElementById:element,activeElement:null},
    location:{search:'?desktop=1'},
    URLSearchParams,Date,ImageData:class ImageData{
      constructor(pixels,width,height){this.pixels=pixels;this.width=width;this.height=height;}
    },
    navigator:{},console,
    setTimeout:(fn,ms)=>{const id=nextTimer++;timers.set(id,{fn,ms});return id;},
    clearTimeout:id=>timers.delete(id),
    setInterval:(fn,ms)=>{const id=nextTimer++;timers.set(id,{fn,ms});return id;},
    clearInterval:id=>timers.delete(id)
  };
  const source=fs.readFileSync(path.resolve(__dirname,'../public/controller.js'),'utf8');
  vm.runInNewContext(source,context,{filename:'controller.js'});
  const rail=elements.get('created-aside-2');
  assert.ok(rail,'real device desk must be built');
  assert.match(rail.innerHTML,/NO LIVE LCD FRAME/);
  assert.doesNotMatch(rail.innerHTML,/OPTIONAL PHYSICAL CAMERA|dc-camera|dc-video-|dc-source-|Show video/);
  assert.ok(!elements.has('dc-source-emitter')&&!elements.has('dc-video-receiver'));
  assert.ok(elements.get('dc-canvas-receiver').hidden,'no fake screen before USB pixels');
  const click=(action,role)=>{
    rail.listeners.click({target:{closest:()=>({dataset:{action,role},disabled:false})}});
  };
  const drain=async()=>{for(let i=0;i<12;i++)await Promise.resolve();};
  click('mirror-on','receiver');
  await drain();
  assert.ok(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR ON'));
  const receive=line=>windowEvents.get('sqe:mirror')({detail:{role:'receiver',line}});
  assert.equal(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR FRAME'),false,
    'frame must wait for a real MIRROR READY acknowledgment');
  receive('MIRROR READY RGB332 320x240');
  await drain();
  assert.ok(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR FRAME'));
  receive('MIRROR BEGIN 9 2 1 RGB332');
  receive('MIRROR ROW 9 0 '+Buffer.from([0xe0,0x03]).toString('base64'));
  receive('MIRROR END 9');
  const canvas=elements.get('dc-canvas-receiver');
  assert.equal(canvas.hidden,false);
  assert.equal(canvas.width,2);
  assert.equal(canvas.height,1);
  assert.deepEqual(Array.from(canvas.lastImage.pixels),[
    255,0,0,255,0,0,255,255
  ]);
  assert.equal(elements.get('dc-placeholder-receiver').hidden,true);
  canvas.listeners.pointerdown({pointerId:4,clientX:1,clientY:0,
    preventDefault(){}});
  await drain();
  assert.ok(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR TOUCH 1 0 DOWN'));
  canvas.listeners.pointerup({pointerId:4,clientX:1,clientY:0,
    preventDefault(){}});
  for(const [id,timer] of Array.from(timers)){
    if(timer.ms<=150){timers.delete(id);timer.fn();}
  }
  await drain();
  assert.ok(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR TOUCH 1 0 UP'));
  click('mirror-off','receiver');
  await drain();
  assert.ok(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR OFF'));
  assert.equal(canvas.hidden,true,'stopped mirror cannot present stale pixels as live');

  // The workbench automatically stops the receiver mirror during a signal
  // test. The UI must hide the old pixels without issuing a duplicate OFF.
  click('mirror-on','receiver');
  await drain();
  receive('MIRROR READY RGB332 320x240');
  await drain();
  const oldOff=commands.filter(([r,c])=>r==='receiver'&&c==='MIRROR OFF').length;
  windowEvents.get('sqe:mirror-paused')({detail:{role:'receiver',
    reason:'Paused for exact-MAC LOG capture'}});
  assert.equal(canvas.hidden,true,'receiver auto-pause hides stale LCD frame');
  assert.equal(elements.get('dc-placeholder-receiver').hidden,false);
  assert.match(elements.get('dc-frame-receiver').textContent,/exact-MAC LOG/);
  assert.equal(commands.filter(([r,c])=>r==='receiver'&&c==='MIRROR OFF').length,oldOff,
    'detection runner, not UI, owns the already-sent MIRROR OFF');

  // Independent SquachEmit panel is PC-drawn. No TFT screenshot command.
  click('mirror-on','emitter');
  await drain();
  assert.ok(commands.some(([r,c])=>r==='emitter'&&c==='UI ON'));
  assert.ok(!commands.some(([r,c])=>r==='emitter'&&c==='MIRROR FRAME'));
  const rx=line=>windowEvents.get('sqe:native-ui')({detail:{role:'emitter',line}});
  rx('UI READY 1 320 240 MODEL');
  await drain();
  assert.ok(commands.some(([r,c])=>r==='emitter'&&c==='UI GET'));
  const syncState='UI STATE 1 HOME 2 0 0 BLE-FLOCK-MFG 12 IDLE 0 0 1 NONE NONE 0 22000';
  windowEvents.get('sqe:catalog')({detail:{cases:[
    {id:'BLE-FLOCK-MFG',expected:'FLOCK',description:'Flock supplier',carrier:'BLE'}
  ]}});
  rx(syncState);
  await drain();
  const ec=elements.get('dc-canvas-emitter');
  assert.equal(ec.hidden,false,'locally rendered device state is visible');
  assert.equal(ec.width,320);
  assert.equal(ec.height,240);
  assert.ok(ec.drawCount>40,'procedural graphics drawn by the desktop Canvas');
  assert.match(elements.get('dc-frame-emitter').textContent,/PC-DRAWN UI/);
  // Physical board state changes (no mouse interaction) appear immediately.
  rx('UI STATE 1 DURATION 2 0 0 BLE-FLOCK-MFG 18 IDLE 0 0 1 NONE NONE 0 22300');
  await drain();
  assert.match(elements.get('dc-frame-emitter').textContent,/DURATION/);
  ec.listeners.pointerdown({pointerId:7,clientX:60,clientY:110,preventDefault(){}});
  await drain();
  assert.ok(commands.some(([r,c])=>r==='emitter'&&c==='UI TOUCH 60 110 DOWN'));
  click('mirror-off','emitter');
  await drain();
  const down=commands.findIndex(([r,c])=>r==='emitter'&&c==='UI TOUCH 60 110 DOWN');
  const up=commands.findIndex(([r,c])=>r==='emitter'&&c==='UI TOUCH 60 110 UP');
  const off=commands.findIndex(([r,c])=>r==='emitter'&&c==='UI OFF');
  assert.ok(down>=0&&up>down&&off>up,'STOP must release native pointer before disconnect');
  assert.equal(ec.hidden,true);
  assert.equal(commands.some(([r,c])=>r==='emitter'&&c==='MIRROR ON'),false,
    'no TFT pixel-capture command may be sent to SquachEmit');
});
