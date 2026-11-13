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
// Every one of the firmware's 172 real signal IDs must have a descriptive
// device-first label, including vendor-OUI and negative-control fixtures.
const allCases=JSON.parse(fs.readFileSync('tests/cases.json','utf8'));
assert.equal(allCases.length,172);
const names=new Map();
for(const c of allCases){
  const name=core.signalName(c);
  assert.ok(name.includes(' — '),'missing device-first label: '+c.id);
  assert.ok(!name.startsWith('Unknown signal'),'anonymous signal: '+c.id);
  assert.ok(!name.includes('undefined'),'incomplete device label: '+c.id);
  assert.ok(!names.has(c.id),'duplicate fixture ID: '+c.id);
  names.set(c.id,name);
}
assert.match(names.get('BLE-HACKER-NAME'),/^Flipper Zero — advertised BLE name$/);
assert.match(names.get('BLE-HACKER-UUID'),/^Flipper Zero — service UUID 0x3081$/);
assert.match(names.get('BLE-META-LUXOTTICA'),/^Luxottica \/ Ray-Ban —/);
assert.match(names.get('OUI-AUTEL-18D7936'),/^Autel Intelligent/);
assert.match(names.get('BLE-NEG-FLIP-MFG'),/^Flipper Zero —/);
assert.throws(()=>core.signalName({id:'BLE-UNKNOWN-NEW',description:'x'}),/No device name/);
const fullPlan=core.batchPlan(allCases,'all');
assert.equal(fullPlan.length,172);
assert.equal(core.batchPlan(allCases,'BLE').length,88);
assert.equal(core.batchPlan(allCases,'WIFI').length,84);
assert.equal(new Set(fullPlan.map(x=>x.id)).size,172);
assert.deepEqual(fullPlan.map(x=>x.id),core.batchPlan(allCases,'all').map(x=>x.id));
assert.throws(()=>core.batchPlan(allCases,'unknown'),/scope/);
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
const invalidPendingCsv=pendingCsv.replace(/(BLE-FLOCK-MFG),,/, '$1,pending,');
assert.notEqual(invalidPendingCsv,pendingCsv);
assert.throws(()=>core.parseCsv(invalidPendingCsv),/Invalid emission/);
const parsed=core.parseCsv(csv);
assert.equal(parsed.length,2);
assert.equal(parsed[0].notes,chars.notes);
assert.equal(parsed[0].id,'BLE-FLOCK-MFG');
assert.equal(parsed[1].observed,'NONE');
assert.throws(()=>core.parseCsv('bad,header\n1,2\n'),/Missing CSV column/);
assert.throws(()=>core.parseCsv(csv.replace('emission','oops')),/Missing CSV column/);
// Actual SquachWatch master LOG command response, not arbitrary debug-text guesses.
assert.equal(core.parseWatchLogCount('[log] 2 rows, newest first'),2);
assert.equal(core.parseWatchLogCount('[log] 0 rows, newest first'),0);
assert.equal(core.parseWatchLogCount('[frame] ble 20, det 1'),null);
assert.equal(core.parseWatchLogCount('[log] 999 rows, newest first'),null);
const row=core.parseWatchLogRow('0,DRONE,04:a8:5a:12:34:56,-53,1,here,+1:04');
assert.equal(row.type,'DRONE');
assert.equal(row.mac,'04:A8:5A:12:34:56');
assert.equal(row.rssi,-53);
assert.equal(core.parseWatchLogRow('0,DRONE,not-a-mac,-53,1,here,+1:04'),null);
assert.equal(core.parseWatchLogRow('0,UNKNOWN,04:a8:5a:12:34:56,-53,1,here,+1:04'),null);
assert.equal(core.parseWatchLogRow('0,EVIL TWIN,04:a8:5a:12:34:56,-53,1,here,+1:04').type,'EVILTWIN');
const snapParser=core.newWatchLogParser();
assert.equal(snapParser.start(950),true);
assert.equal(snapParser.start(951),false,'overlapping LOG commands must not mix');
assert.equal(snapParser.read('[log] 2 rows, newest first',960),null);
assert.equal(snapParser.read('row,type,mac,rssi,hits,state,when',965),null);
assert.equal(snapParser.read('0,DRONE,04:a8:5a:12:34:56,-53,1,here,+1:04',970),null);
const after=snapParser.read('1,FLOCK,02:11:22:33:44:55,-81,2,here,+1:09',985);
assert.equal(after.requestedAt,950);
assert.equal(after.finishedAt,985);
assert.equal(after.rows.length,2);
assert.equal(snapParser.waiting(),false);
assert.equal(snapParser.start(1050),true);
snapParser.read('[log] 0 rows, newest first',1055);
const zero=snapParser.read('row,type,mac,rssi,hits,state,when',1060);
assert.equal(zero.rows.length,0);
const matchingTrial={_mac:'04:a8:5a:12:34:56',_startedAt:1100};
const baseline={requestedAt:990,finishedAt:1000,rows:[]};
const positive={requestedAt:1150,finishedAt:1180,rows:[row]};
const hit=core.matchWatchDetection(matchingTrial,baseline,positive);
assert.equal(hit.observed,'DRONE');
assert.equal(hit.mac,'04:A8:5A:12:34:56');
assert.equal(core.matchWatchDetection(matchingTrial,baseline,
  {...positive,rows:[{...row,mac:'04:A8:5A:AA:BB:CC'}]}),null,
  'same vendor OUI with different MAC is not a receiver hit');
