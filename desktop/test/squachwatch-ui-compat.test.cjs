'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const path=require('node:path');
const root=path.resolve(__dirname,'../..');
const ui=fs.readFileSync(path.join(root,'desktop/public/controller.js'),'utf8');
const wb=fs.readFileSync(path.join(root,'workbench/app.js'),'utf8');

test('receiver UI stream is routed independently from genuine LOG evidence',()=>{
  assert.match(wb,/role==='receiver'&&line\.startsWith\('UI '\)/);
  assert.match(wb,/desktopEvent\('receiver-ui'/);
  assert.match(wb,/else onWatchLine\(line\.trim\(\)\)/);
  assert.match(wb,/UI \(\?:ON\|OFF\|GET\|TOUCH/);
  assert.match(wb,/batch\.active&&!\/\^\(\?:RADIO\|MEM\|MIRROR OFF\|UI OFF\)/);
});
test('desktop retains authentic SquachWatch pixel mirror while adding an explicit state probe',()=>{
  assert.match(ui,/data-action="receiver-ui-on"/);
  assert.match(ui,/data-action="receiver-ui-off"/);
  assert.match(ui,/MIRROR FRAME/);
  assert.match(ui,/MIRROR TOUCH /);
  assert.match(ui,/UI READY 1 SQUACHWATCH TOUCH STATE/);
  assert.match(ui,/UI STATE 1/);
  assert.match(ui,/Full screen remains available via authentic pixel mirror/);
  assert.match(ui,/if\(mirrors\.receiver\.active\)await stopMirror\('receiver',true\)/);
  assert.match(ui,/if\(role==='receiver'&&receiverUi\.enabled\)/);
});
test('numeric UI STATE frames are parsed fail closed and never depicted as LCD pixels',()=>{
  const re=/^UI STATE 1 (\d{1,3}) (\d{1,3}) (\d{1,3}) ([0-3]) (\d{1,10})$/;
  assert.deepEqual(re.exec('UI STATE 1 3 320 240 1 1234').slice(1),
    ['3','320','240','1','1234']);
  for(const bad of ['UI STATE 1 3 320 240 9 1234',
    'UI STATE 1 3 320 240 1 1234 EXTRA','UI STATE 1 3 A 240 1 1234'])
    assert.equal(re.exec(bad),null);
  assert.match(ui,/id>=watchScreenNames\.length\|\|w<1\|\|w>480/);
  assert.match(ui,/if\(!receiverUi\.ready\|\|!m\)return/);
  assert.match(ui,/Native UI compatibility shows screen state only/);
  assert.doesNotMatch(ui,/SquachWatch native pixel-free mirror complete/);
});
test('Windows controller remains syntactically valid',()=>{
  assert.doesNotThrow(()=>new vm.Script(ui,{filename:'controller.js'}));
  assert.doesNotThrow(()=>new vm.Script(wb,{filename:'app.js'}));
});
