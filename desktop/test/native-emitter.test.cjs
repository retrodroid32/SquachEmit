'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const native=require('../public/native-emitter.js');
const state='UI STATE 1 HOME 2 0 0 BLE-FLOCK-MFG 12 IDLE 0 0 1 NONE NONE 0 22000';
test('R7 only accepts bounded, well-formed authoritative UI state records',()=>{
  const s=native.parse(state);
  assert.ok(s);assert.equal(s.screen,'HOME');assert.equal(s.category,2);
  assert.equal(s.duration,12);assert.equal(s.selected,'BLE-FLOCK-MFG');
  assert.equal(s.active,'IDLE');assert.equal(s.calibrated,true);
  assert.equal(s.uptime,22000);
  assert.equal(native.parse('MIRROR ROW 1 0 AAAA'),null);
  assert.equal(native.parse('UI STATE 1 HOME 88 0 0 BLE-FLOCK-MFG 12 IDLE 0 0 1 NONE NONE 0 22000'),null);
  assert.equal(native.parse('UI STATE 1 HOME 2 0 0 BLE-FLOCK-MFG 200 IDLE 0 0 1 NONE NONE 0 22000'),null);
  assert.equal(native.parse('UI STATE 1 HOME 2 0 0 BLE-FLOCK-MFG 12 IDLE 0 0 1 NONE NONE 0 22000 extra'),null);
  assert.equal(native.parse('UI STATE 1 OTHER 2 0 0 BLE-FLOCK-MFG 12 IDLE 0 0 1 NONE NONE 0 22000'),null);
});
test('R7 category assignment and menu order follow authoritative CYD32 category rules',()=>{
  const cases=JSON.parse(fs.readFileSync(path.resolve(__dirname,'../../tests/cases.json'),'utf8'));
  const lib=native.prepare(cases);
  assert.equal(lib.count,172);
  assert.equal(lib.groups.length,10);
  assert.equal(lib.groups.reduce((n,rows)=>n+rows.length,0),172);
  assert.equal(native.categoryFor({id:'RID-DJI-MINI4PRO'}),1);
  assert.equal(native.categoryFor({id:'BLE-FLOCK-MFG'}),2);
  assert.equal(native.categoryFor({id:'BLE-AXON-NAME'}),0);
  assert.equal(native.categoryFor({id:'BLE-NEG-DRONE-UUID'}),1);
  assert.equal(native.categoryFor({id:'BLE-AIRTAG'}),9);
  for(const group of lib.groups){
    assert.deepEqual(group.map(x=>x.id),group.map(x=>x.id).sort(),
      'menu cases must be displayed alphabetically by ID');
  }
});
test('R7 all four screen modes draw on the PC without any screenshot or canvas image data',()=>{
  const operations=[];
  const ctx={
    canvas:{width:320,height:240},
    beginPath(){},moveTo(){},lineTo(){},stroke(){},fill(){},closePath(){},
    roundRect(){},rect(){},arc(){},ellipse(){},save(){},restore(){},
    translate(){},clip(){},
    fillRect(x,y,w,h){operations.push([x,y,w,h]);},
    fillText(){},measureText(str){return {width:str.length*6};}
  };
  const cases=JSON.parse(fs.readFileSync(path.resolve(__dirname,'../../tests/cases.json'),'utf8'));
  const lib=native.prepare(cases);
  const base=native.parse(state);
  for(const screen of ['HOME','CATEGORIES','LIBRARY','DURATION','CALIBRATION','COLORS']){
    const model={...base,screen,calibrated:screen!=='CALIBRATION'};
    operations.length=0;
    assert.equal(native.render(ctx,model,lib,23000),true);
    assert.ok(operations.length>10,screen+' needs actual desktop draw commands');
  }
  assert.equal(typeof ctx.putImageData,'undefined',
    'native software-rendered UI never requires LCD pixel data');
});
test('Windows bridge does not request TFT readback for SquachEmit',()=>{
  const read=rel=>fs.readFileSync(path.resolve(__dirname,'../..',rel),'utf8');
  const fw=read('src/cyd32_ui.cpp');
  const main=read('src/main.cpp');
  const controller=read('desktop/public/controller.js');
  const wb=read('workbench/app.js');
  assert.doesNotMatch(fw,/\blcd\.readRect\s*\(/);
  assert.doesNotMatch(main,/cyd32UiMirrorPump\s*\(/);
  assert.match(main,/UI READY 1 320 240 MODEL/);
  assert.match(main,/cyd32UiStreamState\(/);
  assert.match(wb,/sqe:|desktopEvent\('native-ui'/);
  assert.match(controller,/native\.render\(/);
  assert.match(controller,/role==='emitter'\?'UI TOUCH '/);
  assert.match(controller,/api\.send\('emitter','UI ON'\)/);
  assert.match(controller,/api\.send\('emitter','UI GET'\)/);
});
