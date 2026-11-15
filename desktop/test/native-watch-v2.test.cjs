'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
const read=p=>fs.readFileSync(path.join(root,p),'utf8');
const v2=require('../public/native-watch-v2.js');
const ctrl=read('desktop/public/controller.js'),wb=read('workbench/app.js');
const state=(screen=1,kind=1,privacy=0)=>
  'UI STATE 2 11 '+screen+' 320 240 1 4100 '+privacy+' '+kind;
test('v2 state requires authoritative screen, geometry, rotation and kind',()=>{
  assert.equal(v2.state(state()).screen,1);
  assert.equal(v2.state(state(3,2,1)).privacy,true);
  assert.equal(v2.state(state(4,0)).kind,0);
  for(const bad of [state(3,1),state(1,2),state(42,0),
    'UI STATE 1 1 320 240 1 4100',
    'UI STATE 2 1 1 0 240 1 20 0 1'])assert.equal(v2.state(bad),null);
});
test('partial MAIN counters are ordered and grounded in the firmware groups',()=>{
  const model=v2.create();
  assert.equal(model.feed('UI CAPS 2 SQUACHWATCH STATE COUNTS RECENT_LOG TOUCH PIXEL_FALLBACK PARTIAL').kind,'caps');
  model.feed(state());
  const groups=v2.groups.slice(0,11).map(([id])=>id+':1:4');
  assert.equal(model.feed('UI COUNTS 2 11 '+groups.join(' ')).kind,'counts');
  assert.equal(model.snapshot().counts.length,11);
  assert.equal(model.snapshot().counts[5].label,'TRACKER');
  assert.equal(model.feed('UI COUNTS 2 11 '+groups.reverse().join(' ')).kind,'ignored');
  model.feed(state(4,0));assert.equal(model.snapshot().counts,null);
});
test('native DEVICES list requires matching sequence and exact row count',()=>{
  const m=v2.create();
  m.feed(state(3,2));
  assert.equal(m.feed('UI LIST 2 7 255 0 2 2').kind,'list-start');
  assert.equal(m.feed('UI ROW 2 7 0 7 1 -63 1 12345 0 AA:BB:CC:DD:EE:FF 44524F4E45').kind,'row');
  assert.equal(m.feed('UI ROW 2 8 1 7 0 -90 30 13456 0 AA:BB:CC:DD:EE:FF -').kind,'ignored');
  assert.equal(m.feed('UI END 2 7').kind,'error');
  assert.equal(m.snapshot().rows.length,0);
  assert.equal(m.feed('UI LIST 2 8 255 0 1 1').kind,'list-start');
  assert.equal(m.feed('UI ROW 2 8 0 7 1 -63 1 12345 0 AA:BB:CC:XX:XX:XX -').kind,'row');
  assert.equal(m.feed('UI END 2 8').kind,'list');
  assert.equal(m.snapshot().rows[0].mac,'AA:BB:CC:XX:XX:XX');
});
test('privacy change revokes incomplete LIST and leaves no stale rows',()=>{
  const m=v2.create();m.feed(state(3,2,0));
  m.feed('UI LIST 2 9 255 0 1 1');m.feed(state(3,2,1));
  assert.equal(m.feed('UI END 2 9').kind,'error');
  assert.deepEqual(m.snapshot().rows,[]);
});
test('WINDOWS receiver start negotiates CAPS2 and routes UI TOUCH',()=>{
  assert.match(ctrl,/UI HELLO/);
  assert.match(ctrl,/UI READY 2/); // asserted via v2 parser or explicit UI v2 text
  assert.match(ctrl,/UI LIST 0 6/);
  assert.match(ctrl,/UI COUNTS/);
  assert.match(ctrl,/receiverUi\.mode==='native'\?'UI TOUCH ':'MIRROR TOUCH '/);
  assert.match(ctrl,/kind===0/);
  assert.match(ctrl,/fallbackWatch/);
  assert.match(ctrl,/Date\.now\(\)-receiverUi\.lastAt>3000/);
  assert.match(wb,/receiverNative=false/);
  assert.match(wb,/UI \(\?:HELLO\|ON\|OFF\|GET\|STATUS\|COUNTS\|LIST/);
  assert.match(wb,/wasNative\?'UI OFF':'MIRROR OFF'/);
  assert.match(wb,/desktopEvent\('receiver-ui'/);
});
test('source files parse without syntax errors',()=>{
  const vm=require('node:vm');
  assert.doesNotThrow(()=>new vm.Script(ctrl));
  assert.doesNotThrow(()=>new vm.Script(wb));
});
