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
// Every one of the LAB R14.39 firmware's 369 reviewed signal IDs must have a descriptive
// device-first label, including vendor-OUI and negative-control fixtures.
const allCases=JSON.parse(fs.readFileSync('workbench/catalog-369.json','utf8'));
assert.equal(allCases.length,369);
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
const forkProfile=JSON.parse(fs.readFileSync('workbench/fork_expectations.json','utf8'));
const upstreamCases=core.receiverCatalog(allCases,'upstream',forkProfile);
const forkCases=core.receiverCatalog(allCases,'fork',forkProfile);
assert.equal(upstreamCases.length,369);
assert.equal(forkCases.length,369);
assert.equal(Object.keys(forkProfile.overrides).length,19);
const upstreamById=Object.fromEntries(upstreamCases.map(x=>[x.id,x]));
const forkById=Object.fromEntries(forkCases.map(x=>[x.id,x]));
for(const [id,want] of Object.entries(forkProfile.overrides)){
  assert.equal(forkById[id].expected,want,'fork receiver expectation '+id);
  assert.equal(upstreamById[id].expected,'NONE','upstream untouched '+id);
}
assert.equal(forkById['FORK-BLE-SKIM-BT04'].expected,'SKIMMER');
assert.equal(upstreamById['FORK-BLE-SKIM-BT04'].expected,'NONE');
assert.equal(forkById['OUI-INSPFLIGHT-34B5F32'].expected,'DRONE');
assert.equal(upstreamById['OUI-INSPFLIGHT-34B5F32'].expected,'NONE');
assert.equal(allCases.find(x=>x.id==='FORK-BLE-SKIM-BT04').expected,'NONE',
  'profiles must not mutate raw signal data');
assert.throws(()=>core.receiverCatalog(allCases,'fork',{overrides:{'NOT-A-CASE':'DRONE'}}),/Unknown/);
const receiverTrial={id:'FORK-BLE-SKIM-BT04',emission:'ok',window:'complete',
  observed:'SKIMMER',receiver_sha:'a'.repeat(40),emitter_sha:'b'.repeat(40)};
assert.equal(core.status(receiverTrial,forkById),'TP');
assert.equal(core.status(receiverTrial,upstreamById),'FP');
const fullPlan=core.batchPlan(allCases,'all');
assert.equal(fullPlan.length,369);
assert.equal(core.batchPlan(allCases,'BLE').length,116);
assert.equal(core.batchPlan(allCases,'WIFI').length,253);
assert.equal(new Set(fullPlan.map(x=>x.id)).size,369);
assert.deepEqual(fullPlan.map(x=>x.id),core.batchPlan(allCases,'all').map(x=>x.id));
assert.throws(()=>core.batchPlan(allCases,'unknown'),/scope/);
const retestRows=[
  {id:fullPlan[0].id,emission:'ok',window:'complete',observed:'DRONE'},
  {id:fullPlan[1].id,emission:'ok',window:'',observed:'DRONE'},
  {id:fullPlan[2].id,emission:'ok',window:'',observed:''},
  {id:fullPlan[3].id,emission:'pending',window:'',observed:''}
];
const needsReceiver=core.receiverRetestPlan(allCases,retestRows);
assert.equal(needsReceiver.length,368);
const sameHardware={receiver:'FREENOVE-S3-3.5',receiverSha:'a'.repeat(40),emitterSha:'b'.repeat(40)};
const mixed=[
  {id:fullPlan[0].id,emission:'ok',window:'complete',observed:'DRONE',
   receiver:'CYD-2.8',receiver_sha:'a'.repeat(40),emitter_sha:'b'.repeat(40)},
  {id:fullPlan[1].id,emission:'ok',window:'complete',observed:'DRONE',
   receiver:'FREENOVE-S3-3.5',receiver_sha:'a'.repeat(40),emitter_sha:'b'.repeat(40)},
  {id:fullPlan[2].id,emission:'ok',window:'complete',observed:'DRONE',
   receiver:'FREENOVE-S3-3.5',receiver_sha:'f'.repeat(40),emitter_sha:'b'.repeat(40)}
];
const exactReceiver=core.receiverRetestPlan(allCases,mixed,sameHardware);
assert.equal(exactReceiver.length,368);
assert.ok(exactReceiver.some(c=>c.id===fullPlan[0].id),
  'another receiver must not suppress the Freenove retest');
assert.ok(!exactReceiver.some(c=>c.id===fullPlan[1].id));
assert.ok(exactReceiver.some(c=>c.id===fullPlan[2].id),
  'another firmware SHA must not suppress the Freenove retest');

