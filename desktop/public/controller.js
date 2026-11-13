/* Dual-device control rail for the Windows desktop Workbench.
 * Telemetry panels are NOT pixel-level LCD mirrors. Optional USB camera
 * captures show the actual physical displays locally and are never uploaded.
 */
(function(){
  'use strict';
  if(new URLSearchParams(location.search).get('desktop')!=='1')return;
  const api=window.SQEWorkbenchDesktop;
  if(!api)throw Error('Workbench desktop serial bridge unavailable');
  const root=document.querySelector('main');
  if(!root)throw Error('Workbench main element missing');
  root.classList.add('desktop-workbench');
  const workbench=document.createElement('div');
  workbench.id='desktopWorkbenchLeft';
  while(root.firstChild)workbench.appendChild(root.firstChild);
  root.appendChild(workbench);
  const rail=document.createElement('aside');
  rail.id='desktopControlRail';
  rail.setAttribute('aria-label','Live device controls');
  root.appendChild(rail);
  const cameras={emitter:null,receiver:null};
  const terminal={emitter:'',receiver:''};
  const labels={emitter:'SquachEmit',receiver:'SquachWatch'};
  const types={emitter:'TRANSMITTER · 115,200 BAUD',receiver:'RECEIVER · 2,000,000 BAUD'};
  function markup(role){
    const emitter=role==='emitter';
    const actions=emitter?
      [['status','STATUS'],['identity','IDENTITY'],['list','LIST'],['help','HELP'],
       ['run','RUN SELECTED'],['stop','STOP']]:
      [['log','LOG'],['radio','RADIO'],['radio-scan','RADIO SCAN'],
       ['mem','MEM'],['scan-auto','SCAN AUTO'],
       ['scan-active','SCAN ACTIVE'],['scan-passive','SCAN PASSIVE'],
       ['rotate','ROT']];
    const buttons=actions.map(([action,name])=>
      '<button type="button" data-action="'+action+'" data-role="'+role+'">'+name+'</button>').join('');
    return '<section class="dc-card" id="dc-'+role+'">'+
      '<header class="dc-cardhead"><div class="dc-icon">'+(emitter?'◈':'◎')+'</div>'+
      '<div><div class="dc-category">'+types[role]+'</div>'+
      '<h3>'+labels[role]+'</h3></div>'+
      '<span class="dc-state" id="dc-state-'+role+'">OFFLINE</span></header>'+
      '<div class="dc-display" id="dc-display-'+role+'">'+
      '<div class="dc-screen" id="dc-screen-'+role+'">'+
      '<div class="dc-screenbar"><span>◉ LIVE SERIAL TELEMETRY</span><span id="dc-led-'+role+'">●</span></div>'+
      '<div class="dc-screenname" id="dc-name-'+role+'">'+
        (emitter?'SQUACHEMIT':'SQUACHWATCH')+'</div>'+
      '<div class="dc-screenmetric" id="dc-primary-'+role+'">'+
        (emitter?'NO ACTIVE SIGNAL':'NO RECEIVER LOG YET')+'</div>'+
      '<div class="dc-screensub" id="dc-secondary-'+role+'">'+
        'Connect USB to see real device data'+'</div>'+
      '<div class="dc-screenfoot" id="dc-footer-'+role+'">LOCAL USB · NO SCREEN CAPTURE</div>'+
      '</div><video id="dc-video-'+role+'" autoplay muted playsinline hidden></video></div>'+
      '<div class="dc-displaymode">Actual screen video requires a separate camera '+
        'pointed at the display. The graphic above is live telemetry, not a screenshot.</div>'+
      '<div class="dc-camera"><label for="dc-source-'+role+'">LIVE CAMERA SOURCE</label>'+
      '<select id="dc-source-'+role+'"><option value="">Choose camera…</option></select>'+
      '<div class="dc-camera-actions">'+
      '<button type="button" data-action="refresh-cameras" data-role="'+role+'">Refresh cameras</button>'+
      '<button type="button" data-action="start-camera" data-role="'+role+'">Show video</button>'+
      '<button type="button" data-action="stop-camera" data-role="'+role+'">Stop video</button>'+
      '</div></div>'+
      '<div class="dc-connections">'+
      '<button type="button" data-action="connect" data-role="'+role+'">Connect USB</button>'+
      '<button type="button" data-action="disconnect" data-role="'+role+'">Disconnect</button>'+
      '</div>'+
      '<div class="dc-actiontitle">DEVICE COMMANDS</div><div class="dc-actions">'+buttons+'</div>'+
      (!emitter?'<div class="dc-advanced"><label>Clock font / size / backdrop</label>'+
      '<div class="dc-clock-controls">'+
      '<select id="dc-clockfont"><option value="0">Font 0</option><option value="1">Font 1</option></select>'+
      '<select id="dc-clocksize"><option value="0">Size 0</option><option value="1">Size 1</option><option value="2">Size 2</option></select>'+
      '<select id="dc-clockbg">'+Array.from({length:7},(_,i)=>
        '<option value="'+i+'">BG '+i+'</option>').join('')+'</select></div>'+
      '<button type="button" data-action="clock" data-role="receiver">Apply clock appearance</button>'+
      '<small>Existing SquachWatch serial commands; on-device PIN lock is respected.</small></div>':'')+
      '<div class="dc-actiontitle">LAST USB OUTPUT</div>'+
      '<pre class="dc-terminal" id="dc-terminal-'+role+'">No serial data</pre>'+
      '<div class="dc-feedback" id="dc-feedback-'+role+'" role="status">Ready.</div>'+
      '</section>';
  }
  rail.innerHTML='<div class="dc-railhead"><div><div class="dc-overline">WINDOWS CONTROL CENTER</div>'+
    '<h2>Live device desk</h2><p>Two USB serial sessions · independent camera views</p></div>'+
    '<button type="button" id="dc-export">Export CSV</button></div>'+
    markup('receiver')+markup('emitter')+
    '<div class="dc-footnote"><b>REMOTE CONTROL LIMIT:</b> These USB firmware '+
    'builds do not expose LCD framebuffers or remote touchscreen coordinates. '+
    'The panel controls supported serial functions. Video requires a camera. '+
    'No synthetic display is presented as a real screen capture.</div>';
  const get=id=>document.getElementById(id);
  function info(role,msg){get('dc-feedback-'+role).textContent=msg;}
  function connected(role,state){
    const yes=role==='emitter'?state.emitterConnected:state.receiverConnected;
    get('dc-state-'+role).textContent=yes?'USB CONNECTED':'OFFLINE';
    get('dc-state-'+role).classList.toggle('online',!!yes);
    get('dc-led-'+role).classList.toggle('online',!!yes);
    if(!yes){
      get('dc-footer-'+role).textContent='DISCONNECTED · CAMERA INDEPENDENT';
      const frame=get('dc-primary-'+role);
      if(role==='receiver')frame.textContent='RECEIVER NOT CONNECTED';
      if(role==='emitter')frame.textContent='TRANSMITTER NOT CONNECTED';
    }
  }
  function refreshState(){
    const state=api.getState();
    connected('receiver',state);connected('emitter',state);
    if(state.emitterConnected&&!state.transmitting &&
       !get('dc-primary-emitter').textContent.startsWith('READY'))
      get('dc-footer-emitter').textContent='USB ACTIVE · '+state.catalogCount+' SIGNALS';
    const disabled=state.batchActive;
    rail.querySelectorAll('[data-action="run"],[data-action="scan-auto"],'+
      '[data-action="scan-active"],[data-action="scan-passive"],'+
      '[data-action="rotate"],[data-action="clock"]').forEach(b=>{b.disabled=disabled;});
  }
  function appendLine(role,line){
    terminal[role]=(terminal[role]+String(line)).slice(-1800);
    const lines=terminal[role].split('\n');
    get('dc-terminal-'+role).textContent=lines.slice(-9).join('\n')||'No serial data';
    const box=get('dc-terminal-'+role);box.scrollTop=box.scrollHeight;
    const candidate=String(line).trim();
    if(role==='emitter'&&candidate.startsWith('STATUS,')){
      get('dc-secondary-emitter').textContent=candidate.slice(0,90);
    }
    if(role==='receiver'&&candidate.startsWith('[security]')){
      info('receiver',candidate+' Physical unlock required.');
    }
  }
  function truth(event){
    get('dc-name-emitter').textContent=event.name||'SQUACHEMIT';
    get('dc-primary-emitter').textContent=event.event==='BEGIN'?'TRANSMITTING':
      event.event==='END'?'RUN COMPLETE':'TRANSMITTER ERROR';
    get('dc-secondary-emitter').textContent=event.id+
      ' · SquachWatch expects '+event.expected;
    get('dc-footer-emitter').textContent=event.event+
      (event.status?' · '+event.status:'')+' · USB TRUTH RECORD';
    info('emitter',event.event+' · '+event.id+
      (event.status?' · '+event.status:''));
  }
  function snapshot(data){
    const first=data.rows&&data.rows.find(r=>r.state==='here');
    get('dc-primary-receiver').textContent=data.rowCount+' LOGGED DETECTIONS';
    get('dc-secondary-receiver').textContent=first?
      first.type+' · '+first.mac+' · '+first.rssi+' dBm':
      'No currently active detection in LOG';
    get('dc-footer-receiver').textContent=
      'LOG RECEIVED · '+new Date(data.when).toLocaleTimeString();
  }
  async function refreshCameras(){
    if(!navigator.mediaDevices||!navigator.mediaDevices.enumerateDevices){
      ['receiver','emitter'].forEach(role=>info(role,'Camera API unavailable in this desktop session.'));
      return;
    }
    try{
      const devices=await navigator.mediaDevices.enumerateDevices();
      const list=devices.filter(d=>d.kind==='videoinput');
      for(const role of ['receiver','emitter']){
        const select=get('dc-source-'+role),old=select.value;
        select.replaceChildren();
        const placeholder=document.createElement('option');
        placeholder.value='';placeholder.textContent=list.length?
          'Choose Windows camera…':'No cameras detected';
        select.appendChild(placeholder);
        list.forEach((device,i)=>{
          const option=document.createElement('option');
          option.value=device.deviceId;
          option.textContent=device.label||'Camera '+(i+1);
          select.appendChild(option);
        });
        if(list.some(d=>d.deviceId===old))select.value=old;
      }
    }catch(err){
      info('receiver','Camera enumeration failed: '+err.message);
    }
  }
  function stopCamera(role){
    if(cameras[role])cameras[role].getTracks().forEach(t=>t.stop());
    cameras[role]=null;
    const video=get('dc-video-'+role);
    video.pause();video.srcObject=null;video.hidden=true;
    get('dc-screen-'+role).hidden=false;
    info(role,'Camera stopped; serial telemetry remains live.');
  }
  async function startCamera(role){
    const id=get('dc-source-'+role).value;
    if(!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia){
      info(role,'No Windows video capture source available.');return;
    }
    if(!id){info(role,'Select a camera pointed at '+labels[role]+' first.');return;}
    stopCamera(role);
    try{
      const stream=await navigator.mediaDevices.getUserMedia({
        video:{deviceId:{exact:id},width:{ideal:1280},height:{ideal:720},
          frameRate:{ideal:15,max:30}},audio:false
      });
      cameras[role]=stream;
      const video=get('dc-video-'+role);
      video.srcObject=stream;
      await video.play();
      video.hidden=false;get('dc-screen-'+role).hidden=true;
      info(role,'Live Windows camera connected. Video stays on this PC.');
      await refreshCameras();
    }catch(err){info(role,'Camera unavailable: '+err.message);}
  }
  async function onControl(action,role){
    if(action==='refresh-cameras'){await refreshCameras();return;}
    if(action==='start-camera'){await startCamera(role);return;}
    if(action==='stop-camera'){stopCamera(role);return;}
    if(action==='connect'){info(role,'Choose the '+labels[role]+' serial port…');
      await api.connect(role);refreshState();return;}
    if(action==='disconnect'){await api.disconnect(role);refreshState();return;}
    if(role==='emitter'&&action==='run'){
      await api.runSelected();
      info(role,'Requested selected Workbench case; transmitter TRUTH verifies RUN.');
      return;
    }
    if(role==='emitter'&&action==='stop'){
      await api.send(role,'STOP');info(role,'STOP requested.');return;
    }
    const commandMap={
      status:'STATUS',identity:'IDENTITY',list:'LIST',help:'HELP',
      log:'LOG',radio:'RADIO','radio-scan':'RADIO SCAN',mem:'MEM',
      'scan-auto':'SCAN AUTO','scan-active':'SCAN ACTIVE',
      'scan-passive':'SCAN PASSIVE',rotate:'ROT'
    };
    let cmd=commandMap[action];
    if(role==='receiver'&&action==='clock'){
      cmd='CLOCK '+get('dc-clockfont').value+' '+
        get('dc-clocksize').value+' '+get('dc-clockbg').value;
    }
    if(!cmd)return;
    const ok=await api.send(role,cmd);
    info(role,ok?'Command sent: '+cmd:
      'Command unavailable during batch, device offline or unsupported: '+cmd);
  }
  rail.addEventListener('click',event=>{
    const btn=event.target.closest('button[data-action][data-role]');
    if(!btn||btn.disabled)return;
    void onControl(btn.dataset.action,btn.dataset.role).catch(err=>{
      info(btn.dataset.role,'Operation failed: '+err.message);
    });
  });
  get('dc-export').addEventListener('click',()=>api.exportCsv());
  window.addEventListener('sqe:serial',e=>appendLine(e.detail.role,e.detail.line));
  window.addEventListener('sqe:snapshot',e=>snapshot(e.detail));
  window.addEventListener('sqe:truth',e=>truth(e.detail));
  window.addEventListener('sqe:state',refreshState);
  window.addEventListener('beforeunload',()=>{
    stopCamera('receiver');stopCamera('emitter');
  });
  void refreshCameras();
  refreshState();
})();
