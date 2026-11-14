/* Windows SquachEmit + SquachWatch control desk.
 * SquachEmit renders locally from UI STATE, never a TFT screenshot.
 * Receiver optional screenshot mode stays on the independent USB port.
 */
(function(){
  'use strict';
  if(new URLSearchParams(location.search).get('desktop')!=='1')return;
  const api=window.SQEWorkbenchDesktop,codec=window.SQEMirrorCodec,native=window.SQECydNative;
  if(!api||!codec||!native)throw Error('Windows USB native UI or mirror decoder missing');
  const main=document.querySelector('main');
  if(!main)throw Error('Workbench content unavailable');
  main.classList.add('desktop-workbench');
  const left=document.createElement('div');
  left.id='desktopWorkbenchLeft';
  while(main.firstChild)left.appendChild(main.firstChild);
  main.appendChild(left);
  const rail=document.createElement('aside');
  rail.id='desktopControlRail';
  rail.setAttribute('aria-label','Real USB device displays and remote controls');
  main.appendChild(rail);
  const names={receiver:'SquachWatch',emitter:'SquachEmit'};
  const terminals={receiver:'',emitter:''};
  const local={model:null,catalog:native.prepare([]),receivedAt:0};
  const mirrors={};
  for(const role of ['receiver','emitter']){
    mirrors[role]={active:false,waiting:false,lastFrame:false,lastFrameSequence:0,failures:0,
      assembler:new codec.MirrorAssembler(),timeout:null,next:null,handshake:null,animation:null,ready:false,
      touchDown:false,touchAt:0,touchTimer:null,pendingUp:null,lastPoint:{x:0,y:0},
      lastMove:0};
  }
  const get=id=>document.getElementById(id);
  const isConnected=(role,state)=>role==='emitter'?
    !!state.emitterConnected:!!state.receiverConnected;
  function markup(role){
    const emitter=role==='emitter';
    const actions=emitter?
      [['status','STATUS'],['identity','IDENTITY'],['list','LIST'],
       ['help','HELP'],['run','RUN SELECTED'],['stop','STOP']]:
      [['log','LOG'],['radio','RADIO'],['radio-scan','RADIO SCAN'],
       ['mem','MEM'],['scan-auto','SCAN AUTO'],
       ['scan-active','SCAN ACTIVE'],['scan-passive','SCAN PASSIVE'],
       ['rotate','ROT']];
    const buttons=actions.map(([action,name])=>
      '<button type="button" data-action="'+action+'" data-role="'+role+'">'+name+'</button>').join('');
    return '<section class="dc-card" id="dc-'+role+'">'+
      '<header class="dc-cardhead"><div class="dc-icon">'+(emitter?'◈':'◎')+'</div>'+
      '<div><div class="dc-category">REAL USB DEVICE · '+(emitter?'EMITTER':'RECEIVER')+'</div>'+
      '<h3>'+names[role]+'</h3></div>'+
      '<span class="dc-state" id="dc-state-'+role+'">OFFLINE</span></header>'+
      '<div class="dc-display" id="dc-display-'+role+'">'+
      '<div class="dc-lcd" id="dc-lcd-'+role+'">'+
      '<canvas id="dc-canvas-'+role+'" width="320" height="240" tabindex="0" '+
      'aria-label="USB pixels from physical '+names[role]+' LCD; click or drag to touch" hidden></canvas>'+
      '<div class="dc-placeholder" id="dc-placeholder-'+role+'">'+
      '<div class="dc-placeholder-icon">▣</div>'+
      '<b>NO LIVE LCD FRAME</b><small>Connect USB and choose Mirror live. '+
      'The panel stays blank until real pixels arrive.</small></div>'+
      '</div></div>'+
      '<div class="dc-frame-meta" id="dc-frame-'+role+'" role="status">'+
      'LCD framebuffer: not connected</div>'+
      '<div class="dc-mirror-actions">'+
      '<button type="button" data-role="'+role+'" data-action="mirror-on">'+(emitter?'Sync device UI':'Mirror live')+'</button>'+
      '<button type="button" data-role="'+role+'" data-action="mirror-frame">'+(emitter?'Refresh state':'Get frame')+'</button>'+
      '<button type="button" data-role="'+role+'" data-action="mirror-off">Stop</button></div>'+
      (emitter?'<button type="button" data-role="emitter" data-action="mirror-diag">Check device UI state</button>':'')+
      '<div class="dc-displaymode">'+(emitter?
        'PC-drawn SquachEmit UI, synchronized with device navigation and RF state. No screenshots or 2Mbps switch. Click, hold or drag to operate the physical touchscreen.':
        'Optional SquachWatch USB pixel mirror; pause it for receiver LOG tests.')+'</div>'+
      '<div class="dc-connections">'+
      '<button type="button" data-action="connect" data-role="'+role+'">Connect USB</button>'+
      '<button type="button" data-action="disconnect" data-role="'+role+'">Disconnect</button></div>'+
      '<div class="dc-actiontitle">SERIAL DEVICE COMMANDS</div>'+
      '<div class="dc-actions">'+buttons+'</div>'+
      (!emitter?'<div class="dc-advanced"><label>Clock font / size / backdrop</label>'+
      '<div class="dc-clock-controls">'+
      '<select id="dc-clockfont"><option value="0">Font 0</option><option value="1">Font 1</option></select>'+
      '<select id="dc-clocksize"><option value="0">Size 0</option>'+
      '<option value="1">Size 1</option><option value="2">Size 2</option></select>'+
      '<select id="dc-clockbg">'+Array.from({length:7},(_,i)=>
        '<option value="'+i+'">BG '+i+'</option>').join('')+'</select></div>'+
      '<button type="button" data-action="clock" data-role="receiver">Apply clock appearance</button>'+
      '<small>PIN lock is enforced by the receiver firmware; this GUI cannot unlock it.</small></div>':'')+
      '<div class="dc-actiontitle">LAST SERIAL OUTPUT</div>'+
      '<pre class="dc-terminal" id="dc-terminal-'+role+'">No serial data</pre>'+
      '<div class="dc-feedback" id="dc-feedback-'+role+'" role="status">Ready.</div>'+
      '</section>';
  }
  rail.innerHTML='<div class="dc-railhead"><div><div class="dc-overline">WINDOWS CONTROL CENTER</div>'+
    '<h2>Interactive device screens</h2><p>SquachEmit drawn locally · hardware authoritative</p></div>'+
    '<button type="button" id="dc-export">Export CSV</button></div>'+
    markup('receiver')+markup('emitter')+
    '<div class="dc-footnote"><b>NATIVE SQUACHEMIT UI.</b> The right-side interface is drawn on the PC from real hardware state, not captured LCD pixels. Procedural artwork and font fidelity require device testing. SquachWatch receiver mirroring remains optional and separate.</div>';
  function info(role,message){get('dc-feedback-'+role).textContent=message;}
  function mirrorNote(role,message){get('dc-frame-'+role).textContent=message;}
  function cancelFrameTimers(role){
    const s=mirrors[role];
    if(s.timeout!==null)clearTimeout(s.timeout);
    if(s.next!==null)clearTimeout(s.next);
    if(s.handshake!==null)clearTimeout(s.handshake);
    if(s.animation!==null)clearInterval(s.animation);
    s.timeout=null;s.next=null;s.handshake=null;s.animation=null;
  }
  function resetRole(role){
    const s=mirrors[role];
    cancelFrameTimers(role);
    s.active=false;s.waiting=false;s.ready=false;s.failures=0;
    s.lastFrame=false;s.lastFrameSequence=0;
    s.assembler.reset();
    if(role==='emitter'){local.model=null;local.receivedAt=0;}
    // Do not present a stale screen as live.
    get('dc-canvas-'+role).hidden=true;
    get('dc-placeholder-'+role).hidden=false;
    if(s.touchTimer!==null)clearInterval(s.touchTimer);
    if(s.pendingUp!==null)clearTimeout(s.pendingUp);
    s.touchTimer=null;s.pendingUp=null;s.touchDown=false;
  }
  async function stopMirror(role,send){
    const s=mirrors[role],was=s.active;
    const connected=isConnected(role,api.getState());
    // Deliver an explicit UP before switching off or disconnecting. A lost
    // pointer capture must not leave the physical device pressed.
    if(send&&was&&connected&&(s.touchDown||s.pendingUp!==null))
      await api.send(role,(role==='emitter'?'UI TOUCH ':'MIRROR TOUCH ')+s.lastPoint.x+' '+s.lastPoint.y+' UP');
    resetRole(role);
    if(send&&was&&connected){
      const ok=await api.send(role,role==='emitter'?'UI OFF':'MIRROR OFF');
      mirrorNote(role,ok?'USB mirror stopped. LCD view disconnected.':
        'USB mirror stop was not acknowledged; view is disconnected.');
    }else mirrorNote(role,'USB mirror offline. Connect to show real LCD pixels.');
  }
  function drawRealFrame(role,frame){
    const canvas=get('dc-canvas-'+role);
    canvas.width=frame.width;canvas.height=frame.height;
    const ctx=canvas.getContext('2d',{alpha:false});
    if(!ctx){mirrorNote(role,'Canvas 2D unsupported');return;}
    ctx.putImageData(new ImageData(frame.pixels,frame.width,frame.height),0,0);
    canvas.hidden=false;
    get('dc-placeholder-'+role).hidden=true;
    mirrors[role].lastFrame=true;
    mirrors[role].lastFrameSequence=frame.sequence;
    const panel=get('dc-display-'+role);
    panel.style.aspectRatio=frame.width+' / '+frame.height;
    mirrorNote(role,'LIVE PHYSICAL LCD · '+frame.width+'×'+frame.height+
      ' · '+frame.format+(frame.delta?' · '+frame.changedRows+' changed rows':' · full frame')+
      ' · #'+frame.sequence+' · '+new Date().toLocaleTimeString());
  }
  async function requestMirrorFrame(role){
    if(role==='emitter'){
      if(!mirrors.emitter.active||!isConnected(role,api.getState()))return;
      const ok=await api.send('emitter','UI GET');
      if(!ok)info('emitter','UI GET failed; check USB and firmware.');
      return;
    }
    const s=mirrors[role],state=api.getState();
    if(!s.active||!s.ready||s.waiting||!isConnected(role,state)||
       (role==='receiver'&&state.testActive))return;
    s.waiting=true;
    // Firmware sends a full keyframe unless it recognizes the last complete
    // screen the PC actually accepted. A dropped frame can never corrupt
    // a later delta. SquachWatch's existing firmware retains FRAME syntax.
    const request=(role==='emitter'&&s.lastFrameSequence)?
      'MIRROR FRAME '+s.lastFrameSequence:'MIRROR FRAME';
    const ok=await api.send(role,request);
    if(!ok){
      s.waiting=false;
      info(role,'Cannot request USB frame. Verify connection and LAB firmware.');
      return;
    }
    if(!s.active)return;
    s.timeout=setTimeout(()=>{
      if(!s.active||!s.waiting)return;
      s.assembler.reset();s.waiting=false;s.failures++;
      if(s.failures>=3){
        info(role,'No complete LCD frame after three requests. Confirm firmware MIRROR support, '+
          'receiver PIN unlock, TFT framebuffer availability and serial baud.');
        void stopMirror(role,true);
        return;
      }
      mirrorNote(role,'No complete LCD pixels; retrying frame ('+
        s.failures+'/3). Check device mirror firmware and TFT readback.');
      s.next=setTimeout(()=>void requestMirrorFrame(role),700);
    },role==='emitter'?(state.emitterBaud===115200?45000:16000):12000);
  }
  async function startMirror(role){
    const state=api.getState();
    if(role==='emitter'){
      if(!isConnected('emitter',state)){
        info('emitter','Connect at 115200 to synchronize the native UI.');return;
      }
      const s=mirrors.emitter;
      resetRole('emitter');s.active=true;
      info('emitter','Requesting real navigation state (not LCD pixels)…');
      const ok=await api.send('emitter','UI ON');
      if(!ok){resetRole('emitter');info('emitter','UI ON failed.');return;}
      mirrorNote('emitter','Waiting for UI READY from R7 firmware…');
      s.handshake=setTimeout(()=>{
        s.handshake=null;
        if(!s.active||s.ready)return;
        resetRole('emitter');
        const err='No UI READY: flash the R7 LAB firmware and connect at 115200.';
        mirrorNote('emitter',err);info('emitter',err);
      },6000);
      return;
    }
    if(role==='receiver'&&state.testActive){
      info(role,'SquachWatch LCD mirroring pauses during detection tests so '+
        'its USB connection can deliver exact-MAC LOG results. Re-enable afterward.');
      return;
    }
    if(!isConnected(role,state)){
      info(role,'Connect the '+names[role]+' USB device before mirroring.');return;
    }
    if(role==='emitter'&&![115200,2000000].includes(state.emitterBaud)){
      info(role,'Unsupported SquachEmit serial baud for LCD capture.');return;
    }
    if(role==='emitter'&&state.emitterBaud===115200)
      info(role,'Starting RGB565 mirror at safe 115200 baud. First complete LCD'+
        ' frame can take 20–40 seconds; 2 Mbps is recommended for live motion.');
    const s=mirrors[role];
    resetRole(role);
    s.active=true;
    info(role,'Enabling opt-in USB mirroring…');
    const ok=await api.send(role,'MIRROR ON');
    if(!ok){resetRole(role);info(role,'MIRROR ON command rejected or unavailable.');return;}
    mirrorNote(role,'Waiting for MIRROR READY from '+names[role]+' firmware…');
    // A successful USB write is NOT an acknowledgment from the actual device.
    // Wrong baud, firmware without MIRROR support and MCU reset all fail here.
    s.handshake=setTimeout(()=>{
      s.handshake=null;
      if(!s.active||s.ready)return;
      resetRole(role);
      const message=role==='emitter'?
        'No MIRROR READY. Device may have reset to 115200 after reconnect, '+
        'or firmware lacks MIRROR support. Reconnect at 115200 and click Diagnose.':
        'No MIRROR READY. Check the SquachWatch companion mirror firmware and PIN.';
      mirrorNote(role,message);info(role,message);
    },5000);
  }
  function onMirror({role,line}){
    if(!mirrors[role])return;
    const s=mirrors[role],result=s.assembler.feed(line);
    if(result.kind==='info'){
      if(/^MIRROR DIAG/.test(result.message)){
        mirrorNote(role,result.message);
        info(role,result.message);
      }else if(/^MIRROR READY\b/.test(result.message)&&s.active&&!s.ready){
        s.ready=true;
        if(s.handshake!==null){clearTimeout(s.handshake);s.handshake=null;}
        mirrorNote(role,'Firmware acknowledged. Capturing authentic LCD pixels…');
        void requestMirrorFrame(role);
      }
      return;
    }
    if(result.kind==='error'){
      cancelFrameTimers(role);s.waiting=false;s.failures++;
      info(role,result.message);
      if(/LCD_READ_UNAVAILABLE|LCD_NOT_READY|BAD_DISPLAY_SIZE/.test(result.message)){
        void stopMirror(role,true).then(()=>{
          mirrorNote(role,result.message+' — check 6MHz TFT readback and MIRROR DIAG');
          info(role,result.message);
        });
        return;
      }
      if(s.active&&s.failures>=3){void stopMirror(role,true);return;}
      if(s.active)s.next=setTimeout(()=>void requestMirrorFrame(role),450);
      return;
    }
    if(!s.active)return; // Never display unsolicited screen bytes.
    if(result.kind==='begin'){
      mirrorNote(role,'Receiving '+(result.delta?'changed rows from':'full')+
        ' physical LCD '+result.width+'×'+result.height+' '+result.format+'…');
      return;
    }
    if(result.kind==='row'){
      if(result.rows===result.total||result.rows%64===0){
        mirrorNote(role,'LCD capture '+result.rows+'/'+result.total+' rows…');
      }
      return;
    }
    if(result.kind==='frame'){
      cancelFrameTimers(role);s.waiting=false;s.failures=0;
      drawRealFrame(role,result.frame);
      // Continuous polling with device-side backpressure: changed-row
      // frames are much cheaper than full RGB565 screenshots.
      s.next=setTimeout(()=>void requestMirrorFrame(role),role==='emitter'?65:800);
    }
  }
  function connected(role,state){
    const yes=isConnected(role,state),el=get('dc-state-'+role);
    el.textContent=yes?'USB CONNECTED':'OFFLINE';
    el.classList.toggle('online',yes);
    if(!yes&&mirrors[role].active){
      resetRole(role);
      mirrorNote(role,'USB disconnected — no live LCD image.');
    }
    // Normal 115200 baud carries UI states; no forced 2Mbps handoff.
  }
  function refreshState(){
    const state=api.getState();
    connected('receiver',state);connected('emitter',state);
    const disabled=state.batchActive;
    rail.querySelectorAll('[data-action="run"],[data-action="scan-auto"],'+
      '[data-action="scan-active"],[data-action="scan-passive"],'+
      '[data-action="rotate"],[data-action="clock"],'+
      '[data-action="switch-baud"]').forEach(b=>{b.disabled=disabled;});
    // Only the SquachWatch receiver USB must switch to LOG during tests.
    // SquachEmit has its own port and can keep streaming actual LCD pixels.
    rail.querySelectorAll('[data-action="mirror-on"],[data-action="mirror-frame"]').forEach(b=>{
      b.disabled=b.dataset.role==='receiver'&&!!state.testActive;
    });
  }
  function appendLine(role,line){
    terminals[role]=(terminals[role]+String(line)).slice(-1800);
    const lines=terminals[role].split('\n');
    const box=get('dc-terminal-'+role);
    box.textContent=lines.slice(-9).join('\n')||'No serial data';
    box.scrollTop=box.scrollHeight;
    if(role==='receiver'&&String(line).includes('[security]'))
      info(role,'Receiver reported a security state. Unlock PIN on the physical device.');
  }
  function truth(event){
    info('emitter',event.event+' · '+event.id+
      (event.status?' · '+event.status:''));
  }
  function snapshot(data){
    const first=data.rows&&data.rows.find(r=>r.state==='here');
    const msg='LOG: '+data.rowCount+' detections'+
      (first?' · active '+first.type+' '+first.mac+' '+first.rssi+' dBm':'');
    if(!mirrors.receiver.active)mirrorNote('receiver',msg+
      ' · LCD capture available through Mirror live');
  }
  function touchPoint(role,event){
    const canvas=get('dc-canvas-'+role),r=canvas.getBoundingClientRect();
    if(!r.width||!r.height)return null;
    return {
      x:Math.max(0,Math.min(canvas.width-1,
        Math.floor((event.clientX-r.left)*canvas.width/r.width))),
      y:Math.max(0,Math.min(canvas.height-1,
        Math.floor((event.clientY-r.top)*canvas.height/r.height)))
    };
  }
  function sendTouch(role,point,phase){
    if(!isConnected(role,api.getState()))return;
    void api.send(role,(role==='emitter'?'UI TOUCH ':'MIRROR TOUCH ')+point.x+' '+point.y+' '+phase)
      .then(ok=>{if(!ok)info(role,'Remote touch rejected; check PIN or USB mirror firmware.');})
      .catch(err=>info(role,'Remote touch error: '+err.message));
  }
  function releaseTouch(role,point,minimumHold){
    const s=mirrors[role];
    if(!s.touchDown)return;
    s.touchDown=false;
    if(s.touchTimer!==null)clearInterval(s.touchTimer);
    s.touchTimer=null;
    const finish=()=>sendTouch(role,point||s.lastPoint,'UP');
    const remain=Math.max(0,minimumHold-(Date.now()-s.touchAt));
    if(remain)s.pendingUp=setTimeout(()=>{s.pendingUp=null;finish();},remain);
    else finish();
  }
  for(const role of ['receiver','emitter']){
    const canvas=get('dc-canvas-'+role),s=mirrors[role];
    canvas.addEventListener('pointerdown',event=>{
      if(!s.active||!s.lastFrame||api.getState().batchActive)return;
      const p=touchPoint(role,event);
      if(!p)return;
      event.preventDefault();
      if(s.touchDown)releaseTouch(role,p,0);
      if(s.pendingUp!==null){
        clearTimeout(s.pendingUp);s.pendingUp=null;
        sendTouch(role,s.lastPoint,'UP');
      }
      s.touchDown=true;s.touchAt=Date.now();s.lastPoint=p;s.lastMove=Date.now();
      try{canvas.setPointerCapture(event.pointerId);}catch(_){}
      sendTouch(role,p,'DOWN');
      s.touchTimer=setInterval(()=>{
        if(s.touchDown)sendTouch(role,s.lastPoint,'MOVE');
      },800);
    });
    canvas.addEventListener('pointermove',event=>{
      if(!s.touchDown)return;
      const p=touchPoint(role,event);
      if(!p)return;
      event.preventDefault();
      s.lastPoint=p;
      if(Date.now()-s.lastMove>=90){
        s.lastMove=Date.now();
        sendTouch(role,p,'MOVE');
      }
    });
    canvas.addEventListener('pointerup',event=>{
      if(!s.touchDown)return;
      event.preventDefault();
      const p=touchPoint(role,event)||s.lastPoint;
      releaseTouch(role,p,150);
    });
    canvas.addEventListener('pointercancel',()=>releaseTouch(role,s.lastPoint,0));
    canvas.addEventListener('lostpointercapture',()=>releaseTouch(role,s.lastPoint,0));
  }
  async function onControl(action,role){
    if(action==='mirror-on'){await startMirror(role);return;}
    if(role==='emitter'&&action==='mirror-diag'){
      const ok=await api.send(role,'UI GET');
      if(!ok)info(role,'Cannot read device UI state. Use R7 firmware at 115200.');
      return;
    }
    if(action==='mirror-frame'){
      if(!mirrors[role].active)await startMirror(role);
      else{
        const s=mirrors[role];
        if(s.waiting){
          info(role,'Current LCD frame is still being transmitted.');return;
        }
        cancelFrameTimers(role);
        await requestMirrorFrame(role);
      }
      return;
    }
    if(action==='mirror-off'){await stopMirror(role,true);return;}
    if(action==='connect'){
      info(role,'Choose the '+names[role]+' Windows COM port…');
      await api.connect(role);refreshState();
      const state=api.getState();
      // Emitters sync automatically; receivers stay on LOG by default.
      if(role==='emitter'&&isConnected(role,state))await startMirror(role);
      return;
    }
    if(action==='disconnect'){
      await stopMirror(role,true);await api.disconnect(role);
      refreshState();return;
    }
    if(role==='emitter'&&action==='run'){
      await api.runSelected();
      info(role,'RUN requested; transmitter TRUTH BEGIN confirms acceptance.');
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
      'Command unavailable, device offline, or blocked during automated testing.');
  }
  rail.addEventListener('click',event=>{
    const btn=event.target.closest('button[data-action][data-role]');
    if(!btn||btn.disabled)return;
    void onControl(btn.dataset.action,btn.dataset.role).catch(err=>
      info(btn.dataset.role,'Operation failed: '+err.message));
  });
  get('dc-export').addEventListener('click',()=>api.exportCsv());
  window.addEventListener('sqe:serial',e=>appendLine(e.detail.role,e.detail.line));
  function renderEmitter(){
    const s=mirrors.emitter;
    if(!s.active||!s.ready||!local.model)return;
    if(Date.now()-local.receivedAt>2500){
      get('dc-canvas-emitter').hidden=true;
      get('dc-placeholder-emitter').hidden=false;
      s.lastFrame=false;
      mirrorNote('emitter','Device UI state stale; check USB and power.');
      return;
    }
    const canvas=get('dc-canvas-emitter');
    const ctx=canvas.getContext('2d',{alpha:false});
    if(!ctx)return;
    const now=local.model.uptime+(Date.now()-local.receivedAt);
    if(!native.render(ctx,local.model,local.catalog,now))return;
    canvas.hidden=false;
    get('dc-placeholder-emitter').hidden=true;
    s.lastFrame=true;
  }
  function onNativeUi({role,line}){
    if(role!=='emitter')return;
    const s=mirrors.emitter;
    if(line==='UI READY 1 320 240 MODEL'){
      if(!s.active)return;
      s.ready=true;
      if(s.handshake!==null){clearTimeout(s.handshake);s.handshake=null;}
      mirrorNote('emitter','Connected · rendering UI locally from device state');
      if(s.animation===null)s.animation=setInterval(renderEmitter,120);
      void api.send('emitter','UI GET');
      return;
    }
    if(/^UI ERROR/.test(line)){info('emitter',line);return;}
    const state=native.parse(line);
    if(!state||!s.active||!s.ready)return;
    local.model=state;local.receivedAt=Date.now();
    mirrorNote('emitter','PC-DRAWN UI · '+state.screen+' · '+state.selected+
      ' · '+state.duration+'s · device '+state.uptime+'ms');
    renderEmitter();
  }
  window.addEventListener('sqe:catalog',e=>{
    local.catalog=native.prepare(e.detail.cases);
    renderEmitter();
  });
  window.addEventListener('sqe:native-ui',e=>onNativeUi(e.detail));
  window.addEventListener('sqe:mirror',e=>onMirror(e.detail));
  window.addEventListener('sqe:mirror-paused',e=>{
    const detail=e.detail||{};
    if(detail.role!=='receiver')return;
    // The detection runner owns the serial MIRROR OFF. Hide authentic pixels
    // immediately and cancel outstanding frame timers; don't send it twice.
    resetRole('receiver');
    mirrorNote('receiver',detail.reason||
      'Receiver LCD paused to collect exact-MAC LOG evidence. Restore after testing.');
    info('receiver','Receiver mirroring paused automatically. '+ 
      'The SquachEmit LCD stays live and RF transmission can proceed.');
  });
  window.addEventListener('sqe:snapshot',e=>snapshot(e.detail));
  window.addEventListener('sqe:truth',e=>truth(e.detail));
  window.addEventListener('sqe:state',refreshState);
  window.addEventListener('beforeunload',()=>{
    for(const role of ['receiver','emitter']){
      if(mirrors[role].touchDown&&isConnected(role,api.getState()))
        sendTouch(role,mirrors[role].lastPoint,'UP');
      resetRole(role);
    }
  });
  refreshState();
})();
