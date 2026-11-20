'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const root=path.resolve(__dirname,'../..');
const cyd=fs.readFileSync(path.join(root,'src/cyd32_ui.cpp'),'utf8');
const native=fs.readFileSync(path.join(root,'desktop/public/native-emitter.js'),'utf8');
const controller=fs.readFileSync(path.join(root,'desktop/public/controller.js'),'utf8');
const firmware=fs.readFileSync(path.join(root,'src/main.cpp'),'utf8');
test('handheld details and diagnostics are real navigable pages',()=>{
 for(const t of ['Screen::Details','Screen::Diagnostics','touchDetails(','touchDiagnostics(','diagValues()','UI HEALTH 1'])
   assert.ok(cyd.includes(t),t);
});
test('USB returns hardware health values rather than an invented meter',()=>{
 assert.match(firmware,/cmd\.equalsIgnoreCase\("HEALTH"\)/);
 assert.match(firmware,/ESP\.getFreeHeap\(\)/);
 assert.match(firmware,/ESP\.getMaxAllocHeap\(\)/);
 assert.match(cyd,/Serial\.availableForWrite\(\)/);
 assert.doesNotMatch(cyd,/RF POWER.*dBm/);
});
test('Windows native mirror supports new pages and real health data',()=>{
 assert.match(native,/'DIAGNOSTICS'/);
 assert.match(native,/'DETAILS'/);
 assert.match(native,/state\.health/);
 assert.match(controller,/UI HEALTH 1/);
 assert.match(controller,/state\.health=local\.model\.health/);
});
