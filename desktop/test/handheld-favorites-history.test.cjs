'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs');
const path=require('node:path');
const dir=path.resolve(__dirname,'../..');
const read=p=>fs.readFileSync(path.join(dir,p),'utf8');
const cyd=read('src/cyd32_ui.cpp'),pc=read('desktop/public/controller.js');
test('handheld favorites persist, never trigger RF on browse',()=>{
 assert.match(cyd,/prefs\.putBytes\("fav"/);
 assert.match(cyd,/prefs\.getBytes\("fav"/);
 assert.match(cyd,/Screen::Favorites/);
 assert.match(cyd,/toggleFavorite\(\)/);
 assert.match(cyd,/screen=Screen::Home;redraw=true;/);
});
test('last eight transmitter results are not claimed as receiver detections',()=>{
 assert.match(cyd,/HistoryEntry history\[8\]/);
 assert.match(cyd,/cyd32UiRecordResult\(/);
 assert.match(cyd,/TX RESULTS ONLY - NOT RX DETECTIONS/);
 assert.match(read('src/main.cpp'),/cyd32UiRecordResult\(s->id,status,millis\(\)\)/);
});
test('native PC view has bounded UI FAVORITE, FAVS and HISTORY contracts',()=>{
 assert.match(cyd,/UI FAVS BEGIN/);
 assert.match(cyd,/UI HISTORY BEGIN/);
 assert.match(cyd,/UI FAVORITE 1/);
 assert.match(pc,/UI FAVORITE 1/);
 assert.match(pc,/UI FAVS ROW/);
 assert.match(pc,/UI HISTORY ROW/);
 assert.match(read('desktop/public/native-emitter.js'),/'FAVORITES','HISTORY'/);
});
