'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.resolve(__dirname,'../..');
const ctrl=fs.readFileSync(path.join(root,'desktop/public/controller.js'),'utf8');

test('both devices share identical primary live-view control labels',()=>{
  for(const label of ['Start live view','Refresh view','Stop live view'])
    assert.ok(ctrl.includes('>'+label+'</button>'),label);
  assert.doesNotMatch(ctrl,/emitter\?'Sync device UI':'Mirror live'/);
  assert.doesNotMatch(ctrl,/emitter\?'Refresh state':'Get frame'/);
});
test('USB connect never loops unsupported UI ON on older firmware',()=>{
  assert.match(ctrl,/Never spam unsupported UI ON after USB connect/);
  assert.doesNotMatch(ctrl,/if\(role==='emitter'&&isConnected\(role,state\)\)await startMirror\(role\)/);
  assert.match(ctrl,/if\(mirrors\[role\]\.active\)/);
});
test('unknown command gives explicit mismatch and turns off pending live UI',()=>{
  assert.match(ctrl,/ERROR,unknown_command/);
  assert.match(ctrl,/resetRole\('emitter'\)/);
  assert.match(ctrl,/firmware rejected UI ON/);
  assert.match(ctrl,/older public firmware|Older public firmware/);
  assert.match(ctrl,/lacks the required USB pixel-mirror command/);
  assert.match(ctrl,/void api\.send\('receiver','MIRROR OFF'\)/);
});
test('no fabricated SquachWatch pixels and serial LOG path preserved',()=>{
  assert.match(ctrl,/Full screen remains available via authentic pixel mirror/);
  assert.match(ctrl,/data-action="receiver-ui-on"/);
  assert.match(ctrl,/MIRROR FRAME/);
  assert.match(ctrl,/sqe:mirror-paused/);
});
