'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const root=path.resolve(__dirname,'../..');
const read=p=>fs.readFileSync(path.join(root,p),'utf8');
const ui=read('desktop/public/controller.js');
const wb=read('workbench/app.js');
const v2=read('desktop/public/native-watch-v2.js');
test('receiver UI v2 remains separate from independent LOG scoring',()=>{
 assert.match(wb,/role==='receiver'&&line\.startsWith\('UI '\)/);
 assert.match(wb,/desktopEvent\('receiver-ui'/);
 assert.match(wb,/else onWatchLine\(line\.trim\(\)\)/);
 assert.match(wb,/receiverNative=false/);
 assert.match(wb,/const wasNative=receiverNative/);
 assert.match(wb,/sendReceiver\(wasNative\?'UI OFF':'MIRROR OFF'\)/);
});
test('Windows supports CAPS2, READY2, state, real counts, LIST and pixel fallback',()=>{
 assert.match(v2,/UI CAPS 2 SQUACHWATCH STATE COUNTS RECENT_LOG TOUCH PIXEL_FALLBACK PARTIAL/);
 assert.match(v2,/UI READY 2 SQUACHWATCH TOUCH PARTIAL/);
 assert.match(v2,/a\[1\]!=='STATE'/);
 assert.match(v2,/UI COUNTS 2/);
 assert.match(v2,/UI ROW 2/);
 assert.match(ui,/UI HELLO/);
 assert.match(ui,/UI COUNTS/);
 assert.match(ui,/UI LIST 0 6/);
 assert.match(ui,/fallbackWatch/);
 assert.match(ui,/startMirror\('receiver',true\)/);
 assert.match(ui,/MIRROR FRAME/);
 assert.match(ui,/MIRROR TOUCH /);
});
test('receiver v1 metadata-only UI is not presented as complete native v2',()=>{
 assert.doesNotMatch(ui,/UI READY 1 SQUACHWATCH TOUCH STATE/);
 assert.doesNotMatch(ui,/UI STATE 1 \(\\d/);
 assert.match(ui,/PARTIAL/);
 assert.match(ui,/authentic RGB332/);
});
test('all client sources parse',()=>{
 for(const [file,s] of [['controller.js',ui],['app.js',wb],['native-watch-v2.js',v2]])
   assert.doesNotThrow(()=>new vm.Script(s,{filename:file}));
});
