/* Browser-free workbench behavior tests: no simulated receiver hits. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const core=require('../workbench/core.js');
const catalog={
  'BLE-FLOCK-MFG':{id:'BLE-FLOCK-MFG',expected:'FLOCK'},
  'BLE-NEG-DRONE-UUID':{id:'BLE-NEG-DRONE-UUID',expected:'NONE'},
  'OUI-DJI-04A85A':{id:'OUI-DJI-04A85A',expected:'DRONE'}
};
const sha='a'.repeat(40);
const sample={event:'BEGIN',id:'BLE-FLOCK-MFG',expected:'FLOCK',
  seq:42,ms:1200,carrier:'BLE',mac:'C2:00:01:02:03:04',status:'ok'};
const input='TRUTH,'+JSON.stringify(sample);
assert.deepEqual(core.parseTruth(input),sample);
assert.equal(core.parseTruth('TEST,OUI_ONLY,OUI-DJI-04A85A,04:A8:5A:01:02:03,NO_REMOTE_ID'),null);
assert.equal(core.parseTruth('TRUTH,{not JSON'),null);
assert.equal(core.parseTruth('TRUTH,'+JSON.stringify({...sample,id:'<script>'})),null);
assert.equal(core.parseTruth('TRUTH,'+JSON.stringify({...sample,seq:-1})),null);
const trial=core.makeTrial(sample,{
  runId:'s1',receiver:'CYD-3.2',receiverSha:sha,emitterSha:sha
},catalog);
assert.equal(trial.emission,'pending');
assert.equal(trial.window,'');
assert.equal(core.status(trial,catalog),'UNTESTED');
assert.equal(core.applyEnd(trial,{...sample,event:'END',status:'ok',ms:1250}),true);
assert.equal(trial.emission,'ok');
assert.equal(core.status(trial,catalog),'UNTESTED',
 'emitter BEGIN/END without receiver observation must never score');
trial.window='complete';trial.observed='FLOCK';trial.latency_ms='600';
assert.equal(core.status(trial,catalog),'TP');
assert.equal(core.applyEnd(trial,{...sample,event:'END',id:'OUI-DJI-04A85A'}),false);
const wrong={...trial,observed:'DRONE',latency_ms:''};
const fn={...trial,observed:'NONE',latency_ms:''};
const goodNone={...trial,id:'BLE-NEG-DRONE-UUID',observed:'NONE',latency_ms:''};
const badNone={...goodNone,observed:'DRONE'};
assert.equal(core.status(wrong,catalog),'WRONG');
assert.equal(core.status(fn,catalog),'FN');
assert.equal(core.status(goodNone,catalog),'TN');
assert.equal(core.status(badNone,catalog),'FP');
assert.equal(core.status({...trial,emitter_sha:''},catalog),'UNTESTED');
assert.equal(core.status({...trial,window:'incomplete'},catalog),'EXCLUDED');
assert.equal(core.status({...trial,emission:'failed'},catalog),'EXCLUDED');
const score=core.score([trial,wrong,fn,goodNone,badNone],catalog);
assert.deepEqual(score.count,{TP:1,TN:1,FP:1,FN:1,WRONG:1,UNTESTED:0,EXCLUDED:0});
assert.equal(score.scored,5);
assert.equal(score.precision,1/3);
assert.equal(score.recall,1/3);
assert.equal(score.medianMs,600);
const chars={...trial,notes:'word,"quoted"\nsecond line',run_id:'=2+2'};
const csv=core.exportCsv([chars,fn]);
assert.match(csv,/\"word,\"\"quoted\"\"\nsecond line\"/);
assert.match(csv,/'=2\+2/,'CSV exports must neutralize formulas');
const pendingCsv=core.exportCsv([core.makeTrial(sample,{
  runId:'pending-01',receiver:'CYD-3.2',receiverSha:sha,emitterSha:sha
},catalog)]);
const pendingImported=core.parseCsv(pendingCsv);
assert.equal(pendingImported.length,1);
assert.equal(pendingImported[0].emission,'',
 'pending internal UI state must export blank for Python score_trials.py');
assert.equal(core.status(pendingImported[0],catalog),'UNTESTED');
assert.throws(()=>core.parseCsv(pendingCsv.replace(',,incomplete',',pending,incomplete')),/Invalid emission/);
const parsed=core.parseCsv(csv);
assert.equal(parsed.length,2);
assert.equal(parsed[0].notes,chars.notes);
assert.equal(parsed[0].id,'BLE-FLOCK-MFG');
assert.equal(parsed[1].observed,'NONE');
assert.throws(()=>core.parseCsv('bad,header\n1,2\n'),/Missing CSV column/);
assert.throws(()=>core.parseCsv(csv.replace('emission','oops')),/Missing CSV column/);
const app=fs.readFileSync('workbench/app.js','utf8');
for(const required of [
  'navigator.serial.requestPort()','TRUTH BEGIN','core.makeTrial(',
  'core.applyEnd(','saveObservation','core.status(','core.exportCsv(',
  'core.parseCsv(','new TextDecoder','receiver raw log','no commands sent',
  'window.confirm(','authorization.checked','runCase','stopCase',
  'uniqueRunId()','t.seq!=null','old.seq==null?',
  'Duplicate (run_id, receiver, case)']){
  assert.ok(app.toLowerCase().includes(required.toLowerCase()),'missing: '+required);
}
const html=fs.readFileSync('workbench/index.html','utf8');
assert.match(html,/id="receiverSha"/);
assert.match(html,/id="emitterSha"/);
assert.match(html,/id="recvConnect"/);
assert.match(html,/id="emitConnect"/);
assert.match(html,/id="observed"/);
assert.match(html,/id="trialRows"/);
const src=fs.readFileSync('tools/build_installer_site.py','utf8');
assert.match(src,/publish_workbench/);
assert.match(src,/cases\.json/);
const workflow=fs.readFileSync('.github/workflows/ci.yml','utf8');
assert.match(workflow,/node tests\/verify_workbench\.cjs/);
console.log('PASS workbench: strict TRUTH, manual receiver scoring, CSV, UI and publication guards');
