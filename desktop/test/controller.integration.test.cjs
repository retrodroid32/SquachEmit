'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const mirrorCodec=require('../public/mirror.js');

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
      getContext(){return {putImageData:(image)=>{this.lastImage=image;}};},
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
    SQEWorkbenchDesktop:api,SQEMirrorCodec:mirrorCodec,
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
  assert.ok(elements.get('dc-canvas-receiver').hidden,'no fake screen before USB pixels');
  const click=(action,role)=>{
    rail.listeners.click({target:{closest:()=>({dataset:{action,role},disabled:false})}});
  };
  const drain=async()=>{for(let i=0;i<12;i++)await Promise.resolve();};
  click('mirror-on','receiver');
  await drain();
  assert.ok(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR ON'));
  assert.ok(commands.some(([r,c])=>r==='receiver'&&c==='MIRROR FRAME'));
  const receive=line=>windowEvents.get('sqe:mirror')({detail:{role:'receiver',line}});
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

  // Independent CYD3.2 panel: a real RGB565 frame followed by a differential
  // transfer must reproduce the exact full image without any synthetic redraw.
  click('mirror-on','emitter');
  await drain();
  assert.ok(commands.some(([r,c])=>r==='emitter'&&c==='MIRROR FRAME'));
  const rx=line=>windowEvents.get('sqe:mirror')({detail:{role:'emitter',line}});
  rx('MIRROR BEGIN 20 2 2 RGB565');
  rx('MIRROR ROW 20 0 '+Buffer.from([0x00,0xf8,0xe0,0x07]).toString('base64'));
  rx('MIRROR ROW 20 1 '+Buffer.from([0x1f,0x00,0xff,0xff]).toString('base64'));
  rx('MIRROR END 20');
  const ec=elements.get('dc-canvas-emitter');
  assert.equal(ec.hidden,false);
  assert.deepEqual(Array.from(ec.lastImage.pixels),[
    255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255
  ]);
  // Trigger the next short poll, verifying it names the host's accepted seq.
  for(const [id,timer] of Array.from(timers)){
    if(timer.ms===65){timers.delete(id);timer.fn();}
  }
  await drain();
  assert.ok(commands.some(([r,c])=>r==='emitter'&&c==='MIRROR FRAME 20'));
  rx('MIRROR DELTA 21 20 2 2 RGB565');
  rx('MIRROR ROW 21 1 '+Buffer.from([0x00,0x00,0x1f,0x00]).toString('base64'));
  rx('MIRROR END 21');
  assert.deepEqual(Array.from(ec.lastImage.pixels),[
    255,0,0,255,0,255,0,255,0,0,0,0,255,0,0,255,255
  ]);
  // Holding a button must be released before the serial MIRROR OFF.
  ec.listeners.pointerdown({pointerId:7,clientX:0,clientY:0,preventDefault(){}});
  await drain();
  click('mirror-off','emitter');
  await drain();
  const down=commands.findIndex(([r,c])=>r==='emitter'&&c==='MIRROR TOUCH 0 0 DOWN');
  const up=commands.findIndex(([r,c])=>r==='emitter'&&c==='MIRROR TOUCH 0 0 UP');
  const off=commands.findIndex(([r,c])=>r==='emitter'&&c==='MIRROR OFF');
  assert.ok(down>=0&&up>down&&off>up,'stop must release physical touch first');
  assert.equal(ec.hidden,true);
});
