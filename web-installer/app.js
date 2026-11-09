(() => {
  'use strict';
  const params = new URLSearchParams(location.search);
  const lab = params.get('lab') === '1';
  const el = id => document.getElementById(id);
  const ui = Object.fromEntries(['board','build','buildDetails','confirm','notice','channelBadge','enterLab','exitLab','installArea','installLocked','installButton','downloadBin','serialState','connect','disconnect','serialLog','command','commandForm','send','clear','saveLog'].map(k => [k,el(k)]));
  let catalog = { builds: [] }, visible = [], selected = null;
  ui.channelBadge.textContent = lab ? 'LAB CHANNEL · UNVERIFIED' : 'PRODUCTION';
  ui.channelBadge.classList.toggle('lab', lab);
  ui.enterLab.hidden = lab;
  ui.exitLab.hidden = !lab;

  function notice(s, kind = '') { ui.notice.textContent = s; ui.notice.className = 'notice ' + kind; }
  function txt(node, value) { node.textContent = value; }
  function opt(value,label) { const o = document.createElement('option');o.value = value; o.textContent = label;return o; }
  function boardLabel(b){ return b === 'c5-waveshare-28' ? 'Waveshare ESP32-C5 · 2.8" touchscreen' : b === 's3-devkit' ? 'ESP32-S3 · DevKitC-1' : b; }
  function buildOptions() {
    const b = ui.board.value;
    const arr = visible.filter(x=>x.board === b).sort((a,c)=>c.created_at.localeCompare(a.created_at));
    ui.build.replaceChildren(opt('', arr.length ? 'Select a firmware build…' : 'No builds available for this board'));
    arr.forEach(entry => {
      const title = entry.title || 'Uncatalogued firmware — review changes';
      const version = entry.version_label ? ` · ${entry.version_label}` : '';
      // Short SHA is retained for exact firmware identification. Date remains
      // available below with the complete commit and source branch.
      const option = opt(entry.id, `${entry.channel.toUpperCase()} · ${title}${version} · ${entry.short_sha}`);
      option.title = entry.summary || title;
      ui.build.appendChild(option);
    });
    ui.build.disabled = !arr.length;
    ui.confirm.checked = false;
    selectBuild();
  }
  function selectBuild() {
    selected = visible.find(x => x.id === ui.build.value) || null;
    ui.confirm.disabled = !selected;
    ui.confirm.checked = false;
    if (!selected) {
      txt(ui.buildDetails, 'Choose a firmware build above. No device is modified until you explicitly connect and approve flashing.');
    } else {
      const title = selected.title || 'Uncatalogued firmware — review changes';
      const features = Array.isArray(selected.highlights)
        ? selected.highlights.filter(s => typeof s === 'string').slice(0, 7) : [];
      const fragments = [
        `FIRMWARE: ${title}`,
        `VERSION: ${selected.version_label || 'Unknown LAB version'}`,
        `WHAT CHANGED: ${selected.summary || 'No reviewed feature summary exists for this commit. Check the CI run before flashing.'}`
      ];
      if (features.length) fragments.push('FEATURES:\n' + features.map(x => '  • ' + x).join('\n'));
      if (selected.notes_verified_for_sha === false)
        fragments.push('NOTE: This SHA has no curated changelog. Review the source commit.');
      fragments.push(
        `CHANNEL: ${selected.channel.toUpperCase()} / HARDWARE-UNVERIFIED`,
        `DEVICE: ${boardLabel(selected.board)}`,
        `COMMIT: ${selected.sha}`,
        `SOURCE: ${selected.branch}`,
        `BUILT: ${selected.created_at.replace('T',' ').slice(0,19)} UTC`,
        `SHA256: ${selected.sha256}`,
        'Installation uses bootloader, partitions, OTA selection and application. Existing user data may change.'
      );
      txt(ui.buildDetails, fragments.join('\n'));
      ui.buildDetails.style.whiteSpace = 'pre-line';
    }
    updateInstall();
  }
  function updateInstall() {
    const allowed = !!selected && ui.confirm.checked;
    ui.installArea.hidden = !allowed;
    ui.installLocked.hidden = allowed;
    ui.downloadBin.hidden = !selected;
    if (selected) {
      ui.installButton.setAttribute('manifest', `./${selected.manifest}`);
      ui.downloadBin.href = `./${selected.application}`;
      ui.downloadBin.download = `${selected.id}.bin`;
    } else ui.installButton.setAttribute('manifest','./manifest-unavailable.json');
    if (allowed) txt(ui.installLocked, '');
  }
  async function loadCatalog() {
    try {
      const response = await fetch('./catalog.json', {cache:'no-store'});
      if (!response.ok) throw Error(`HTTP ${response.status}`);
      catalog = await response.json();
      if (!Array.isArray(catalog.builds)) throw Error('Invalid firmware catalog');
      visible = catalog.builds.filter(entry => lab || entry.channel === 'stable');
      const boards = [...new Set(visible.map(x=>x.board))];
      ui.board.replaceChildren(opt('',boards.length?'Choose a target board…':'No approved production firmware yet'));
      boards.forEach(b => ui.board.appendChild(opt(b,boardLabel(b))));
      ui.board.disabled = !boards.length;
      if (lab) {
        notice(`LAB mode enabled. ${visible.length} build(s) loaded. These builds compile in CI but have NOT passed physical RF/display verification.`, 'warn');
        if (boards.length) { ui.board.value = boards.includes('c5-waveshare-28')?'c5-waveshare-28':boards[0]; buildOptions(); }
      } else if (!boards.length) {
        notice('No hardware-approved production release yet. To use development firmware, open the LAB channel using ENABLE LAB BUILDS.', 'warn');
      } else notice(`${visible.length} approved production build(s) available.`);
      if (!navigator.serial || !window.isSecureContext) notice('Browser serial requires HTTPS and Web Serial (desktop Chrome or Edge). Firmware catalog is available for viewing.', 'warn');
    } catch (e) { notice(`Firmware catalog unavailable: ${e.message}. Check the GitHub Actions Pages deployment.`, 'error'); }
  }
  ui.board.addEventListener('change', buildOptions);
  ui.build.addEventListener('change', selectBuild);
  ui.confirm.addEventListener('change', updateInstall);
  loadCatalog();

  // Serial console. All serial operations originate from explicit user gestures;
  // it never starts or resumes the transmitter when the page opens.
  let port = null, reader = null, reading = null, writer = null;
  const encoder = new TextEncoder(), decoder = new TextDecoder();
  let log = '';
  function append(data) {
    if (!data) return;
    log += data;
    if (log.length > 200000) log = log.slice(-165000);
    ui.serialLog.textContent = log;
    ui.serialLog.scrollTop = ui.serialLog.scrollHeight;
  }
  function setState(connected) {
    ui.serialState.textContent = connected ? 'CONNECTED' : 'DISCONNECTED';
    ui.serialState.classList.toggle('connected', connected);
    ui.connect.disabled = connected;
    ui.disconnect.disabled = !connected;
    ui.command.disabled = !connected;
    ui.send.disabled = !connected;
    document.querySelectorAll('[data-cmd]').forEach(b=>b.disabled=!connected);
  }
  async function monitorLoop(p) {
    const r = p.readable.getReader(); reader = r;
    try {
      for (;;) {
        const {value,done} = await r.read();
        if (done) break;
        append(decoder.decode(value,{stream:true}));
      }
    } catch (e) { if (port === p) append(`\n[serial read stopped: ${e.message}]\n`); }
    finally { try {r.releaseLock();} catch (_) {} if(reader===r) reader=null; }
  }
  async function disconnect() {
    const p=port;port=null;setState(false);
    try { if(reader) await reader.cancel(); } catch (_) {}
    try { if(reading) await reading; } catch (_) {}
    try { if(writer) {writer.releaseLock();writer=null;} } catch (_) {}
    try { if(p) await p.close(); } catch (e) {append(`[port close: ${e.message}]\n`);}
  }
  ui.connect.addEventListener('click', async()=>{
    if (!('serial' in navigator) || !window.isSecureContext) {append('Web Serial unavailable. Use desktop Chrome or Edge over HTTPS.\n');return;}
    try {
      port=await navigator.serial.requestPort();
      await port.open({baudRate:115200});
      setState(true);
      append('\n[serial connected at 115200 baud; type HELP]\n');
      reading=monitorLoop(port);
    } catch(e) {append(`[serial connect: ${e.message}]\n`);if(port) await disconnect();}
  });
  ui.disconnect.addEventListener('click',disconnect);
  async function send(cmd) {
    if (!port || !cmd) return;
    try {
      writer=port.writable.getWriter();
      await writer.write(encoder.encode(cmd+'\n'));
      writer.releaseLock();writer=null;
      append(`\n> ${cmd}\n`);
    } catch(e) {append(`[serial write: ${e.message}]\n`);try{writer?.releaseLock();}catch(_){}writer=null;}
  }
  document.querySelectorAll('[data-cmd]').forEach(b=>b.addEventListener('click',()=>send(b.dataset.cmd)));
  ui.commandForm.addEventListener('submit',e=>{e.preventDefault();const cmd=ui.command.value.trim();if(cmd)send(cmd);ui.command.value='';});
  ui.clear.addEventListener('click',()=>{log='';ui.serialLog.textContent='';});
  ui.saveLog.addEventListener('click',()=>{
    const blob=new Blob([log],{type:'text/plain'});
    const url=URL.createObjectURL(blob);const link=document.createElement('a');
    link.href=url;link.download=`squachemit-truth-${new Date().toISOString().replace(/[:.]/g,'-')}.txt`;
    link.click();setTimeout(()=>URL.revokeObjectURL(url),1000);
  });
  window.addEventListener('beforeunload',()=>{ if(reader) reader.cancel().catch(()=>{}); });
  setState(false);
})();