assert.equal(core.matchWatchDetection(matchingTrial,baseline,
  {...positive,rows:[{...row,state:'KEPT'}]}),null,
  'restored flash record must not count');
assert.equal(core.matchWatchDetection(matchingTrial,
  {...baseline,rows:[row]},positive),null,
  'preexisting unchanged MAC must not count');
assert.equal(core.matchWatchDetection(matchingTrial,
  {...baseline,rows:[{...row,hits:1,state:'gone'}]},
  {...positive,rows:[{...row,hits:2}]}).observed,'DRONE',
  'verified reappearance after baseline can count');
assert.equal(core.matchWatchDetection(matchingTrial,
  {...baseline,finishedAt:1099},
  {...positive,requestedAt:1098}),null,
  'poll requested before emitter BEGIN is not fresh evidence');
assert.equal(core.matchWatchDetection(matchingTrial,
  {...baseline,finishedAt:1000},
  {...positive,requestedAt:1105,rows:[]}),null,
  'absence cannot be auto-scored as NONE');
const amb=core.matchWatchDetection(matchingTrial,baseline,
  {...positive,rows:[row,{...row,type:'FLOCK'}]});
assert.equal(amb.ambiguous,true,'two classifications for one MAC require review');
const malformed=core.newWatchLogParser();
malformed.start(100);
malformed.read('[log] 1 rows, newest first',105);
malformed.read('row,type,mac,rssi,hits,state,when',106);
assert.equal(malformed.read('0,DRONE,not a mac,-53,1,here,+1:04',107),null);
assert.equal(malformed.waiting(),false,
  'malformed receiver rows must fail closed, never auto-detect');
// An actual AUTO SquachWatch hit is real evidence, but missing commit SHAs
// must not silently turn it into a counted benchmark result.
const missingBoth={...trial,receiver_sha:'',emitter_sha:''};
assert.equal(core.status(missingBoth,catalog),'UNTESTED');
assert.match(core.pendingReason(missingBoth,catalog),/SquachWatch and SquachEmit firmware SHAs/);
assert.equal(core.pendingReason({...missingBoth,receiver_sha:sha},catalog),
  'Missing/invalid SquachEmit firmware SHA');
assert.equal(core.pendingReason({...missingBoth,emitter_sha:sha},catalog),
  'Missing/invalid SquachWatch firmware SHA');
assert.equal(core.status({...missingBoth,emitter_sha:sha,receiver_sha:sha},catalog),'TP');
assert.equal(core.pendingReason(trial,catalog),'');
assert.match(core.pendingReason({...trial,emission:'pending'},catalog),/Waiting for actual/);
assert.match(core.pendingReason({...trial,window:''},catalog),/receiver window/);
const app=fs.readFileSync('workbench/app.js','utf8');
for(const required of [
  'navigator.serial.requestPort()','TRUTH BEGIN','core.makeTrial(',
  'core.applyEnd(','saveObservation','core.status(','core.exportCsv(',
  'core.parseCsv(','new TextDecoder','receiver raw log','no commands sent',
  'window.confirm(','authorization.checked','runCase','stopCase',
  'uniqueRunId()','t.seq!=null','old.seq==null?',
  'applySessionShas()','core.pendingReason(','runProgress(',
  'Promise.race([','receiver.preRun={id:c.id,snapshot:baseline',
  'RUN was sent but no TRUTH BEGIN arrived',
  'Duplicate (run_id, receiver, case)','requestWatchSnapshot()',
  'core.matchWatchDetection(','onWatchSnapshot(','startWatchPolling()',
  'core.signalName(','core.batchPlan(','onBatchTruth(','startBatchStep()',
  'endBatch(','batchDuration','batchGap','batchScope',
  "new TextEncoder().encode('LOG\\n')",'if(t.emission===\'ok\')finishAutoMatch(t)']){
  assert.ok(app.toLowerCase().includes(required.toLowerCase()),'missing: '+required);
}
assert.ok(!app.includes('RUN not sent.'),'receiver LOG timeout must not suppress a user RUN');
assert.ok(app.includes("if(!baseline){\n          autoStatus("),
  'missing receiver baseline should downgrade automatic scoring, not prevent RUN');
const html=fs.readFileSync('workbench/index.html','utf8');
assert.match(html,/id="applyShas"/);
assert.match(html,/id="shaNotice"/);
assert.match(html,/id="runFeedback"/);
assert.match(html,/id="receiverSha"/);
assert.match(html,/id="emitterSha"/);
assert.match(html,/id="recvConnect"/);
assert.match(html,/id="emitConnect"/);
assert.match(html,/id="observed"/);
assert.match(html,/id="trialRows"/);
for(const id of ['batchStart','batchStop','batchScope','batchDuration',
    'batchGap','batchProgress','batchCount','batchCurrent'])
  assert.ok(html.includes('id="'+id+'"'),'missing automation control: '+id);
const src=fs.readFileSync('tools/build_installer_site.py','utf8');
assert.match(src,/publish_workbench/);
assert.match(src,/cases\.json/);
const workflow=fs.readFileSync('.github/workflows/ci.yml','utf8');
assert.match(workflow,/node tests\/verify_workbench\.cjs/);
console.log('PASS workbench: strict TRUTH, manual receiver scoring, CSV, UI and publication guards');