assert.ok(!needsReceiver.some(c=>c.id===fullPlan[0].id));
assert.ok(needsReceiver.some(c=>c.id===fullPlan[1].id));
assert.ok(needsReceiver.some(c=>c.id===fullPlan[2].id));
assert.ok(needsReceiver.some(c=>c.id===fullPlan[3].id));
const stableCases=JSON.parse(fs.readFileSync('tests/cases.json','utf8'));
assert.equal(stableCases.length,362);
assert.notEqual(core.catalogHash(stableCases),core.catalogHash(allCases),
  '362 and 369 catalogs must have distinct capability fingerprints');
assert.equal(core.catalogHash([{id:'A'}]),'64D579B2');

const alreadyTransmitted=[
  {id:fullPlan[0].id,emission:'ok'},
  {id:fullPlan[1].id,emission:'pending'},
  {id:fullPlan[2].id,emission:'failed'}
];
const outstanding=core.remainingEmitterPlan(allCases,alreadyTransmitted);
assert.equal(outstanding.length,368);
assert.ok(!outstanding.some(x=>x.id===fullPlan[0].id));
assert.ok(outstanding.some(x=>x.id===fullPlan[1].id));
assert.ok(outstanding.some(x=>x.id===fullPlan[2].id));
const completed300=fullPlan.slice(0,300).map(x=>({id:x.id,emission:'ok'}));
assert.equal(core.remainingEmitterPlan(allCases,completed300).length,69,
  '300 successful transmissions must leave 69 to test, not 68');
assert.throws(()=>core.remainingEmitterPlan(allCases,{}),/Invalid trial ledger/);
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
assert.equal(core.parseWatchLogRow('0,UNKNOWN,04:a8:5a:12:34:56,-53,1,here,+1:04').type,'UNKNOWN',
  'an UNKNOWN receiver row must not invalidate an otherwise valid LOG snapshot');
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
// Periodic firmware diagnostics can interleave LOG table rows while
// another task sends data over the same USB serial port. They must not
// destroy a legitimate receiver snapshot or ACTUAL stays blank.
const interleaved=core.newWatchLogParser();
assert.equal(interleaved.start(1200),true);
assert.equal(interleaved.read('[log] 2 rows, newest first',1202),null);
assert.equal(interleaved.read('row,type,mac,rssi,hits,state,when',1203),null);
assert.equal(interleaved.read('[gps] bytes=1200 UBX=17 SAT=4 fix=1',1204),null);
assert.equal(interleaved.read('0,DRONE,04:a8:5a:12:34:56,-55,1,here,+1:09',1205),null);
assert.equal(interleaved.read('[queue] ble q 0 hi 2 drop 0',1206),null);
const interleavedResult=interleaved.read('1,FLOCK,02:11:22:33:44:55,-81,2,gone,+1:08',1207);
assert.equal(interleavedResult.rows.length,2);
assert.equal(interleavedResult.rows[1].state,'gone');
assert.equal(interleaved.waiting(),false);

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
assert.equal(core.matchWatchDetection(matchingTrial,baseline,
  {...positive,rows:[{...row,state:'gone'}]}).observed,'DRONE',
  'a new exact-MAC detection must be reported even after the signal stopped');
assert.equal(core.matchWatchDetection(matchingTrial,baseline,
  {...positive,rows:[{...row,type:'UNKNOWN'}]}),null,
  'UNKNOWN receiver classification is not a positive detection');
assert.equal(core.matchWatchDetection(matchingTrial,null,positive),null,
  'missing baseline cannot count toward accuracy');
const provisional=core.findUnverifiedWatchObservation(matchingTrial,positive);
assert.equal(provisional.observed,'DRONE');
assert.equal(provisional.verified,false);
assert.equal(core.status({...trial,observed:provisional.observed,window:''},catalog),
  'UNTESTED','ACTUAL may be populated without inventing a verified listening window');
assert.equal(core.findUnverifiedWatchObservation(matchingTrial,{
  ...positive,rows:[{...row,mac:'04:A8:5A:AA:BB:CC'}]}),null,
  'classification alone cannot populate ACTUAL when the MAC does not match');
assert.equal(core.findUnverifiedWatchObservation(matchingTrial,{
  ...positive,requestedAt:1099}),null,
  'a pre-RUN receiver snapshot is not evidence of this transmission');
