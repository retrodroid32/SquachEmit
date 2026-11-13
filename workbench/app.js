/* SquachEmit Detection Workbench — browser-side observation UI.
 * Hardware is touched ONLY after a user clicks Connect/Run/STOP.
 * Structured receiver LOG snapshots provide correlated, fresh exact-MAC
 * detections; never guess an alert from unrelated debug text or old rows.
 */
(function () {
  'use strict';
  var core=window.SQEWorkbenchCore;
  if(!core) throw new Error('Workbench core.js missing');
  function byId(id){var el=document.getElementById(id);if(!el)throw new Error('Missing '+id);return el;}
  var ids=['securityNotice','scoredCount','pendingCount','recall','precision','latency',
    'runId','receiverBoard','receiverSha','emitterSha','applyShas','shaNotice',
    'emitState','emitConnect',
    'emitDisconnect','caseSelect','caseDetails','duration','authorization','runCase',
    'stopCase','runFeedback','emitLog','recvState','recvConnect','recvDisconnect','receiverBaud',
    'recvLog','copyReceiver','autoLogState','autoDetectionFeed','pollNow',
    'trialSelect','trialDetails','emissionStatus','listenWindow',
    'observed','latencyMs','evidenceNotes','saveObservation','observationNotice',
    'trialRows','importCsv','csvFile','exportCsv','clearTrials',
    'batchScope','batchDuration','batchGap','batchStart','batchStop',
    'batchProgress','batchCount','batchCurrent'];
  var ui={};
  ids.forEach(function(id){ui[id]=byId(id);});
  var catalog={},cases=[],trials=[],selectedTrial=-1;
  var devices={emitter:{port:null,reader:null,reading:null,buffer:'',log:'',baud:115200},
               receiver:{port:null,reader:null,reading:null,buffer:'',log:'',baud:2000000,
                 parser:core.newWatchLogParser(),lastSnapshot:null,pending:null,
                 pollTimer:null,preRun:null,autoLog:'',session:0}};
  var opening={emitter:false,receiver:false};
  var epoch=0;
  var startingRun=false;
  var runToken=0; // STOP/disconnect cancels any outstanding LOG preflight
  var runAwaitTimer=null;
  var awaitingBegin=false;
  var transmitting=false;
  var batch={active:false,queue:[],index:0,completed:0,current:null,
    duration:8,gap:5,timer:null,endTimer:null,failures:0,unverified:0,
    message:'Ready to start'};
  var emitterWrites=Promise.resolve();
  var receiverWrites=Promise.resolve();
  var desktopMode=(new URLSearchParams(window.location.search)).get('desktop')==='1';
  function desktopEvent(type,detail){
    if(desktopMode)window.dispatchEvent(new CustomEvent('sqe:'+type,{detail:detail}));
  }
  function runProgress(message){text('runFeedback',message);}
  function cancelRunAck(){
    if(runAwaitTimer!==null){clearTimeout(runAwaitTimer);runAwaitTimer=null;}
    awaitingBegin=false;
  }
  ui.runId.value=new Date().toISOString().slice(0,10)+'-session-01';
  function text(id,value){ui[id].textContent=String(value);}
  function notice(message,isError){
    text('securityNotice',message);
    ui.securityNotice.style.borderLeftColor=isError?'#ff7a85':'#ffbc75';
  }
  function append(role,line){
    var d=devices[role], el=role==='emitter'?ui.emitLog:ui.recvLog;
    d.log=(d.log+line).slice(-60000);
    el.textContent=d.log;
    el.scrollTop=el.scrollHeight;
    desktopEvent('serial',{role:role,line:line});
  }
  function option(value,label){var o=document.createElement('option');o.value=value;o.textContent=label;return o;}
  function formatPct(x){return x===null?'—':(x*100).toFixed(1)+'%';}
  function validSha(str){return /^[a-f0-9]{7,40}$/i.test(String(str||'').trim());}
  function chooseCase(){
    var c=catalog[ui.caseSelect.value];
    if(!c){text('caseDetails','Select a catalog case to inspect its expected classification.');}
    else{
      text('caseDetails',core.signalName(c)+'\nID: '+c.id+' · '+c.carrier+
        '\nSQUACHWATCH EXPECTED: '+c.expected+
        '\n'+c.description+
        (c.id.startsWith('OUI-')?
        '\nCAUTION: selected case transmits a registered vendor MAC prefix; this is NOT proof of a genuine device or Remote ID.':''));
    }
    updateButtons();
  }
  function refreshMetric(){
    var m=core.score(trials,catalog);
    text('scoredCount',m.scored);
    var evidenced=trials.filter(function(t){
      return t.emission==='ok'&&t.window==='complete'&&!!t.observed &&
        core.status(t,catalog)==='UNTESTED';
    }).length;
    text('pendingCount',m.count.UNTESTED+' untested · '+m.count.EXCLUDED+
      ' excluded'+(evidenced?' · '+evidenced+' detections awaiting firmware SHAs':''));
    text('recall',formatPct(m.recall));
    text('precision',formatPct(m.precision));
    text('latency',m.medianMs===null?'—':Math.round(m.medianMs)+' ms');
  }
  function addCell(tr,value){
    var td=document.createElement('td');td.textContent=String(value);tr.appendChild(td);return td;
  }
  function redrawTrials(){
    ui.trialRows.replaceChildren();
    if(!trials.length){
      var empty=document.createElement('tr');var cell=document.createElement('td');
      cell.colSpan=7;cell.className='empty';
      cell.textContent='No trials yet. Connect an emitter or import a trial CSV.';
      empty.appendChild(cell);ui.trialRows.appendChild(empty);
    }
    trials.forEach(function(t,i){
      var row=document.createElement('tr'),status=core.status(t,catalog);
      row.addEventListener('click',function(){ui.trialSelect.value=String(i);selectTrial();});
      addCell(row,(catalog[t.id]?core.signalName(catalog[t.id]):'Unknown device')+
        '\n'+t.id);
      addCell(row,(catalog[t.id]||{}).expected||'Unknown');
      addCell(row,t.observed||'—');addCell(row,t.emission||'—');addCell(row,t.window||'—');
      var td=document.createElement('td'),badge=document.createElement('span');
      badge.className='result '+status;badge.textContent=status;td.appendChild(badge);
      var why=core.pendingReason(t,catalog);
      if(why){
        var reason=document.createElement('div');
        reason.className='result-reason';reason.textContent=why;td.appendChild(reason);
        row.title=why;
      }
      row.appendChild(td);
      addCell(row,t.latency_ms?String(t.latency_ms)+' ms':'—');
      ui.trialRows.appendChild(row);
    });
    refreshMetric();
    ui.trialSelect.replaceChildren(option('','Select a trial…'));
    trials.forEach(function(t,i){
      ui.trialSelect.appendChild(option(String(i),String(i+1)+' · '+
        (catalog[t.id]?core.signalName(catalog[t.id]):t.id)+
        ' ['+t.id+'] · '+core.status(t,catalog)));
    });
    if(selectedTrial>=0 && selectedTrial<trials.length)ui.trialSelect.value=String(selectedTrial);
    updateButtons();
  }
  function selectTrial(){
    var v=ui.trialSelect.value;selectedTrial=v===''?-1:Number(v);
    var t=trials[selectedTrial];
    if(!t){
      text('trialDetails','Select a TRUTH BEGIN trial or import existing CSV records.');
      ui.emissionStatus.value='';ui.emissionStatus.disabled=true;
      ui.listenWindow.value='';ui.observed.value='';
      ui.latencyMs.value='';ui.evidenceNotes.value='';
    }else{
      text('trialDetails',(catalog[t.id]?core.signalName(catalog[t.id]):'Unknown device')+
        '\nCASE: '+t.id+' · expected '+((catalog[t.id]||{}).expected||'UNKNOWN')+
        '\nEmitter sequence: '+(t.seq==null?'Imported':t.seq)+
        '\nEmitter result: '+(t.emission||'pending')+
        '\nReceiver: '+(t._autoHit ? 'AUTO MATCH '+t._autoHit.observed+
            ' at '+t._autoHit.mac+' (fresh LOG row)' :
            t._baseline ? 'Watching for new MAC match in LOG' :
            'No pre-test LOG baseline; manual review required')+
        '\nNo automatic NONE is inferred from absent entries.');
      ui.emissionStatus.value=t.emission==='ok'||t.emission==='failed'?t.emission:'';
      // Live serial attempts can only be successful after an actual
      // matched TRUTH END. Manual 'ok' is permitted only for imported CSV.
      ui.emissionStatus.disabled=t.seq!=null;
      ui.listenWindow.value=t.window||'';
      ui.observed.value=t.observed||'';
      ui.latencyMs.value=t.latency_ms||'';
      ui.evidenceNotes.value=t.notes||'';
    }
    if(t && core.status(t,catalog)==='UNTESTED' && t.observed){
      text('observationNotice','Receiver result recorded: '+t.observed+
        '. '+core.pendingReason(t,catalog));
    }
    updateButtons();
  }
  function updateButtons(){
    var connected=!!devices.emitter.port && !opening.emitter;
    var eBusy=opening.emitter;
    ui.emitState.textContent=connected?'CONNECTED · 115,200':eBusy?'CONNECTING':'DISCONNECTED';
    ui.emitState.classList.toggle('connected',connected);
    ui.emitConnect.disabled=connected||eBusy;
    ui.emitDisconnect.disabled=!connected||eBusy;
    var receiver=!!devices.receiver.port && !opening.receiver;
    ui.recvState.textContent=receiver?'CONNECTED · '+devices.receiver.baud.toLocaleString('en-US'):
      opening.receiver?'CONNECTING':'DISCONNECTED';
    ui.recvState.classList.toggle('connected',receiver);
    ui.recvConnect.disabled=receiver||opening.receiver;
    ui.recvDisconnect.disabled=!receiver||opening.receiver;
    ui.receiverBaud.disabled=receiver||opening.receiver;
    ui.pollNow.disabled=!receiver;
    var duration=Number(ui.duration.value);
    ui.runCase.disabled=!connected||batch.active||startingRun||awaitingBegin||transmitting||
      !catalog[ui.caseSelect.value]||!ui.authorization.checked||
      !Number.isInteger(duration)||duration<3||duration>30;
    ui.runCase.textContent=startingRun?'Preparing receiver…':
      awaitingBegin?'Waiting for emitter…':
      transmitting?'Transmitting…':'Run selected case';
    ui.applyShas.disabled=!(validSha(ui.receiverSha.value)&&validSha(ui.emitterSha.value));
    ui.stopCase.disabled=!connected;
    updateBatchUi();
    ui.saveObservation.disabled=!(selectedTrial>=0 && trials[selectedTrial]);
    ui.clearTrials.disabled=batch.active;
    ui.importCsv.disabled=batch.active;
    desktopEvent('state',{
      emitterConnected:connected,
      receiverConnected:receiver,
      receiverBaud:devices.receiver.baud,batchActive:batch.active,
      transmitting:transmitting
    });
  }
  function updateBatchUi(){
    var ready=!!devices.emitter.port&&!!devices.receiver.port&&
      !opening.emitter&&!opening.receiver&&ui.authorization.checked&&
      !startingRun&&!awaitingBegin&&!transmitting&&cases.length===172;
    var duration=Number(ui.batchDuration.value),gap=Number(ui.batchGap.value);
    var valid=Number.isInteger(duration)&&duration>=3&&duration<=30&&
      Number.isInteger(gap)&&gap>=3&&gap<=30;
    ui.batchStart.disabled=batch.active||!ready||!valid;
    ui.batchStop.disabled=!batch.active;
    ['batchScope','batchDuration','batchGap','caseSelect','duration'].forEach(function(id){
      ui[id].disabled=batch.active;
    });
    ui.batchProgress.max=batch.queue.length||cases.length||172;
    ui.batchProgress.value=batch.completed;
    text('batchCount',batch.completed+' / '+(batch.queue.length||cases.length||172)+
      ' completed · '+batch.failures+' transmitter failures · '+
      batch.unverified+' unverified recoveries'+
      (batch.active?' · RUNNING':''));
    text('batchCurrent',batch.message);
  }
  function uniqueRunId(){
    // The existing Python scorer requires (run_id, receiver, case ID) unique.
    // Each repeated physical trial therefore receives its own run suffix.
    var base=ui.runId.value.trim()||'workbench-session';
    var index=trials.length+1;
    var id=base+'-trial-'+String(index).padStart(4,'0');
    while(trials.some(function(t){return t.run_id===id;})){
      index++;
      id=base+'-trial-'+String(index).padStart(4,'0');
    }
    return id;
  }
  function trialKey(t){
    return [t.run_id,t.receiver,t.id].join('\u0000');
  }
  function autoStatus(message) {
    text('autoLogState',message);
  }
  function matchedLog(message) {
    var d=devices.receiver;
    d.autoLog=(d.autoLog+message+'\n').slice(-9000);
    ui.autoDetectionFeed.textContent=d.autoLog;
    ui.autoDetectionFeed.scrollTop=ui.autoDetectionFeed.scrollHeight;
  }
  function finishAutoMatch(t) {
    if(!t._autoHit || t._manual || t.emission!=='ok')return false;
    t.observed=t._autoHit.observed;
    t.window='complete'; // one independently observed hit is enough for a positive
    t.latency_ms=''; // the two device clocks are NOT synchronized
    t.notes='AUTO SquachWatch LOG: fresh exact MAC '+t._autoHit.mac+
      ', class '+t._autoHit.observed+', RSSI '+t._autoHit.rssi+
      ' dBm, hits '+t._autoHit.hits+', receiver snapshot '+new Date(t._autoHit.finishedAt).toISOString()+
      '. Emission END ok reported; polling cadence is not precise detection latency.';
    return true;
  }
  function onWatchSnapshot(snapshot) {
    var d=devices.receiver;
    d.lastSnapshot=snapshot;
    desktopEvent('snapshot',{rows:snapshot.rows.slice(0,20).map(function(r){
      return {type:r.type,mac:r.mac,rssi:r.rssi,hits:r.hits,state:r.state};
    }),rowCount:snapshot.rows.length,when:snapshot.finishedAt});
    autoStatus('AUTO LOG ACTIVE · '+snapshot.rows.length+' receiver rows · latest snapshot '+
      new Date(snapshot.finishedAt).toLocaleTimeString()+' · polling every 1.5 s');
    if(d.pending){
      var ready=d.pending;d.pending=null;
      clearTimeout(ready.timeout);
      ready.resolve(snapshot);
    }
    var change=false;
    for(var i=trials.length-1;i>=0;i--){
      var t=trials[i];
      if(t._epoch!==epoch || t._receiverSession!==d.session ||
         !t._baseline || !t._mac || t._autoHit || t._manual ||
         t.emission==='failed')continue;
      if(t._endedAt && snapshot.requestedAt>t._endedAt+5000)continue;
      var hit=core.matchWatchDetection(t,t._baseline,snapshot);
      if(!hit)continue;
      if(hit.ambiguous){
        autoStatus('AMBIGUOUS: '+hit.mac+' appeared under multiple classifications; inspect LOG manually.');
        continue;
      }
      t._autoHit=hit;
      t.observed=hit.observed;
      t.notes='SquachWatch matched '+hit.mac+' as '+hit.observed+
        ' in a fresh LOG snapshot. Awaiting emitter END for scoring.';
      if(t.emission==='ok')finishAutoMatch(t);
      matchedLog(new Date(hit.finishedAt).toLocaleTimeString()+
        '  '+t.id+'  '+hit.observed+'  MAC '+hit.mac+
        '  '+hit.rssi+' dBm  '+(t.emission==='ok'?
          core.status(t,catalog)+' ('+(core.pendingReason(t,catalog)||'scored')+')':
          'WAIT END'));
      change=true;
      break; // one receiver sighting belongs to one active test
    }
    if(change){
      redrawTrials();selectTrial();
      notice('SquachWatch automatically logged a fresh exact-MAC detection. '+
        'Only confirmed emitter END can complete the score.',false);
    }
  }
  function onWatchLine(line) {
    var d=devices.receiver;
    var snapshot=d.parser.read(line,Date.now());
    if(snapshot)onWatchSnapshot(snapshot);
  }
  function stopWatchPolling() {
    var d=devices.receiver;
    if(d.pollTimer!==null){clearInterval(d.pollTimer);d.pollTimer=null;}
    d.parser.cancel();
    if(d.pending){
      var pending=d.pending;d.pending=null;
      clearTimeout(pending.timeout);
      pending.resolve(null);
    }
    d.lastSnapshot=null;
    d.preRun=null;
    autoStatus('Receiver LOG polling stopped. Reconnect SquachWatch to resume automatic matching.');
  }
  function sendReceiver(command){
    // A single write queue is shared by 1.5s LOG polling and desktop controls.
    // This prevents a user control command from colliding with a pending LOG write.
    var d=devices.receiver,port=d.port;
    var task=receiverWrites.then(async function(){
      if(!port||d.port!==port||!port.writable)return false;
      var writer=null;
      try{
        writer=port.writable.getWriter();
        await writer.write(new TextEncoder().encode(command+'\n'));
        if(command!=='LOG')append('receiver','> '+command+'\n');
        return true;
      }catch(e){
        autoStatus('SquachWatch command failed: '+e.message);
        return false;
      }finally{if(writer)try{writer.releaseLock();}catch(_){}}
    });
    receiverWrites=task.then(function(){},function(){});
    return task;
  }
  async function requestWatchSnapshot() {
    var d=devices.receiver;
    if(!d.port||!d.port.writable)return null;
    if(d.pending)return d.pending.promise;
    var requestedAt=Date.now();
    if(!d.parser.start(requestedAt))return null;
    var pending={promise:null,resolve:null,timeout:null};
    pending.promise=new Promise(function(resolve){pending.resolve=resolve;});
    d.pending=pending;
    pending.timeout=setTimeout(function(){
      if(d.pending===pending){
        d.pending=null;d.parser.cancel();
        autoStatus('LOG response timed out. Confirm SquachWatch serial baud and connection.');
        pending.resolve(null);
      }
    },3800);
    try{
      if(!(await sendReceiver('LOG')))throw new Error('USB LOG write did not complete');
    }catch(e){
      if(d.pending===pending){
        d.pending=null;d.parser.cancel();clearTimeout(pending.timeout);
        pending.resolve(null);
      }
      autoStatus('Cannot request LOG from SquachWatch: '+e.message);
    }
    return pending.promise;
  }
  function startWatchPolling() {
    var d=devices.receiver;
    stopWatchPolling();
    // Explicitly connected port is the only trigger. No radio commands here.
    autoStatus('Connected. Requesting the initial SquachWatch LOG baseline…');
    void requestWatchSnapshot();
    d.pollTimer=setInterval(function(){
      if(d.port && !d.pending)void requestWatchSnapshot();
    },1500);
  }

  // All RF actions remain locally user initiated and are strictly serial.
  function clearBatchTimers(){
    if(batch.timer!==null){clearTimeout(batch.timer);batch.timer=null;}
    if(batch.endTimer!==null){clearTimeout(batch.endTimer);batch.endTimer=null;}
  }
  function endBatch(message,sendStop){
    if(!batch.active)return;
    clearBatchTimers();
    batch.active=false;batch.current=null;batch.message=message;
    runToken++;devices.receiver.preRun=null;
    cancelRunAck();transmitting=false;
    updateButtons();
    if(sendStop&&devices.emitter.port)void sendEmitter('STOP');
    runProgress(message);
  }
  function startBatchStep(){
    if(!batch.active)return;
    if(!devices.emitter.port||!devices.receiver.port){
      endBatch('Automation stopped: device disconnected.',true);return;
    }
    if(batch.index>=batch.queue.length){
      endBatch('Automation complete: '+batch.completed+' cases; '+
        batch.failures+' transmitter failures; '+batch.unverified+
        ' unverified recoveries. Review receiver evidence and export CSV.',false);
      return;
    }
    var c=batch.queue[batch.index];
    batch.current={id:c.id,seq:null,begun:false,finished:false};
    ui.caseSelect.value=c.id;ui.duration.value=String(batch.duration);
    chooseCase();
    batch.message='CASE '+(batch.index+1)+' / '+batch.queue.length+
      '\n'+core.signalName(c)+'\n'+c.id+
      '\nSquachWatch expected: '+c.expected+'\nPreparing receiver baseline…';
    updateBatchUi();
    void run(true);
  }
  function advanceBatch(reason,failed,requestStop){
    if(!batch.active||!batch.current)return;
    var last=batch.current;
    if(last.finished)return;
    last.finished=true;
    if(batch.endTimer!==null){clearTimeout(batch.endTimer);batch.endTimer=null;}
    if(failed)batch.failures++;
    if(reason==='no receiver baseline'||failed)batch.unverified++;
    batch.completed++;batch.index++;batch.current=null;
    // Missing BEGIN/END is not evidence that RF was never on. Request an
    // explicit STOP and allow a longer cool-off before moving on.
    if(requestStop && devices.emitter.port)void sendEmitter('STOP');
    transmitting=false;cancelRunAck();devices.receiver.preRun=null;
    batch.message='CASE '+batch.completed+' / '+batch.queue.length+
      '\n'+core.signalName(catalog[last.id])+'\n'+last.id+
      '\n'+reason+'; continuing after safe cooldown. No receiver result inferred.';
    updateButtons();
    batch.timer=setTimeout(function(){
      batch.timer=null;
      if(!batch.active)return;
      if(!devices.emitter.port||!devices.receiver.port){
        endBatch('Automation stopped: a USB device disconnected.',false);return;
      }
      // A stale in-flight preflight is invalidated before starting next.
      runToken++;startingRun=false;awaitingBegin=false;transmitting=false;
      startBatchStep();
    },Math.max(batch.gap,requestStop?8:3)*1000);
  }
  function onBatchTruth(event){
    if(!batch.active||!batch.current||event.id!==batch.current.id)return;
    var current=batch.current;
    if(event.event==='BEGIN'){
      if(current.begun)return;
      current.begun=true;current.seq=event.seq;
      batch.message='CASE '+(batch.index+1)+' / '+batch.queue.length+
        '\n'+core.signalName(catalog[event.id])+'\n'+event.id+
        '\nTRUTH BEGIN confirmed. Waiting for TRUTH END.';
      if(batch.endTimer!==null)clearTimeout(batch.endTimer);
      batch.endTimer=setTimeout(function(){
        if(batch.active&&batch.current===current&&!current.finished)
          advanceBatch('No transmitter TRUTH END (timed out)',true,true);
      },(batch.duration+20)*1000);
      updateBatchUi();return;
    }
    if(event.event!=='END'&&event.event!=='ERROR')return;
    if(current.finished||(current.begun&&current.seq!==event.seq)||
       (!current.begun&&event.event!=='ERROR'))return;
    advanceBatch(event.event==='END'&&event.status==='ok'?
      'Transmitter END ok':'Transmitter '+event.event+' '+(event.status||'unknown'),
      !(event.event==='END'&&event.status==='ok'),false);
  }
  function startBatch(){
    if(batch.active||startingRun||awaitingBegin||transmitting||
       !devices.emitter.port||!devices.receiver.port||!ui.authorization.checked)return;
    var duration=Number(ui.batchDuration.value),gap=Number(ui.batchGap.value);
    if(!Number.isInteger(duration)||duration<3||duration>30||
       !Number.isInteger(gap)||gap<3||gap>30)return;
    var queue=core.batchPlan(cases,ui.batchScope.value);
    if(!queue.length)return;
    if(!devices.receiver.lastSnapshot||
       Date.now()-devices.receiver.lastSnapshot.finishedAt>5000){
      batch.message='Receiver LOG not ready: click Poll LOG now and wait for a complete snapshot.';
      updateBatchUi();return;
    }
    if(queue.some(function(c){return c.id.startsWith('OUI-');}) &&
       !window.confirm('This queue contains synthetic manufacturer Wi-Fi MAC/OUI tests.'+
         ' These are NOT genuine drones or Remote ID. Confirm controlled, authorized RF testing?'))return;
    if(!window.confirm('Run '+queue.length+' signals sequentially while attended?'+
      ' STOP cancels the remaining tests. Export the CSV to save results.'))return;
    clearBatchTimers();
    batch.queue=queue;batch.index=0;batch.completed=0;batch.failures=0;batch.unverified=0;
    batch.duration=duration;batch.gap=gap;batch.active=true;batch.current=null;
    startBatchStep();
  }
  function recordEmitter(line){
    var event=core.parseTruth(line);
    if(!event || !catalog[event.id])return;
    desktopEvent('truth',{event:event.event,id:event.id,expected:event.expected,
      name:core.signalName(catalog[event.id]),status:event.status,seq:event.seq});
    if(event.event==='BEGIN'){
      if(trials.some(function(t){return t.seq===event.seq&&t.id===event.id&&
        t._epoch===epoch;}))return;
      var t=core.makeTrial(event,{
        runId:uniqueRunId(),receiver:ui.receiverBoard.value,
        receiverSha:ui.receiverSha.value,emitterSha:ui.emitterSha.value
      },catalog);
      cancelRunAck();
      transmitting=true;
      t._epoch=epoch;
      t._receiverSession=devices.receiver.port?devices.receiver.session:-1;
      t._mac=core.normalizeMac(event.mac);
      t._startedAt=Date.now();
      var receiver=devices.receiver;
      // UI-controlled runs have a dedicated LOG preflight baseline.
      // Externally started tests can use a recently completed poll.
      var ready=receiver.preRun;
      // A UI RUN with an explicitly missing baseline must NOT fall back to
      // an unrelated older snapshot, which could fabricate a match.
      t._baseline=(ready && ready.id===event.id &&
        ready.expiresAt>=t._startedAt ? ready.snapshot :
        !ready && receiver.lastSnapshot &&
        receiver.lastSnapshot.finishedAt<=t._startedAt &&
        t._startedAt-receiver.lastSnapshot.finishedAt<=5000 ?
        receiver.lastSnapshot : null);
      receiver.preRun=null;
      runProgress('TRANSMITTING confirmed by SquachEmit TRUTH BEGIN · '+event.id);
      trials.push(t);
      selectedTrial=trials.length-1;
      redrawTrials();selectTrial();
      notice('Emitter BEGIN '+event.id+' ('+(t._mac||'no MAC')+'). '+
        (t._baseline?'Automatic SquachWatch LOG correlation active.':
        'No fresh pre-test LOG baseline; verify any receiver detection manually.'),false);
    }else{
      var found=false;
      for(var i=trials.length-1;i>=0;i--){
        if(trials[i]._epoch!==epoch)continue;
        if(core.applyEnd(trials[i],event)){
          found=true;
          trials[i]._endedAt=Date.now();
          if(trials[i].emission==='failed')trials[i].window='incomplete';
          else if(finishAutoMatch(trials[i])){
            matchedLog(new Date().toLocaleTimeString()+'  '+trials[i].id+
              '  FINAL '+core.status(trials[i],catalog)+
              '  receiver '+trials[i].observed+' · emitter END ok');
          }
          break;
        }
      }
      if(event.event==='END'||event.event==='ERROR'){
        cancelRunAck();transmitting=false;
        runProgress('Emitter '+event.event+' ('+(event.status||'unknown')+
          '). '+(found?'Trial updated.':'No matching BEGIN record.'));
      }
      if(!found && event.event==='ERROR'){
        // Error without BEGIN is an actual recorded emission failure,
        // not an invented received detection.
        var row=core.makeTrial({event:'BEGIN',id:event.id,seq:event.seq,ms:event.ms},
          {runId:uniqueRunId(),receiver:ui.receiverBoard.value,
           receiverSha:ui.receiverSha.value,emitterSha:ui.emitterSha.value},catalog);
        row.emission='failed';row.window='incomplete';row._epoch=epoch;
        row.notes='Emitter reported ERROR before TRUTH BEGIN; exclude from RF accuracy.';
        trials.push(row);
        selectedTrial=trials.length-1;
      }
      redrawTrials();selectTrial();
      notice('Emitter '+event.event+' ('+(event.status||'unspecified')+
        '). '+(trials[selectedTrial]&&trials[selectedTrial]._autoHit ?
        'SquachWatch exact-MAC detection also recorded.' :
        'Waiting for independent SquachWatch LOG detection; no automatic NONE.'),false);
    }
    onBatchTruth(event);
  }
  function consume(role,chunk){
    var d=devices[role];
    d.buffer+=chunk;
    if(d.buffer.length>65000)d.buffer=d.buffer.slice(-40000);
    var lines=d.buffer.split(/\r?\n/);
    d.buffer=lines.pop();
    lines.forEach(function(line){
      append(role,line+'\n');
      if(role==='emitter')recordEmitter(line.trim());
      else onWatchLine(line.trim());
    });
  }
  async function readPort(role,port){
    var d=devices[role];
    var decoder=new TextDecoder('utf-8');
    try{
      while(port.readable){
        var rd=port.readable.getReader();
        d.reader=rd;
        try{
          for(;;){
            var result=await rd.read();
            if(result.done)break;
            if(result.value)consume(role,decoder.decode(result.value,{stream:true}));
          }
        }finally{
          try{rd.releaseLock();}catch(_){}
          if(d.reader===rd)d.reader=null;
        }
        if(d.port!==port)break;
      }
    }catch(e){append(role,'\n[serial read stopped: '+e.message+']\n');}
    finally{
      if(d.port===port){
        if(batch.active)endBatch('Automation stopped: '+role+' serial read closed.',role==='receiver');
        if(role==='receiver')stopWatchPolling();
        d.port=null;
        try{await port.close();}catch(_){}
        updateButtons();
      }
    }
  }
  async function connect(role){
    if(opening[role]||devices[role].port)return;
    if(!window.isSecureContext || !navigator.serial){
      notice('Web Serial requires HTTPS/localhost and desktop Chrome or Edge.',true);return;
    }
    opening[role]=true;updateButtons();
    var port=null;
    try{
      port=await navigator.serial.requestPort();
      var baud=role==='emitter'?115200:Number(ui.receiverBaud.value);
      if(!Number.isSafeInteger(baud)||baud<300||baud>4000000)
        throw new Error('Unsupported baud rate.');
      await port.open({baudRate:baud});
      var d=devices[role];d.port=port;d.baud=baud;d.buffer='';
      if(role==='emitter'){epoch++;transmitting=false;cancelRunAck();}
      else d.session++;
      opening[role]=false;updateButtons();
      append(role,'\n[connected at '+baud+' baud; '+
        (role==='receiver'?'automatic LOG polling enabled':'no commands sent')+']\n');
      d.reading=readPort(role,port);
      if(role==='receiver')startWatchPolling();
    }catch(e){
      opening[role]=false;
      if(port)try{await port.close();}catch(_){}
      notice(role+' connection failed: '+e.message,true);
      updateButtons();
    }
  }
  async function disconnect(role){
    var d=devices[role],p=d.port;
    if(!p)return;
    d.port=null;
    if(batch.active)endBatch('Automation stopped: '+role+' disconnected.',role==='receiver');
    if(role==='receiver')stopWatchPolling();
    if(role==='emitter'){
      runToken++;devices.receiver.preRun=null;
      transmitting=false;cancelRunAck();
      runProgress('Transmitter disconnected. Connect again to run a case.');
    }
    updateButtons();
    try{if(d.reader)await d.reader.cancel();}catch(_){}
    try{if(d.reading)await d.reading;}catch(_){}
    try{await p.close();}catch(_){}
    d.reader=null;d.reading=null;
    append(role,'\n[disconnected]\n');updateButtons();
  }
  function sendEmitter(command){
    // Serialize STOP behind a pending RUN to avoid colliding writable locks.
    var port=devices.emitter.port;
    var pending=emitterWrites.then(async function(){
      if(!port||devices.emitter.port!==port||!port.writable){
        notice('Emitter disconnected; command not sent.',true);return false;
      }
      var writer=null;
      try{
        writer=port.writable.getWriter();
        await writer.write(new TextEncoder().encode(command+'\n'));
        append('emitter','> '+command+'\n');
        return true;
      }catch(e){notice('Serial write failed: '+e.message,true);return false;}
      finally{if(writer)try{writer.releaseLock();}catch(_){}}
    });
    emitterWrites=pending.then(function(){},function(){});
    return pending;
  }
  async function run(fromBatch){
    if(batch.active&&!fromBatch)return;
    if(fromBatch&&(!batch.active||!batch.current||batch.current.id!==ui.caseSelect.value))return;
    var c=catalog[ui.caseSelect.value],duration=Number(ui.duration.value);
    if(startingRun||awaitingBegin||transmitting||!c||!ui.authorization.checked||
       !Number.isInteger(duration)||duration<3||duration>30||!devices.emitter.port)return;
    startingRun=true;
    var token=++runToken;
    updateButtons();
    runProgress('Preparing to transmit '+c.id+'…');
    try{
      var receiver=devices.receiver,baseline=null;
      if(receiver.port){
        var now=Date.now();
        // Reuse a complete fresh receiver poll if available; waiting for a
        // second LOG response was causing unnecessary delay and missed runs.
        if(receiver.lastSnapshot && receiver.lastSnapshot.finishedAt<=now &&
           now-receiver.lastSnapshot.finishedAt<=1800){
          baseline=receiver.lastSnapshot;
          runProgress('Using fresh SquachWatch baseline; sending RUN…');
        }else{
          runProgress('Refreshing SquachWatch LOG baseline (max 1.2 seconds)…');
          var timer;
          try{
            baseline=await Promise.race([
              requestWatchSnapshot(),
              new Promise(function(resolve){
                timer=setTimeout(function(){resolve(null);},fromBatch?4200:1200);
              })
            ]);
          }finally{if(timer)clearTimeout(timer);}
        }
        if(token!==runToken || !devices.emitter.port)return;
        // An intermittent LOG timeout cannot halt a 172-signal suite.
        // The individual case remains UNTESTED unless independent receiver
        // evidence is subsequently captured with a complete baseline.
        if(fromBatch&&!baseline){
          batch.unverified++;
          batch.message='Receiver LOG temporarily unavailable for '+c.id+
            '. Sending test, but automatic scoring is disabled for this case.';
          updateBatchUi();
        }
        // Explicit null means auto-matching is disabled for this run. Never
        // prevent a valid user-requested RUN just because receiver LOG is late.
        receiver.preRun={id:c.id,snapshot:baseline,expiresAt:Date.now()+5000};
        if(!baseline){
          autoStatus('Receiver LOG baseline unavailable: RUN will be sent, but '+
            'its detection cannot be auto-scored without a fresh baseline.');
        }
      }else receiver.preRun=null;
      if(token!==runToken || !devices.emitter.port)return;
      runProgress('Sending RUN '+c.id+' '+duration+' to SquachEmit…');
      // Only TRUTH BEGIN establishes actual emitter acceptance, not a click.
      awaitingBegin=true;updateButtons();
      var sent=await sendEmitter('RUN '+c.id+' '+duration);
      if(token!==runToken)return;
      if(!sent){
        awaitingBegin=false;receiver.preRun=null;
        runProgress('RUN write failed. Check the emitter connection.');
        if(fromBatch)advanceBatch('RUN serial write failed',true,true);
        return;
      }
      // TRUTH BEGIN may already arrive while awaiting the serial write.
      if(awaitingBegin){
        runProgress('RUN sent to emitter. Waiting for TRUTH BEGIN confirmation…');
        runAwaitTimer=setTimeout(function(){
          if(token!==runToken || !awaitingBegin)return;
          awaitingBegin=false;
          runAwaitTimer=null;
          runProgress('RUN was sent but no TRUTH BEGIN arrived within 7 seconds. '+
            'Check SquachEmit serial log or try again; no transmission was confirmed.');
          if(fromBatch)advanceBatch('No TRUTH BEGIN within seven seconds',true,true);
          updateButtons();
        },7000);
      }
      notice('RUN command sent. '+(baseline?'Automatic receiver detection is armed.':
        'Receiver baseline unavailable: use manual detection verification.'),!baseline&&!!receiver.port);
    }finally{startingRun=false;updateButtons();}
  }
  function saveObservation(){
    var old=trials[selectedTrial];if(!old)return;
    var result=Object.assign({},old,{
      // Preserve the unique per-trial run ID rather than replacing it with
      // the session prefix when saving an observation.
      run_id:old.run_id,
      receiver:ui.receiverBoard.value,
      receiver_sha:ui.receiverSha.value.trim()||old.receiver_sha,
      emitter_sha:ui.emitterSha.value.trim()||old.emitter_sha,
      emission:old.seq==null?(ui.emissionStatus.value||old.emission):old.emission,
      window:ui.listenWindow.value||'',
      observed:ui.observed.value,latency_ms:ui.latencyMs.value.trim(),
      notes:ui.evidenceNotes.value.trim()
    });
    if(result.window==='complete'&&result.emission==='ok'&&!result.observed){
      text('observationNotice','Choose a receiver classification or NONE for a complete listening window.');return;
    }
    if(result.latency_ms){
      var ms=Number(result.latency_ms);
      if(!isFinite(ms)||ms<0||core.status(result,catalog)!=='TP'){
        text('observationNotice','Latency must be a nonnegative number on a true-positive trial with both firmware SHAs.');return;
      }
    }
    var key=trialKey(result);
    if(trials.some(function(t,i){return i!==selectedTrial&&trialKey(t)===key;})){
      text('observationNotice','Duplicate run ID / receiver / case: this CSV would be rejected by score_trials.py.');return;
    }
    result._manual=true; // future automated polling cannot overwrite a reviewed record
    trials[selectedTrial]=result;
    redrawTrials();selectTrial();
    var status=core.status(result,catalog);
    text('observationNotice',status+' — '+(status==='UNTESTED'?'Needs complete verified window, observation, successful emission and both SHAs.':status==='EXCLUDED'?'Excluded from detection accuracy.':'Recorded from your explicit receiver observation; not inferred from transmitter log.'));
  }
  function download(name,contents){
    var blob=new Blob([contents],{type:'text/csv;charset=utf-8'});
    var url=URL.createObjectURL(blob);
    var anchor=document.createElement('a');anchor.href=url;anchor.download=name;
    document.body.appendChild(anchor);anchor.click();anchor.remove();
    setTimeout(function(){URL.revokeObjectURL(url);},1000);
  }
  async function loadCases(){
    try{
      var response=await fetch('./cases.json',{cache:'no-store'});
      if(!response.ok)throw new Error('HTTP '+response.status);
      var data=await response.json();
      if(!Array.isArray(data)||data.length!==172)
        throw new Error('Expected exactly 172 current firmware catalog cases');
      var ids=new Set();
      data.forEach(function(c){
        if(!c||typeof c.id!=='string'||ids.has(c.id)||
           !/^[A-Z0-9][A-Z0-9_-]{1,100}$/.test(c.id)||!c.expected)
          throw new Error('Invalid or duplicated case in catalog');
        core.signalName(c); // fail closed if device name is missing
        ids.add(c.id);catalog[c.id]=c;
      });
      cases=data;
      cases.sort(function(a,b){return a.id.localeCompare(b.id);});
      ui.caseSelect.replaceChildren(option('','Choose a test case…'));
      cases.forEach(function(c){
        ui.caseSelect.appendChild(option(c.id,core.signalName(c)+'  ['+c.id+']'));
      });
      var values=Array.from(new Set(cases.map(function(c){return c.expected;})
        .concat(['NONE','ALPR','CAMERA','DEAUTH','EVILTWIN','RING','AXON']))).sort();
      ui.observed.replaceChildren(option('','Not evaluated'));
      values.forEach(function(s){ui.observed.appendChild(option(s,s));});
      chooseCase();redrawTrials();updateBatchUi();
      notice('Loaded the 172-case catalog. No hardware is connected or transmitting.',false);
    }catch(e){
      notice('Catalog could not load: '+e.message+'. Run the published workbench site or the build-installer static output.',true);
      ui.caseSelect.replaceChildren(option('','Case catalog unavailable'));
    }
  }
  function applySessionShas(){
    var receiverSha=ui.receiverSha.value.trim();
    var emitterSha=ui.emitterSha.value.trim();
    if(!validSha(receiverSha)||!validSha(emitterSha)){
      text('shaNotice','Enter real 7–40-character hexadecimal commit SHAs for both boards first.');
      return;
    }
    // Never rewrite existing valid firmware identities or another board.
    // CSV imports and previous connections may belong to OTHER firmware
    // builds; require an explicit user confirmation before backfilling them.
    var candidates=trials.filter(function(t){
      return t.receiver===ui.receiverBoard.value &&
        (!validSha(t.receiver_sha)||!validSha(t.emitter_sha));
    });
    var historical=candidates.filter(function(t){
      return t.seq==null||t._epoch!==epoch;
    });
    if(historical.length && !window.confirm(
      'You have '+historical.length+' imported/earlier-connection trials for '+
      ui.receiverBoard.value+'. Apply these firmware SHAs to them ONLY if '+
      'both devices were running exactly these builds for every affected '+
      'trial? Existing valid SHAs are never overwritten.'))return;
    candidates.forEach(function(t){
      if(!validSha(t.receiver_sha))t.receiver_sha=receiverSha;
      if(!validSha(t.emitter_sha))t.emitter_sha=emitterSha;
    });
    redrawTrials();selectTrial();
    text('shaNotice',candidates.length+' trial(s) for '+ui.receiverBoard.value+
      ' updated with the specified firmware identities. Imported/historical '+
      'trials were only modified with explicit confirmation.');
    notice('Verified firmware SHAs recorded for '+candidates.length+
      ' trial(s). Receiver results were preserved; no detections were invented.',false);
  }
  ui.applyShas.addEventListener('click',applySessionShas);
  ['receiverSha','emitterSha'].forEach(function(id){
    ui[id].addEventListener('input',function(){
      updateButtons();
      if(validSha(ui.receiverSha.value)&&validSha(ui.emitterSha.value)){
        text('shaNotice','Both commit SHAs look valid. Click Apply to score '+
          'current or imported trials; new trials capture them automatically.');
      }else{
        text('shaNotice','Both firmware commit SHAs are required for scoring. '+
          'Detected cases are preserved but marked UNTESTED until then.');
      }
    });
  });
  ui.caseSelect.addEventListener('change',chooseCase);
  ui.authorization.addEventListener('change',updateButtons);
  ui.duration.addEventListener('input',updateButtons);
  ui.emitConnect.addEventListener('click',function(){void connect('emitter');});
  ui.recvConnect.addEventListener('click',function(){void connect('receiver');});
  ui.emitDisconnect.addEventListener('click',function(){void disconnect('emitter');});
  ui.recvDisconnect.addEventListener('click',function(){void disconnect('receiver');});
  ui.runCase.addEventListener('click',function(){void run(false);});
  ui.batchStart.addEventListener('click',startBatch);
  ui.batchStop.addEventListener('click',function(){
    endBatch('Automation stopped by operator after '+batch.completed+' completed cases.',true);
  });
  ['batchScope','batchDuration','batchGap'].forEach(function(id){
    ui[id].addEventListener('change',updateBatchUi);
    ui[id].addEventListener('input',updateBatchUi);
  });
  ui.stopCase.addEventListener('click',function(){
    if(batch.active){endBatch('Automation stopped by operator. No additional signals will run.',true);return;}
    runToken++;devices.receiver.preRun=null;
    cancelRunAck();transmitting=false;
    runProgress('STOP requested. Waiting for emitter to stop.');
    updateButtons();
    void sendEmitter('STOP');
  });
  ui.pollNow.addEventListener('click',function(){void requestWatchSnapshot();});
  ui.trialSelect.addEventListener('change',selectTrial);
  ui.saveObservation.addEventListener('click',saveObservation);
  ui.copyReceiver.addEventListener('click',function(){
    if(!navigator.clipboard){notice('Clipboard unavailable.',true);return;}
    navigator.clipboard.writeText(devices.receiver.log).then(function(){
      notice('Receiver raw log copied. It remains unclassified until manually verified.',false);
    }).catch(function(e){notice('Could not copy: '+e.message,true);});
  });
  ui.exportCsv.addEventListener('click',function(){
    download('squachemit-trials-'+new Date().toISOString().slice(0,10)+'.csv',
      core.exportCsv(trials));
    notice('Exported '+trials.length+' trials. Run tools/score_trials.py on the CSV to reproduce statistics.',false);
  });
  ui.importCsv.addEventListener('click',function(){ui.csvFile.click();});
  ui.csvFile.addEventListener('change',async function(){
    var f=ui.csvFile.files[0];
    ui.csvFile.value='';
    if(!f)return;
    try{
      if(f.size>4000000)throw new Error('File exceeds 4 MB.');
      var imported=core.parseCsv(await f.text());
      var seen=new Set(trials.map(trialKey));
      var labels=new Set(cases.map(function(c){return c.expected;})
        .concat(['NONE','ALPR','CAMERA','DEAUTH','EVILTWIN','RING','AXON']));
      imported.forEach(function(row){
        if(!catalog[row.id])throw new Error('Unknown case: '+row.id);
        if(!['CYD-2.8','CYD-3.2'].includes(row.receiver))
          throw new Error('Unknown receiver board');
        if(!row.run_id)throw new Error('A run ID is required');
        if(row.observed&&!labels.has(row.observed))
          throw new Error('Unknown observed classification: '+row.observed);
        if(row.latency_ms&&core.status(row,catalog)!=='TP')
          throw new Error('Latency is allowed only on complete true-positive trials');
        var key=trialKey(row);
        if(seen.has(key))throw new Error('Duplicate (run_id, receiver, case): '+row.id);
        seen.add(key);
      });
      if(imported.length+trials.length>5000)throw new Error('Maximum 5,000 trials.');
      trials.push.apply(trials,imported);
      selectedTrial=-1;redrawTrials();selectTrial();
      notice('Imported '+imported.length+' trial records. Unverified or incomplete data remains unscored.',false);
    }catch(e){notice('CSV import failed: '+e.message,true);}
  });
  ui.clearTrials.addEventListener('click',function(){
    if(!trials.length||batch.active)return;
    if(!window.confirm('Permanently clear all '+trials.length+' local trials? Export CSV first.'))return;
    trials=[];selectedTrial=-1;redrawTrials();selectTrial();
    notice('Local trial ledger cleared.',false);
  });
  window.addEventListener('beforeunload',function(){
    if(batch.active)endBatch('Automation stopped: browser is unloading.',true);
    stopWatchPolling();
    Object.keys(devices).forEach(function(role){var r=devices[role].reader;if(r)r.cancel().catch(function(){});});
  });

  if(desktopMode){
    // Narrow public API for the local Windows control rail. No raw serial port
    // objects, no automatic transmission and no unrestricted firmware commands.
    function desktopSend(role,cmd){
      var value=String(cmd||'').trim().toUpperCase();
      if(role==='receiver'){
        if(!/^(LOG|RADIO|RADIO SCAN|MEM|WIFI|BLACKBOX|ROT|SCAN (?:AUTO|ACTIVE|PASSIVE)|CLOCK [01] [0-2] [0-6]|BG (?:[0-9]|1[0-9]))$/.test(value))
          return Promise.resolve(false);
        if(batch.active&&!/^(?:RADIO|MEM)$/.test(value))return Promise.resolve(false);
        if(value==='LOG')return requestWatchSnapshot().then(function(r){return !!r;});
        return sendReceiver(value);
      }
      if(role==='emitter'){
        if(!/^(STATUS|IDENTITY|LIST|HELP|COLORS|STOP)$/.test(value))
          return Promise.resolve(false);
        if(batch.active&&value!=='STATUS'&&value!=='STOP')return Promise.resolve(false);
        if(value==='STOP'){ui.stopCase.click();return Promise.resolve(true);}
        return sendEmitter(value);
      }
      return Promise.resolve(false);
    }
    window.SQEWorkbenchDesktop=Object.freeze({
      connect:function(role){return ['receiver','emitter'].includes(role)?connect(role):Promise.resolve();},
      disconnect:function(role){return ['receiver','emitter'].includes(role)?disconnect(role):Promise.resolve();},
      send:desktopSend,
      runSelected:function(){return run(false);},
      startBatch:function(){ui.batchStart.click();},
      stopBatch:function(){ui.batchStop.click();},
      exportCsv:function(){ui.exportCsv.click();},
      getState:function(){
        return {
          emitterConnected:!!devices.emitter.port,
          receiverConnected:!!devices.receiver.port,
          receiverBaud:devices.receiver.baud,
          batchActive:batch.active,transmitting:transmitting,
          selectedCase:catalog[ui.caseSelect.value] ?
            core.signalName(catalog[ui.caseSelect.value]):'No signal selected',
          latestSnapshotRows:devices.receiver.lastSnapshot ?
            devices.receiver.lastSnapshot.rows.length:null,
          catalogCount:cases.length
        };
      }
    });
  }
  redrawTrials();updateButtons();void loadCases();
})();
