'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');

const root = path.resolve(__dirname, '..');
const ids = ['board','build','buildDetails','confirm','notice','channelBadge','enterLab',
  'exitLab','installArea','installLocked','installButton','downloadBin','serialState',
  'connect','disconnect','serialLog','command','commandForm','send','clear',
  'saveLog','baudRate','customBaud','baudHint'];
class Element {
  constructor(id) {
    this.id=id;this.value='';this.textContent='';this.children=[];this.handlers={};
    this.dataset={};this.style={};this.disabled=false;this.hidden=false;this.checked=false;
    this.classList={toggle(){}};
  }
  addEventListener(event,fn){this.handlers[event]=fn;}
  replaceChildren(...nodes){this.children=nodes;this.value=nodes.length?nodes[0].value:'';}
  appendChild(node){this.children.push(node);}
  setAttribute(name,value){this[name]=value;}
}
const elements=Object.fromEntries(ids.map(id=>[id,new Element(id)]));
elements.baudRate.value='115200';
elements.customBaud.value='115200';
const document={
  getElementById(id){return elements[id];},
  createElement(){return new Element('option');},
  querySelectorAll(){return [];}
};
const opened=[];
let picked=0, closed=0;
const fakePort={
  async open(options){opened.push(options.baudRate);},
  async close(){closed++;},
  readable:{getReader(){return {async read(){return {done:true};},releaseLock(){},async cancel(){}};}},
  writable:{getWriter(){return {async write(){},releaseLock(){}};}}
};
const context={
  document, location:{search:'?lab=1'},
  navigator:{serial:{async requestPort(){picked++;return fakePort;}}},
  window:{isSecureContext:true,addEventListener(){}},
  URLSearchParams,TextEncoder,TextDecoder,
  fetch:async()=>({ok:true,json:async()=>({builds:[]})}),
  console
};
vm.runInNewContext(fs.readFileSync(path.join(root,'web-installer/app.js'),'utf8'),context,{timeout:2500});
async function run(){
  await new Promise(resolve=>setImmediate(resolve));
  assert.equal(elements.baudRate.value,'115200');
  assert.equal(elements.customBaud.hidden,true);
  // Existing default must remain SquachEmit's 115,200.
  await elements.connect.handlers.click();
  assert.equal(opened[0],115200);
  assert.equal(elements.baudRate.disabled,true);
  assert.ok(elements.serialLog.textContent.includes('115,200 baud'));
  await elements.disconnect.handlers.click();
  assert.equal(elements.baudRate.disabled,false);
  // SquachWatch CYD diagnostics use 2,000,000.
  elements.baudRate.value='2000000';
  elements.baudRate.handlers.change();
  await elements.connect.handlers.click();
  assert.equal(opened[1],2000000);
  assert.ok(elements.serialState.textContent.includes('2,000,000'));
  await elements.disconnect.handlers.click();
  // Custom rate works and locks while connected.
  elements.baudRate.value='custom';
  elements.baudRate.handlers.change();
  assert.equal(elements.customBaud.hidden,false);
  assert.equal(elements.customBaud.disabled,false);
  elements.customBaud.value='74880';
  await elements.connect.handlers.click();
  assert.equal(opened[2],74880);
  assert.equal(elements.customBaud.disabled,true);
  await elements.disconnect.handlers.click();
  assert.equal(elements.customBaud.disabled,false);
  // Invalid input is rejected before the browser asks for a COM port.
  const previouslyPicked=picked;
  const previouslyOpened=opened.length;
  for(const input of ['-1','10','4000001','115200.5','', 'NaN']){
    elements.customBaud.value=input;
    await elements.connect.handlers.click();
  }
  assert.equal(picked,previouslyPicked);
  assert.equal(opened.length,previouslyOpened);
  assert.equal(elements.baudRate.disabled,false);
  assert.ok(elements.serialLog.textContent.includes('Enter a whole-number baud rate'));
  assert.equal(closed,3);
  console.log('Serial baud selector: default/preset/custom/lock/invalid tests PASS');
}
run().catch(e=>{console.error(e);process.exitCode=1;});
