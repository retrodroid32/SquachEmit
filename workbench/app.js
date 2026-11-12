/* SquachEmit Detection Workbench — browser-side observation UI.
 * Hardware is touched ONLY after a user clicks Connect/Run/STOP.
 * The receiver console is unstructured evidence, never auto-classification.
 */
(function () {
  'use strict';
  var core=window.SQEWorkbenchCore;
  if(!core) throw new Error('Workbench core.js missing');
  function byId(id){var el=document.getElementById(id);if(!el)throw new Error('Missing '+id);return el;}
  var ids=['securityNotice','scoredCount','pendingCount','recall','precision','latency',
    'runId','receiverBoard','receiverSha','emitterSha','emitState','emitConnect',
    'emitDisconnect','caseSelect','caseDetails','duration','authorization','runCase',
    'stopCase','emitLog','recvState','recvConnect','recvDisconnect','receiverBaud',
    'recvLog','copyReceiver','trialSelect','trialDetails','emissionStatus','listenWindow',
    'observed','latencyMs','evidenceNotes','saveObservation','observationNotice',
    'trialRows','importCsv','csvFile','exportCsv','clearTrials'];
  var ui={};
  ids.forEach(function(id){ui[id]=byId(id);});
  var catalog={},cases=[],trials=[],selectedTrial=-1;
  var devices={emitter:{port:null,reader:null,reading:null,buffer:'',log:'',baud:115200},
               receiver:{port:null,reader:null,reading:null,buffer:'',log:'',baud:2000000}};
  var opening={emitter:false,receiver:false};
  var epoch=0;
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
  }
  function option(value,label){var o=document.createElement('option');o.value=value;o.textContent=label;return o;}
  function formatPct(x){return x===null?'—':(x*100).toFixed(1)+'%';}
  function validSha(str){return /^[a-f0-9]{7,40}$/i.test(String(str||'').trim());}
  function chooseCase(){
    var c=catalog[ui.caseSelect.value];
    if(!c){text('caseDetails','Select a catalog case to inspect its expected classification.');}
    else{
      text('caseDetails',c.id+' · '+c.carrier+
        '\nEXPECTED: '+c.expected+
        '\n'+c.description+
        (c.id.startsWith('OUI-')?
        '\nCAUTION: selected case transmits a registered vendor MAC prefix; this is NOT proof of a genuine device or Remote ID.':''));
    }
    updateButtons();
  }
  function refreshMetric(){
    var m=core.score(trials,catalog);
    text('scoredCount',m.scored);
    text('pendingCount',m.count.UNTESTED+' untested · '+m.count.EXCLUDED+' excluded');
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
      addCell(row,t.id);addCell(row,(catalog[t.id]||{}).expected||'Unknown');
      addCell(row,t.observed||'—');addCell(row,t.emission||'—');addCell(row,t.window||'—');
      var td=document.createElement('td'),badge=document.createElement('span');
      badge.className='result '+status;badge.textContent=status;td.appendChild(badge);row.appendChild(td);
      addCell(row,t.latency_ms?String(t.latency_ms)+' ms':'—');
      ui.trialRows.appendChild(row);
    });
    refreshMetric();
    ui.trialSelect.replaceChildren(option('','Select a trial…'));
    trials.forEach(function(t,i){
      ui.trialSelect.appendChild(option(String(i),String(i+1)+' · '+t.id+
        ' · '+core.status(t,catalog)));
    });
    if(selectedTrial>=0 && selectedTrial<trials.length)ui.trialSelect.value=String(selectedTrial);
    updateButtons();
  }
  function selectTrial(){
    var v=ui.trialSelect.value;selectedTrial=v===''?-1:Number(v);
    var t=trials[selectedTrial];
    if(!t){
      text('trialDetails','Select a TRUTH BEGIN trial or import existing CSV records.');
      ui.emissionStatus.value='';ui.listenWindow.value='';ui.observed.value='';
      ui.latencyMs.value='';ui.evidenceNotes.value='';
    }else{
      text('trialDetails',t.id+' · expected '+((catalog[t.id]||{}).expected||'UNKNOWN')+
        '\nEmitter sequence: '+(t.seq==null?'Imported':t.seq)+
        '\nEmitter result: '+(t.emission||'pending')+
        '\nYou must independently confirm the SquachWatch event (or NONE).');
      ui.emissionStatus.value=t.emission==='ok'||t.emission==='failed'?t.emission:'';
      ui.listenWindow.value=t.window||'';
      ui.observed.value=t.observed||'';
      ui.latencyMs.value=t.latency_ms||'';
      ui.evidenceNotes.value=t.notes||'';
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
    var duration=Number(ui.duration.value);
    ui.runCase.disabled=!connected||!catalog[ui.caseSelect.value]||
      !ui.authorization.checked||!Number.isInteger(duration)||duration<3||duration>30;
    ui.stopCase.disabled=!connected;
    ui.saveObservation.disabled=!(selectedTrial>=0 && trials[selectedTrial]);
  }
  function recordEmitter(line){
    var event=core.parseTruth(line);
    if(!event || !catalog[event.id])return;
    if(event.event==='BEGIN'){
      if(trials.some(function(t){return t.seq===event.seq&&t.id===event.id&&
        t._epoch===epoch;}))return;
      var t=core.makeTrial(event,{
        runId:ui.runId.value,receiver:ui.receiverBoard.value,
        receiverSha:ui.receiverSha.value,emitterSha:ui.emitterSha.value
      },catalog);
      t._epoch=epoch;
      trials.push(t);
      selectedTrial=trials.length-1;
      redrawTrials();selectTrial();
      notice('Emitter reported BEGIN for '+event.id+'. Verify the actual receiver evidence separately.',false);
    }else{
      var found=false;
      for(var i=trials.length-1;i>=0;i--){
        if(trials[i]._epoch!==epoch)continue;
        if(core.applyEnd(trials[i],event)){found=true;break;}
      }
      if(!found && event.event==='ERROR'){
        // Error without BEGIN is an actual recorded emission failure,
        // not an invented received detection.
        var row=core.makeTrial({event:'BEGIN',id:event.id,seq:event.seq,ms:event.ms},
          {runId:ui.runId.value,receiver:ui.receiverBoard.value,
           receiverSha:ui.receiverSha.value,emitterSha:ui.emitterSha.value},catalog);
        row.emission='failed';row.window='incomplete';row._epoch=epoch;
        row.notes='Emitter reported ERROR before TRUTH BEGIN; exclude from RF accuracy.';
        trials.push(row);
        selectedTrial=trials.length-1;
      }
      redrawTrials();selectTrial();
      notice('Emitter '+event.event+' ('+(event.status||'unspecified')+
        '). This is not proof of receiving a signal.',false);
    }
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
      if(role==='emitter')epoch++;
      opening[role]=false;updateButtons();
      append(role,'\n[connected at '+baud+' baud; no commands sent]\n');
      d.reading=readPort(role,port);
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
    d.port=null;updateButtons();
    try{if(d.reader)await d.reader.cancel();}catch(_){}
    try{if(d.reading)await d.reading;}catch(_){}
    try{await p.close();}catch(_){}
    d.reader=null;d.reading=null;
    append(role,'\n[disconnected]\n');updateButtons();
  }
  async function sendEmitter(command){
    var port=devices.emitter.port;
    if(!port||!port.writable){notice('Connect SquachEmit before sending a command.',true);return;}
    var writer=port.writable.getWriter();
    try{
      await writer.write(new TextEncoder().encode(command+'\n'));
      append('emitter','> '+command+'\n');
    }catch(e){notice('Serial write failed: '+e.message,true);}
    finally{try{writer.releaseLock();}catch(_){}}
  }
  async function run(){
    var c=catalog[ui.caseSelect.value],duration=Number(ui.duration.value);
    if(!c||!ui.authorization.checked||!Number.isInteger(duration)||
       duration<3||duration>30||!devices.emitter.port)return;
    // Never mark detection from the user clicking RUN.
    await sendEmitter('RUN '+c.id+' '+duration);
    notice('RUN command sent. Trial appears only after actual emitter TRUTH BEGIN; verify physical receiver.',false);
  }
  function saveObservation(){
    var old=trials[selectedTrial];if(!old)return;
    var result=Object.assign({},old,{
      run_id:ui.runId.value.trim()||old.run_id,
      receiver:ui.receiverBoard.value,
      receiver_sha:ui.receiverSha.value.trim()||old.receiver_sha,
      emitter_sha:ui.emitterSha.value.trim()||old.emitter_sha,
      emission:ui.emissionStatus.value||old.emission,
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
        ids.add(c.id);catalog[c.id]=c;
      });
      cases=data;
      cases.sort(function(a,b){return a.id.localeCompare(b.id);});
      ui.caseSelect.replaceChildren(option('','Choose a test case…'));
      cases.forEach(function(c){
        ui.caseSelect.appendChild(option(c.id,c.id+' · '+c.expected));
      });
      var values=Array.from(new Set(cases.map(function(c){return c.expected;})
        .concat(['NONE','ALPR','CAMERA','DEAUTH','EVILTWIN','RING','AXON']))).sort();
      ui.observed.replaceChildren(option('','Not evaluated'));
      values.forEach(function(s){ui.observed.appendChild(option(s,s));});
      chooseCase();redrawTrials();
      notice('Loaded the 172-case catalog. No hardware is connected or transmitting.',false);
    }catch(e){
      notice('Catalog could not load: '+e.message+'. Run the published workbench site or the build-installer static output.',true);
      ui.caseSelect.replaceChildren(option('','Case catalog unavailable'));
    }
  }
  ui.caseSelect.addEventListener('change',chooseCase);
  ui.authorization.addEventListener('change',updateButtons);
  ui.duration.addEventListener('input',updateButtons);
  ui.emitConnect.addEventListener('click',function(){void connect('emitter');});
  ui.recvConnect.addEventListener('click',function(){void connect('receiver');});
  ui.emitDisconnect.addEventListener('click',function(){void disconnect('emitter');});
  ui.recvDisconnect.addEventListener('click',function(){void disconnect('receiver');});
  ui.runCase.addEventListener('click',function(){void run();});
  ui.stopCase.addEventListener('click',function(){void sendEmitter('STOP');});
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
      imported.forEach(function(row){
        if(!catalog[row.id])throw new Error('Unknown case: '+row.id);
        if(!['CYD-2.8','CYD-3.2'].includes(row.receiver))
          throw new Error('Unknown receiver board');
      });
      if(imported.length+trials.length>5000)throw new Error('Maximum 5,000 trials.');
      trials.push.apply(trials,imported);
      selectedTrial=-1;redrawTrials();selectTrial();
      notice('Imported '+imported.length+' trial records. Unverified or incomplete data remains unscored.',false);
    }catch(e){notice('CSV import failed: '+e.message,true);}
  });
  ui.clearTrials.addEventListener('click',function(){
    if(!trials.length)return;
    if(!window.confirm('Permanently clear all '+trials.length+' local trials? Export CSV first.'))return;
    trials=[];selectedTrial=-1;redrawTrials();selectTrial();
    notice('Local trial ledger cleared.',false);
  });
  window.addEventListener('beforeunload',function(){
    Object.keys(devices).forEach(function(role){var r=devices[role].reader;if(r)r.cancel().catch(function(){});});
  });
  redrawTrials();updateButtons();void loadCases();
})();
