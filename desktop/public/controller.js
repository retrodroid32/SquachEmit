/* Windows SquachEmit + SquachWatch control desk.
 * Pixel canvases are decoded from real USB MIRROR frame rows only.
 * No synthetic replica screens, inferred receiver signals or network video.
 */
(function(){
  'use strict';
  if(new URLSearchParams(location.search).get('desktop')!=='1')return;
  const api=window.SQEWorkbenchDesktop,codec=window.SQEMirrorCodec;
  if(!api||!codec)throw Error('Windows USB workbench bridge/mirror decoder missing');
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
  const cameras={receiver:null,emitter:null};
  const terminals={receiver:'',emitter:''};
  const mirrors={};
  for(const role of ['receiver','emitter']){
    mirrors[role]={active:false,waiting:false,lastFrame:false,failures:0,
      assembler:new codec.MirrorAssembler(),timeout:null,next:null,
      touchDown:false,touchAt:0,touchTimer:null,lastPoint:{x:0,y:0},
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
      '</div><video id="dc-video-'+role+'" autoplay muted playsinline hidden></video></div>'+
      '<div class="dc-frame-meta" id="dc-frame-'+role+'" role="status">'+
      'LCD framebuffer: not connected</div>'+
      '<div class="dc-mirror-actions">'+
      '<button type="button" data-role="'+role+'" data-action="mirror-on">Mirror live</button>'+
      '<button type="button" data-role="'+role+'" data-action="mirror-frame">Get frame</button>'+
      '<button type="button" data-role="'+role+'" data-action="mirror-off">Stop</button></div>'+
      '<div class="dc-displaymode">Actual decoded USB LCD pixels, not a recreated UI. '+
      'Click, hold or drag on the displayed screen to use the device touchscreen. '+
      'The receiver requires SquachWatch LAB PR #225; both firmware mirrors require physical verification.</div>'+
      (emitter?
        '<div class="dc-baud"><label for="dc-baud-select">Emitter USB baud</label>'+
        '<div class="dc-baud-row"><select id="dc-baud-select">'+
        '<option value="115200">115,200 (boot default)</option>'+
        '<option value="2000000">2,000,000 (LCD frames)</option></select>'+
        '<button type="button" data-role="emitter" data-action="set-baud">Use for next connect</button></div>'+
        '<button type="button" data-role="emitter" data-action="switch-baud">Switch device UART to 2 Mbps</button>'+
        '<small>For SquachEmit CYD 3.2 only: first connect at 115,200, switch the device UART, '+
        'disconnect, set 2,000,000 above, reconnect, then Mirror live. A reboot resets the UART. '+
        'No LCD mirror exists yet on the S3/C5 builds.</small></div>':'')+
      '<div class="dc-camera"><label for="dc-source-'+role+'">OPTIONAL PHYSICAL CAMERA</label>'+
      '<select id="dc-source-'+role+'"><option value="">Choose camera…</option></select>'+
      '<div class="dc-camera-actions">'+
      '<button type="button" data-action="refresh-cameras" data-role="'+role+'">Refresh cameras</button>'+
      '<button type="button" data-action="start-camera" data-role="'+role+'">Show video</button>'+
      '<button type="button" data-action="stop-camera" data-role="'+role+'">Stop video</button></div></div>'+
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
    '<h2>Live USB screens</h2><p>Two real-device LCD captures · pointer-to-touch control</p></div>'+
    '<button type="button" id="dc-export">Export CSV</button></div>'+
    markup('receiver')+markup('emitter')+
    '<div class="dc-footnote"><b>PHYSICAL VALIDATION REQUIRED.</b> Pixel streaming and '+
    'remote touchscreen actions require compatible LAB firmware. SquachWatch #225 '+
    'must be verified separately on both CYD sizes. SquachEmit CYD 3.2 uses experimental LCD readback. '+
    'The Workbench never presents serial telemetry as an actual screenshot.</div>';
  function info(role,message){get('dc-feedback-'+role).textContent=message;}
  function mirrorNote(role,message){get('dc-frame-'+role).textContent=message;}
  function cancelFrameTimers(role){
    const s=mirrors[role];
    if(s.timeout!==null)clearTimeout(s.timeout);
    if(s.next!==null)clearTimeout(s.next);
    s.timeout=null;s.next=null;
  }
  function resetRole(role){
    const s=mirrors[role];
    cancelFrameTimers(role);
    s.active=false;s.waiting=false;s.failures=0;
    s.assembler.reset();
    if(s.touchTimer!==null)clearInterval(s.touchTimer);
    s.touchTimer=null;s.touchDown=false;
  }
  async function stopMirror(role,send){
    const was=mirrors[role].active;
    resetRole(role);
    if(send&&was&&isConnected(role,api.getState())){
      const ok=await api.send(role,'MIRROR OFF');
      mirrorNote(role,ok?'USB mirror stopped; last real frame retained.':
        'USB mirror stop was not acknowledged by the local command path.');
    }else{
      mirrorNote(role,'USB mirror stopped; last real frame retained.');
    }
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
    const panel=get('dc-display-'+role);
    panel.style.aspectRatio=frame.width+' / '+frame.height;
    mirrorNote(role,'REAL LCD · '+frame.width+'×'+frame.height+' · '+frame.format+
      ' · frame '+frame.sequence+' · '+new Date().toLocaleTimeString());
  }
  async function requestMirrorFrame(role){
    const s=mirrors[role],state=api.getState();
    if(!s.active||s.waiting||!isConnected(role,state)||state.batchActive)return;
    s.waiting=true;
    const ok=await api.send(role,'MIRROR FRAME');
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
      mirrorNote(role,'No LCD push captured; retrying a requested frame ('+
        s.failures+'/3). The receiver only streams real physical redraws.');
      s.next=setTimeout(()=>void requestMirrorFrame(role),700);
    },role==='emitter'?16000:12000);
  }
  async function startMirror(role){
    const state=api.getState();
    if(state.batchActive){
      info(role,'Stop automated testing first. LCD streaming shares the USB serial channel.');
      return;
    }
    if(!isConnected(role,state)){
      info(role,'Connect the '+names[role]+' USB device before mirroring.');return;
    }
    if(role==='emitter'&&state.emitterBaud!==2000000){
      info(role,'SquachEmit CYD32 mirror requires 2 Mbps. Use the UART switch '+
        'and reconnect sequence shown below.');return;
    }
    const s=mirrors[role];
    resetRole(role);
    s.active=true;
    info(role,'Enabling opt-in USB mirroring…');
    const ok=await api.send(role,'MIRROR ON');
    if(!ok){resetRole(role);info(role,'MIRROR ON command rejected or unavailable.');return;}
    mirrorNote(role,'Waiting for authentic '+names[role]+' LCD pixels…');
    await requestMirrorFrame(role);
  }
  function onMirror({role,line}){
    if(!mirrors[role])return;
    const s=mirrors[role],result=s.assembler.feed(line);
    if(result.kind==='info'){
      if(/MIRROR READY/.test(result.message))mirrorNote(role,'Mirror enabled; requesting LCD draw…');
      return;
    }
    if(result.kind==='error'){
      cancelFrameTimers(role);s.waiting=false;s.failures++;
      info(role,result.message);
      if(s.active&&s.failures>=3){void stopMirror(role,true);return;}
      if(s.active)s.next=setTimeout(()=>void requestMirrorFrame(role),900);
      return;
    }
    if(!s.active)return; // Never display unsolicited screen bytes.
    if(result.kind==='begin'){
      mirrorNote(role,'Receiving real '+result.width+'×'+result.height+
        ' '+result.format+' LCD frame…');
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
      s.next=setTimeout(()=>void requestMirrorFrame(role),800);
    }
  }
  function connected(role,state){
    const yes=isConnected(role,state),el=get('dc-state-'+role);
    el.textContent=yes?'USB CONNECTED':'OFFLINE';
    el.classList.toggle('online',yes);
    if(!yes&&mirrors[role].active){
      resetRole(role);
      mirrorNote(role,'USB disconnected; last real frame retained.');
    }
    if(role==='emitter'&&document.activeElement!==get('dc-baud-select'))
      get('dc-baud-select').value=String(state.emitterBaud);
  }
  function refreshState(){
    const state=api.getState();
    connected('receiver',state);connected('emitter',state);
    const disabled=state.batchActive;
    rail.querySelectorAll('[data-action="run"],[data-action="scan-auto"],'+
      '[data-action="scan-active"],[data-action="scan-passive"],'+
      '[data-action="rotate"],[data-action="clock"],'+
      '[data-action="mirror-on"],[data-action="mirror-frame"],'+
      '[data-action="switch-baud"]').forEach(b=>{b.disabled=disabled;});
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
  async function refreshCameras(){
    if(!navigator.mediaDevices||!navigator.mediaDevices.enumerateDevices)return;
    try{
      const list=(await navigator.mediaDevices.enumerateDevices())
        .filter(d=>d.kind==='videoinput');
      for(const role of ['receiver','emitter']){
        const el=get('dc-source-'+role),old=el.value;
        el.replaceChildren();
        const option=document.createElement('option');
        option.value='';
        option.textContent=list.length?'Choose Windows camera…':'No cameras detected';
        el.appendChild(option);
        list.forEach((device,i)=>{
          const item=document.createElement('option');
          item.value=device.deviceId;
          item.textContent=device.label||'Camera '+(i+1);
          el.appendChild(item);
        });
        if(list.some(d=>d.deviceId===old))el.value=old;
      }
    }catch(err){info('receiver','Camera enumeration failed: '+err.message);}
  }
  function stopCamera(role){
    if(cameras[role])cameras[role].getTracks().forEach(t=>t.stop());
    cameras[role]=null;
    const video=get('dc-video-'+role);
    video.pause();video.srcObject=null;video.hidden=true;
    get('dc-lcd-'+role).hidden=false;
  }
  async function startCamera(role){
    const id=get('dc-source-'+role).value;
    if(!navigator.mediaDevices||!navigator.mediaDevices.getUserMedia){
      info(role,'Windows camera not available.');return;
    }
    if(!id){info(role,'Choose an actual Windows camera first.');return;}
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
      video.hidden=false;get('dc-lcd-'+role).hidden=true;
      info(role,'Live physical camera active. Video is local and not uploaded.');
      await refreshCameras();
    }catch(err){info(role,'Camera unavailable: '+err.message);}
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
    void api.send(role,'MIRROR TOUCH '+point.x+' '+point.y+' '+phase)
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
    if(remain)setTimeout(finish,remain);else finish();
  }
  for(const role of ['receiver','emitter']){
    const canvas=get('dc-canvas-'+role),s=mirrors[role];
    canvas.addEventListener('pointerdown',event=>{
      if(!s.active||!s.lastFrame||api.getState().batchActive)return;
      const p=touchPoint(role,event);
      if(!p)return;
      event.preventDefault();
      if(s.touchDown)releaseTouch(role,p,0);
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
    if(action==='refresh-cameras'){await refreshCameras();return;}
    if(action==='start-camera'){await startCamera(role);return;}
    if(action==='stop-camera'){stopCamera(role);return;}
    if(action==='mirror-on'){await startMirror(role);return;}
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
      await api.connect(role);refreshState();return;
    }
    if(action==='disconnect'){
      await stopMirror(role,true);await api.disconnect(role);
      refreshState();return;
    }
    if(role==='emitter'&&action==='set-baud'){
      const baud=Number(get('dc-baud-select').value);
      const ok=api.setEmitterBaud(baud);
      info(role,ok?'Next emitter USB connection: '+baud+' baud.':
        'Disconnect SquachEmit first to change the host COM baud.');
      return;
    }
    if(role==='emitter'&&action==='switch-baud'){
      const state=api.getState();
      if(!state.emitterConnected||state.emitterBaud!==115200){
        info(role,'Connect the CYD 3.2 emitter at 115200 before switching its UART.');
        return;
      }
      const ok=await api.send(role,'MIRROR BAUD 2000000');
      info(role,ok?'Device UART switch requested. Disconnect, set host baud '+
        'to 2000000, then reconnect to the same COM port.':
        'Device UART switch was rejected.');
      return;
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
  window.addEventListener('sqe:mirror',e=>onMirror(e.detail));
  window.addEventListener('sqe:snapshot',e=>snapshot(e.detail));
  window.addEventListener('sqe:truth',e=>truth(e.detail));
  window.addEventListener('sqe:state',refreshState);
  window.addEventListener('beforeunload',()=>{
    for(const role of ['receiver','emitter']){
      if(mirrors[role].touchDown)releaseTouch(role,mirrors[role].lastPoint,0);
      resetRole(role);
      stopCamera(role);
    }
  });
  refreshState();
  void refreshCameras();
})();
