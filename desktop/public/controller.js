/* Windows SquachEmit + SquachWatch control desk.
 * SquachEmit renders locally from UI STATE, never a TFT screenshot.
 * Receiver optional screenshot mode stays on the independent USB port.
 */
(function(){
  'use strict';
  if(new URLSearchParams(location.search).get('desktop')!=='1')return;
  const api=window.SQEWorkbenchDesktop,codec=window.SQEMirrorCodec,native=window.SQECydNative,
    watch=window.SQEWatchNativeV2;
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
  const receiverUi={enabled:false,ready:false,lastAt:0,screen:null,
    mode:'off',caps:false,model:watch?watch.create():null,lastListAt:0,listRequestedAt:0};
  const watchScreenNames=['BOOT','MAIN','ALERT','LOG','SETTINGS','DIARY','OUTFIT','SCAN','WATCH ALERT','DIAGNOSTICS','HUNT','COLOR CHECK','DETECTION FILTER','OUTFIT UNLOCK','POLICIES','POWER SAVER','PHONE','MESH MENU','MESH WARNING','MESH PHRASE','MESH COMPOSE','BEACON WARNING','SECURITY','LOCKED','PIN ENTRY','SQUAD','UPDATE','WIFI PASSWORD','STATUS LIGHT','NUDGE','SQUAD UPDATE','INVITE','DESK','WIFI NETWORKS','WIFI ADD','SYSTEM PROPERTIES','BINGO','DEX','ALERT RULES','WATCH LIST','HUNT LIST','SD FILES'];
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
      'aria-label="Device native UI state or authentic LCD pixels from '+names[role]+'; click or drag to touch" hidden></canvas>'+
      '<div class="dc-placeholder" id="dc-placeholder-'+role+'">'+
      '<div class="dc-placeholder-icon">▣</div>'+
      '<b>NO LIVE LCD FRAME</b><small>Connect USB and choose Start live view. '+
      'A compatible firmware response is required before showing a screen.</small></div>'+
      '</div></div>'+
      '<div class="dc-frame-meta" id="dc-frame-'+role+'" role="status">'+
      'LCD framebuffer: not connected</div>'+
      '<div class="dc-mirror-actions">'+
      '<button type="button" data-role="'+role+'" data-action="mirror-on">Start live view</button>'+
      '<button type="button" data-role="'+role+'" data-action="mirror-frame">Refresh view</button>'+
      '<button type="button" data-role="'+role+'" data-action="mirror-off">Stop live view</button></div>'+
      (emitter?'<button type="button" data-role="emitter" data-action="mirror-diag">Check device UI state</button>':
        '<div class="dc-mirror-actions"><button type="button" data-role="receiver" data-action="receiver-ui-on">Native v2 mode</button>'+
        '<button type="button" data-role="receiver" data-action="receiver-ui-off">Pixel fallback</button></div>'+
        '<div id="dc-receiver-ui" class="dc-frame-meta" role="status">Start live view = authentic LCD pixels. Native v2 mode = partial DATA visualization, not the physical screen.</div>')+
      '<div class="dc-displaymode">'+(emitter?
        'PC-drawn SquachEmit UI, synchronized with device navigation and RF state. No screenshots or 2Mbps switch. Click, hold or drag to operate the physical touchscreen.':
        'SquachWatch: Start live view requests authentic RGB332 LCD pixels. Native v2 mode is an optional simplified diagnostic view, NOT a reproduction of the device LCD.')+'</div>'+
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
    '<h2>Interactive device screens</h2><p>SquachEmit state model · SquachWatch physical LCD capture</p></div>'+
    '<button type="button" id="dc-export">Export CSV</button></div>'+
    markup('receiver')+markup('emitter')+
    '<div class="dc-footnote"><b>PHYSICAL LCD ACCURACY:</b> SquachEmit is PC-drawn from state. SquachWatch requires authentic MIRROR pixels for an identical screen; Native v2 is explicitly a partial data-only view. Receiver LOG remains the independent detection evidence.</div>';
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
    if(role==='receiver'&&receiverUi.model)receiverUi.model.reset();
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
      await api.send(role,(role==='emitter'||receiverUi.mode==='native'?'UI TOUCH ':'MIRROR TOUCH ')+s.lastPoint.x+' '+s.lastPoint.y+' UP');
    const priorMode=role==='receiver'?receiverUi.mode:null;
    resetRole(role);
    if(role==='receiver'){
      receiverUi.enabled=false;receiverUi.ready=false;receiverUi.mode='off';receiverUi.caps=false;
    }
    if(send&&was&&connected){
      const ok=await api.send(role,role==='emitter'||priorMode!=='pixel'?'UI OFF':'MIRROR OFF');
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
    if(role==='receiver'&&receiverUi.mode==='native'){
      if(!mirrors.receiver.active||!receiverUi.ready)return;
      await api.send('receiver','UI GET');
      if(receiverUi.screen===1)await api.send('receiver','UI COUNTS');
      if(receiverUi.screen===3)await api.send('receiver','UI LIST 0 6');
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
  async function startMirror(role,forcePixel=false){
    // The receiver's primary Live view must be the physical LCD, not a
    // fabricated counter grid. Native UI v2 remains an explicitly labeled
    // optional diagnostics mode until SquachWatch exports full view models.
    if(role==='receiver'&&!forcePixel)return startMirror('receiver',true);
    if(role==='receiver'&&receiverUi.mode==='native')await stopMirror('receiver',true);
    const state=api.getState();
    if(mirrors[role].active){
      info(role,mirrors[role].ready?'Live view already active. Use Refresh view.':
        'Waiting for firmware acknowledgment. Check the serial output.');
      return;
    }
    if(role==='emitter'){
      if(!isConnected('emitter',state)){
        info('emitter','Connect at 115200 to synchronize the native UI.');return;
      }
      const s=mirrors.emitter;
      resetRole('emitter');s.active=true;
      info('emitter','Requesting real navigation state (not LCD pixels)…');
      const ok=await api.send('emitter','UI ON');
      if(!ok){resetRole('emitter');info('emitter','UI ON failed.');return;}
      mirrorNote('emitter','Checking firmware UI STATE support at 115200 baud…');
      s.handshake=setTimeout(()=>{
        s.handshake=null;
        if(!s.active||s.ready)return;
        resetRole('emitter');
        const err='SquachEmit did not acknowledge UI ON. The EXE requires R7-compatible '+
          'SquachEmit firmware with UI READY/UI STATE support at 115200 baud. '+
          'Older public firmware cannot provide this native view.';
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
    if(role==='receiver')receiverUi.mode='pixel';
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
        'No MIRROR READY. SquachWatch requires firmware with the USB pixel-mirror '+
        'protocol (PR #225), an unlocked PIN and matching serial baud.';
      if(role==='receiver'){
        void api.send('receiver','MIRROR OFF'); // also return serial to LOG polling
      }
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
    if(!s.active||role==='receiver'&&receiverUi.mode!=='pixel')return; // Never display unsolicited screen bytes.
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
      if(role==='receiver'){
        receiverUi.mode='off';receiverUi.enabled=false;receiverUi.ready=false;receiverUi.caps=false;
      }
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
    rail.querySelectorAll('[data-action="mirror-on"],[data-action="mirror-frame"],[data-action="receiver-ui-on"]').forEach(b=>{
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
    if(/ERROR,unknown_command/.test(String(line))){
      if(role==='emitter'&&mirrors.emitter.active&&!mirrors.emitter.ready){
        resetRole('emitter');
        const msg='SquachEmit firmware rejected UI ON. This is a firmware/EXE '+
          'version mismatch, not a USB connection problem. Flash R7-compatible '+
          'SquachEmit LAB firmware with UI STATE support, then reconnect at 115200.';
        mirrorNote('emitter',msg);info('emitter',msg);
      }else if(role==='receiver'&&mirrors.receiver.active&&!mirrors.receiver.ready){
        resetRole('receiver');
        void api.send('receiver','MIRROR OFF');
        const msg='SquachWatch rejected MIRROR ON. Its installed firmware lacks '+
          'the required USB pixel-mirror command. The EXE cannot create a '+
          'real LCD view without compatible receiver firmware.';
        mirrorNote('receiver',msg);info('receiver',msg);
      }
    }
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
    void api.send(role,(role==='emitter'||receiverUi.mode==='native'?'UI TOUCH ':'MIRROR TOUCH ')+point.x+' '+point.y+' '+phase)
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
    if(role==='receiver'&&action==='receiver-ui-on'){
      await startWatchNative();return;
    }
    if(role==='receiver'&&action==='receiver-ui-off'){
      await stopMirror('receiver',true);
      await startMirror('receiver',true);return;
    }
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
      // Both devices now use the same explicit Start live view action.
      // Never spam unsupported UI ON after USB connect; require operator choice.
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
    const display=typeof canvas.closest==='function'?canvas.closest('.dc-display'):null;
    if(display)display.classList.toggle('dc-portrait',local.model.screen==='PORTRAIT');
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
    const activity=/^UI TXACT 1 (\d{1,10}) (\d{1,10}) (\d{1,10}) ([01]) (\d{1,10})$/.exec(line);
    if(activity){
      if(local.model&&s.active&&s.ready){
        local.model.txact={begins:Number(activity[1]),ends:Number(activity[2]),
          errors:Number(activity[3]),active:activity[4]==='1',duration:Number(activity[5])};
        renderEmitter();
      }return;
    }
    const rx=/^UI RXREPORT 1 ([A-Z0-9_-]{2,100}|NONE) ([A-Z0-9_-]{2,100}|NONE) (-?\d{1,3}) (VERIFIED|CANDIDATE|UNVERIFIED)$/.exec(line);
    if(rx){
      if(local.model&&s.active&&s.ready){
        local.model.rxreport={id:rx[1],classification:rx[2],rssi:Number(rx[3]),quality:rx[4]};
        renderEmitter();
      }return;
    }
    if(/^UI FILTER (?:BEGIN|ROW|END)/.test(line)){
      if(!s.active||!s.ready||!local.model)return;
      const b=/^UI FILTER BEGIN (\d{1,2}) (\d{1,3}) (\d{1,3})$/.exec(line);
      const row=/^UI FILTER ROW ([0-3]) ([A-Z0-9_-]{2,100})$/.exec(line);
      if(b){local.filterPending={letter:Number(b[1]),page:Number(b[2]),count:Number(b[3]),rows:[]};return;}
      if(row&&local.filterPending){local.filterPending.rows[Number(row[1])]=row[2];return;}
      if(line==='UI FILTER END'&&local.filterPending){
        local.model.filter=local.filterPending;local.filterPending=null;renderEmitter();
      }return;
    }
    const device=/^UI DEVICE 1 ([0-2]) ([0-2]) ([0-2]) ([01]) ([01]) (\d{1,3}) ([0-2]) ([0-2]) ([01]) ([01])$/.exec(line);
    if(device){
      if(local.model&&s.active&&s.ready){
        local.model.device={brightness:Number(device[1]),animation:Number(device[2]),
          timeout:Number(device[3]),inverted:device[4]==='1',pc:device[5]==='1',
          reset:Number(device[6]),theme:Number(device[8]),font:Number(device[9]),
          settingsMore:device[10]==='1'};
        local.model.helpPage=Number(device[7]);renderEmitter();
      }return;
    }
    const identity=/^UI IDENTITY 1 ([A-Z0-9_-]{2,100}|NONE) ((?:[A-Fa-f0-9]{2}:){5}[A-Fa-f0-9]{2}|UNKNOWN)$/.exec(line);
    if(identity){
      if(local.model&&s.active&&s.ready){local.model.identity={id:identity[1],mac:identity[2]};renderEmitter();}
      return;
    }
    const playlist=/^UI PLAYLIST 1 ([01]) ([01]) (\d{1,3}) (\d{1,3})$/.exec(line);
    if(playlist){
      if(local.model&&s.active&&s.ready){
        local.model.playlistRunning=playlist[1]==='1';
        local.model.playlistPaused=playlist[2]==='1';
        local.model.playlistIndex=Number(playlist[3]);
        local.model.playlistTotal=Number(playlist[4]);renderEmitter();
      }return;
    }
    const favorite=/^UI FAVORITE 1 ([A-Z0-9_-]{2,100}|NONE) ([01])$/.exec(line);
    if(favorite){
      if(local.model&&s.active&&s.ready){
        local.model.favorite=local.model.selected===favorite[1]&&favorite[2]==='1';
        renderEmitter();
      }
      return;
    }
    if(/^UI (?:FAVS|HISTORY) (?:BEGIN|ROW|END)/.test(line)){
      if(!s.active||!s.ready||!local.model)return;
      const type=line.startsWith('UI FAVS ')?'favorites':'history';
      const begin=/^UI (?:FAVS|HISTORY) BEGIN (\d{1,3}) (\d{1,3})$/.exec(line);
      const row=type==='favorites'?/^UI FAVS ROW ([0-3]) ([A-Z0-9_-]{2,100})$/.exec(line):
        /^UI HISTORY ROW ([0-3]) ([A-Z0-9_-]{2,100}) ([A-Za-z0-9_-]{1,20}) (\d{1,10})$/.exec(line);
      if(begin){local.pendingList={type,count:Number(begin[1]),page:Number(begin[2]),rows:[]};return;}
      if(row&&local.pendingList?.type===type){
        local.pendingList.rows[Number(row[1])]=type==='favorites'?row[2]:
          {id:row[2],status:row[3],uptime:Number(row[4])};return;
      }
      if(line==='UI '+(type==='favorites'?'FAVS':'HISTORY')+' END'&&local.pendingList?.type===type){
        local.model[type]=local.pendingList.rows;
        if(type==='favorites')local.model.favoriteCount=local.pendingList.count;
        local.pendingList=null;renderEmitter();
      }
      return;
    }
    const health=/^UI HEALTH 1 (\d{1,10}) (\d{1,10}) (\d{1,6}) (\d{1,10}) ([01])$/.exec(line);
    if(health){
      if(!s.active||!s.ready||!local.model)return;
      local.model.health={heap:Number(health[1]),largest:Number(health[2]),
        serialFree:Number(health[3]),uptime:Number(health[4]),tx:health[5]==='1'};
      renderEmitter();return;
    }
    const state=native.parse(line);
    if(!state||!s.active||!s.ready)return;
    if(local.model){
      if(local.model.health)state.health=local.model.health;
      if(local.model.favorites)state.favorites=local.model.favorites;
      if(local.model.history)state.history=local.model.history;
      if(local.model.favoriteCount!==undefined)state.favoriteCount=local.model.favoriteCount;
      if(local.model.selected===state.selected&&local.model.favorite!==undefined)state.favorite=local.model.favorite;
      if(local.model.device)state.device=local.model.device;
      if(local.model.filter)state.filter=local.model.filter;
      if(local.model.txact)state.txact=local.model.txact;
      if(local.model.rxreport)state.rxreport=local.model.rxreport;
      if(local.model.helpPage!==undefined)state.helpPage=local.model.helpPage;
      if(local.model.identity)state.identity=local.model.identity;
      state.pcControl=true;
      if(local.model.playlistRunning!==undefined){
        state.playlistRunning=local.model.playlistRunning;
        state.playlistPaused=local.model.playlistPaused;
        state.playlistIndex=local.model.playlistIndex;
        state.playlistTotal=local.model.playlistTotal;
      }
    }
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
  // Native v2: independently sourced partial MAIN and DEVICES model.
  function renderWatch(){
    if(receiverUi.mode!=='native'||!receiverUi.ready||!receiverUi.model)return;
    const model=receiverUi.model.snapshot();
    if(!model.state)return;
    if(model.state.kind===0){
      // Alerts and other unsupported overlays are TEMPORARY. Do not tear down
      // the v2 session: doing so prevents automatic recovery when MAIN returns.
      const canvas=get('dc-canvas-receiver');
      canvas.hidden=true;get('dc-placeholder-receiver').hidden=false;
      mirrors.receiver.lastFrame=false;
      mirrorNote('receiver','SquachWatch screen '+model.state.screen+
        ' is not represented by native v2. UI session remains active; '+
        'MAIN/DEVICES will return automatically. Use Pixel fallback for '+
        'authentic alert pixels (firmware readback required).');
      return;
    }
    if(Date.now()-receiverUi.lastAt>3000){
      get('dc-canvas-receiver').hidden=true;
      get('dc-placeholder-receiver').hidden=false;
      mirrors.receiver.lastFrame=false;
      mirrorNote('receiver','Native v2 UI STATE stale; check receiver USB, baud, PIN and firmware.');
      return;
    }
    const canvas=get('dc-canvas-receiver'),ctx=canvas.getContext('2d',{alpha:false});
    if(!ctx||!watch.render(ctx,model))return;
    const panel=get('dc-display-receiver');
    if(panel)panel.style.aspectRatio=model.state.width+' / '+model.state.height;
    canvas.hidden=false;get('dc-placeholder-receiver').hidden=true;
    mirrors.receiver.lastFrame=true;
    mirrorNote('receiver','NATIVE V2 PARTIAL · '+(model.state.kind===1?'MAIN COUNTS':'RAM DEVICES LOG')+
      ' · '+model.state.width+'×'+model.state.height+
      (model.state.privacy?' · PRIVACY':'')+' · not a pixel-perfect screenshot');
  }
  async function startWatchNative(){
    const state=api.getState();
    if(!state.receiverConnected||state.testActive){
      info('receiver','Connect SquachWatch USB and stop active detection testing first.');return;
    }
    if(!watch||!receiverUi.model){
      info('receiver','Native v2 renderer unavailable in this EXE build.');return;
    }
    if(mirrors.receiver.active)await stopMirror('receiver',true);
    resetRole('receiver');
    const s=mirrors.receiver;
    s.active=true;receiverUi.enabled=true;receiverUi.mode='negotiating';
    receiverUi.ready=false;receiverUi.caps=false;receiverUi.screen=null;
    receiverUi.lastListAt=0;receiverUi.listRequestedAt=0;
    mirrorNote('receiver','Checking SquachWatch UI CAPS v2 at connected receiver baud…');
    const ok=await api.send('receiver','UI HELLO');
    if(!ok){resetRole('receiver');receiverUi.mode='off';info('receiver','UI HELLO write failed.');return;}
    s.handshake=setTimeout(()=>{
      if(!s.active||receiverUi.ready)return;
      void fallbackWatch('Native v2 capabilities/READY not received; switching to authentic RGB332 LCD.');
    },6000);
  }
  async function fallbackWatch(reason){
    if(receiverUi.mode==='pixel')return;
    if(!api.getState().receiverConnected){resetRole('receiver');receiverUi.mode='off';return;}
    info('receiver',reason);
    await stopMirror('receiver',true);
    if(!api.getState().testActive)await startMirror('receiver',true);
  }
  function onReceiverUi({role,line}){
    if(role!=='receiver'||!receiverUi.model)return;
    const result=receiverUi.model.feed(line);
    if(result.kind==='error'){
      info('receiver','Native UI: '+result.reason);
      if(result.reason==='LOCKED'||result.reason==='DISABLED'){
        void stopMirror('receiver',false);return;
      }
      if(result.reason==='PRIVACY_CHANGED'||result.reason==='LIST_CHANGED'){
        void api.send('receiver','UI GET');return;
      }
      return;
    }
    if(receiverUi.mode==='negotiating'&&result.kind==='caps'){
      receiverUi.caps=true;
      void api.send('receiver','UI ON');
      return;
    }
    if(result.kind==='ready'&&receiverUi.caps&&mirrors.receiver.active){
      receiverUi.mode='native';receiverUi.ready=true;receiverUi.enabled=true;
      if(mirrors.receiver.handshake!==null){clearTimeout(mirrors.receiver.handshake);mirrors.receiver.handshake=null;}
      mirrorNote('receiver','SquachWatch native UI v2 connected · partial MAIN/LOG models');
      if(mirrors.receiver.animation===null)mirrors.receiver.animation=setInterval(renderWatch,450);
      get('dc-receiver-ui').textContent='Native v2 DATA VIEW (not the LCD): MAIN counters and DEVICES log. Unsupported screens resume automatically when MAIN returns; use Pixel fallback for authentic pixels.';
      void api.send('receiver','UI GET');
      return;
    }
    if(receiverUi.mode!=='native'||!receiverUi.ready)return;
    if(result.kind==='state'){
      const st=result.state;
      const changed=receiverUi.screen!==st.screen;
      receiverUi.screen=st.screen;receiverUi.lastAt=Date.now();
      if(st.kind===0){
        // Keep consuming UI STATE during transient alerts and resume native
        // MAIN automatically when kind=1 returns. No manual Start click.
        renderWatch();
        return;
      }
      renderWatch();
      if(changed&&st.kind===1)void api.send('receiver','UI COUNTS');
      if(st.kind===2&&(changed||Date.now()-receiverUi.lastListAt>=3500)&&
         Date.now()-receiverUi.listRequestedAt>=2500){
        receiverUi.listRequestedAt=Date.now();
        void api.send('receiver','UI LIST 0 6');
      }
      return;
    }
    if(result.kind==='counts'||result.kind==='list'){
      renderWatch();
      if(result.kind==='list')receiverUi.lastListAt=Date.now();
    }
    if(result.kind==='status')get('dc-receiver-ui').textContent=result.text;
  }
  window.addEventListener('sqe:receiver-ui',e=>onReceiverUi(e.detail));
  window.addEventListener('sqe:mirror',e=>onMirror(e.detail));
  window.addEventListener('sqe:mirror-paused',e=>{
    const detail=e.detail||{};
    if(detail.role!=='receiver')return;
    // The detection runner owns the serial MIRROR OFF. Hide authentic pixels
    // immediately and cancel outstanding frame timers; don't send it twice.
    resetRole('receiver');
    receiverUi.mode='off';receiverUi.enabled=false;receiverUi.ready=false;receiverUi.caps=false;
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
