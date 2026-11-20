'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path');
const root=path.resolve(__dirname,'../..');
const src=p=>fs.readFileSync(path.join(root,p),'utf8');
const m=src('src/main.cpp'),cyd=src('src/cyd32_ui.cpp');
test('playlist requires a physical action and uses favorite cases only',()=>{
 assert.match(cyd,/Cyd32UiAction::PlaylistStart/);
 assert.match(m,/if\(cyd32UiFavoriteCount\(\)\)/);
 assert.match(m,/cyd32UiFavoriteAt\(g_playlistNext\)/);
 assert.match(m,/startScenario\(next,selectedDurationMs\(\)\)/);
});
test('STOP revokes playlist and pause resumes only by user tap',()=>{
 assert.match(m,/g_playlist=false;g_playlistPaused=false;g_playlistPauseRequested=false/);
 assert.match(m,/STATE,PLAYLIST_PAUSED/);
 assert.match(m,/STATE,PLAYLIST_RESUMED/);
 assert.match(cyd,/Cyd32UiAction::PlaylistPause/);
 assert.match(cyd,/Cyd32UiAction::PlaylistResume/);
});
test('duration presets remain inside valid three to thirty second range',()=>{
 assert.match(cyd,/setDuration\(3\)/);
 assert.match(cyd,/setDuration\(10\)/);
 assert.match(cyd,/setDuration\(30\)/);
});
test('native Windows renderer follows bounded playlist state',()=>{
 assert.match(src('desktop/public/native-emitter.js'),/playlistPaused/);
 assert.match(src('desktop/public/controller.js'),/UI PLAYLIST 1/);
});
