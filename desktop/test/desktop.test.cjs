'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const vm=require('node:vm');
const root=path.resolve(__dirname,'../..');
const read=rel=>fs.readFileSync(path.join(root,rel),'utf8');

test('all desktop JS is syntactically valid',()=>{
  for(const file of ['desktop/main.cjs','desktop/public/controller.js','desktop/public/mirror.js',
    'workbench/app.js','workbench/core.js']){
    assert.doesNotThrow(()=>new vm.Script(read(file),{filename:file}),file);
  }
});
test('Windows application is a standalone NSIS and portable EXE bundle',()=>{
  const p=JSON.parse(read('desktop/package.json'));
  assert.match(p.name,/squachemit/);
  assert.match(p.scripts['dist:win'],/--win nsis portable --x64/);
  assert.equal(p.build.asar,true);
  assert.ok(p.build.extraResources.some(x=>x.from==='../workbench'));
  assert.ok(p.build.extraResources.some(x=>x.to==='workbench/cases.json'));
  assert.ok(p.build.extraResources.some(x=>x.to==='desktop-panel'));
  assert.ok(p.build.files.includes('main.cjs'));
  assert.ok(p.build.files.includes('public/**/*'));
});
test('desktop is local only with restricted renderer and USB chooser',()=>{
  const s=read('desktop/main.cjs');
  for(const required of [
    "server.listen(0,'127.0.0.1'","'select-serial-port'",
    "nodeIntegration:false","contextIsolation:true","sandbox:true",
    "'Content-Security-Policy'","'X-Content-Type-Options'",
    "event.preventDefault()","dialog.showMessageBox",
    "file:path.join(roots.workbench,'index.html')",
    "'/desktop/controller.js'","'/desktop/mirror.js'","'webSecurity'"]){
    if(required==="'webSecurity'")assert.match(s,/webSecurity:true/);
    else assert.ok(s.includes(required),'missing desktop contract: '+required);
  }
  assert.doesNotMatch(s,/disable-web-security|allowRunningInsecureContent:true/);
  assert.doesNotMatch(s,/nodeIntegration:true/);
});
test('all 172 device-first names are compatible with the existing Workbench',()=>{
  const all=JSON.parse(read('tests/cases.json'));
  const core=require('../../workbench/core.js');
  assert.equal(all.length,172);
  for(const c of all){
    const name=core.signalName(c);
    assert.ok(name.includes(' — ')&&name.length>6,c.id);
  }
  assert.match(core.signalName(all.find(x=>x.id==='BLE-HACKER-NAME')),/^Flipper Zero/);
  assert.match(core.signalName(all.find(x=>x.id==='BLE-HACKER-UUID')),/^Flipper Zero/);
});
test('USB LCD frames use real decoded pixels and pointer touchscreen input',()=>{
  const ui=read('desktop/public/controller.js');
  const wb=read('workbench/app.js');
  const main=read('desktop/main.cjs');
  for(const key of ['SQEMirrorCodec','sqe:mirror','dc-canvas-','ImageData(',
    'putImageData(','MIRROR ON','MIRROR FRAME','MIRROR OFF','MIRROR TOUCH ',
    "'DOWN'","'MOVE'","'UP'",'pointerdown','pointermove','pointerup',
    'touchPoint(','startMirror(','stopMirror(','requestMirrorFrame(',
    'getUserMedia','enumerateDevices','videoinput','stopCamera',
    'deviceId:{exact:id}','sqe:serial','sqe:snapshot','sqe:truth'])
    assert.ok(ui.includes(key),'missing real LCD and remote-control feature: '+key);
  for(const command of ['STATUS|IDENTITY|LIST|HELP|COLORS|STOP',
    'RADIO SCAN','SCAN (?:AUTO|ACTIVE|PASSIVE)','CLOCK [01] [0-2] [0-6]',
    'MIRROR (?:ON|OFF|FRAME','BAUD (?:115200|2000000)'])
    assert.ok(wb.includes(command),'missing serial control: '+command);
  assert.match(wb,/if\(batch\.active&&!\//);
  assert.ok(main.includes('/desktop/mirror.js')&&main.indexOf(
    '<script src="/desktop/mirror.js"')<main.indexOf(
    '<script src="/desktop/controller.js"'));
  assert.match(ui,/NO LIVE LCD FRAME/);
  assert.match(ui,/not a recreated UI/);
  assert.doesNotMatch(ui,/LIVE SERIAL TELEMETRY/);
  assert.doesNotMatch(ui,/dc-screenmetric/);
  assert.doesNotMatch(wb,/getUserMedia\(\{audio:true/);
});
test('Windows Actions validates contracts and publishes review EXEs, not releases',()=>{
  const workflow=read('.github/workflows/windows-desktop-workbench.yml');
  assert.match(workflow,/windows-latest/);
  assert.match(workflow,/npm test/);
  assert.match(workflow,/npm run dist:win/);
  assert.match(workflow,/upload-artifact@v4/);
  assert.doesNotMatch(workflow,/gh release create|create-release|publish:\s*always/);
});