assert.equal(core.findUnverifiedWatchObservation(matchingTrial,{
  ...positive,rows:[{...row,state:'gone'}]}).observed,'DRONE',
  'post-run exact-MAC gone rows can be shown as unverified receiver evidence');
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
  'core.matchWatchDetection(','core.findUnverifiedWatchObservation(',
  'onWatchSnapshot(','startWatchPolling()',
  'core.signalName(','core.batchPlan(','onBatchTruth(','startBatchStep()',
  'endBatch(','batchDuration','batchGap','batchScope',
  "sendReceiver('LOG')",'if(t.emission===\'ok\')finishAutoMatch(t)']){
  assert.ok(app.toLowerCase().includes(required.toLowerCase()),'missing: '+required);
}
assert.ok(app.includes('var receiverWrites=Promise.resolve()'),
  'LOG polling and Windows GUI commands must share one serialized receiver writer');
assert.ok(app.includes('batch.receiverLogUnavailable=true'),
  'a missing SquachWatch LOG response must require an explicit unverified batch choice');
assert.ok(app.includes('TRANSMITTER-ONLY / ') && app.includes('UNVERIFIED batch?'),
  'unverified RF permission prompt must distinguish transmission from detection proof');
assert.ok(app.includes('noBatchLog?{id:c.id,snapshot:null'),
  'unverified batch must prevent stale baseline fallback and false scoring');
assert.ok(app.includes('batch.current.noBaseline=true'),
  'every unverified batch case must contribute to unverified count');
assert.ok(!app.includes('LOG unavailable after automatic mirror pause. Check receiver USB'),
  'old generic blocking message must be removed');
assert.ok(app.includes("if(!(await sendReceiver('LOG')))"),
  'automatic receiver polling must use the queued serial LOG path');
assert.ok(!app.includes('RUN not sent.'),'receiver LOG timeout must not suppress a user RUN');
assert.ok(app.includes("if(!baseline){\n          autoStatus("),
  'missing receiver baseline should downgrade automatic scoring, not prevent RUN');
assert.ok(app.includes("endBatch('AUTOMATION HALTED at case '"),
  'unconfirmed transmitter attempts must HALT with a resumable checkpoint');
assert.ok(app.includes("This signal is NOT counted as complete"),
  'failed attempts must not increment the completed counter');
assert.ok(app.includes("if((batch.active||preparingBatch)&&!fromBatch)return"),
  'a manual RUN must not race a batch still preparing receiver/emitter preflight');
assert.ok(app.includes("footerCount!==null") && app.includes("footerHash"),
  'new emitter LIST streams must verify count and hash before authorizing RF');
assert.ok(app.includes("await sendEmitter('BUILD')") &&
  app.includes("await sendEmitter('CATALOG')"),
  'connected emitter must expose build and catalog identity when available');
assert.ok(app.includes("Emitter SHA does not match connected firmware"),
  'do not start a batch when connected source SHA contradicts the user entry');
assert.ok(app.includes("USB LOG snapshots during run"),
  'unverified trial records should preserve receiver LOG transport diagnostics');
assert.ok(app.includes("probeEmitterCatalog()") && app.includes("sendEmitter('LIST')"),
  'batch and individual tests must check actual transmitter capabilities');
assert.ok(app.includes("core.receiverRetestPlan(cases,trials,{receiver:ui.receiverBoard.value"),
  'operator must be able to retest emission-successful but unverified receiver cases with board-specific provenance');
assert.ok(app.includes("receiver:old.receiver||ui.receiverBoard.value"),
  'manual edits must not relabel imported hardware evidence');
assert.ok(app.includes("core.remainingEmitterPlan(cases,trials)"),
  'remaining-run UI must use successful TRUTH END records from trial ledger');
assert.ok(app.includes("if((ui.batchScope.value==='missing'||ui.batchScope.value==='unverified') && !trials.length)"),
  'remaining-run UI must require a prior ledger import');
const html=fs.readFileSync('workbench/index.html','utf8');
assert.match(html,/id="applyShas"/);
assert.match(html,/id="shaNotice"/);
assert.match(html,/id="runFeedback"/);
assert.match(html,/id="receiverSha"/);
assert.match(html,/id="emitterSha"/);
assert.match(html,/id="recvConnect"/);
assert.match(html,/FREENOVE-S3-3\.5/);
assert.match(html,/id="receiverLibrary"/);
assert.match(html,/369 cases/);
assert.match(html,/Remaining \(not END ok in imported\/current trial ledger\)/);
assert.match(html,/Retest receiver-unverified/);
assert.match(app,/fork_expectations\.json/);
assert.match(app,/FREENOVE-S3-3\.5/);
assert.match(app,/cases\.length===369/);
assert.match(app,/ui\.receiverBoard\.value==='FREENOVE-S3-3\.5'\?'115200':'2000000'/);

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
console.log('PASS workbench: TRUTH, exact-MAC ACTUAL (live/gone), unverified baseline safety, CSV, UI and publication guards');
