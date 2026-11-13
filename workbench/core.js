/* SquachEmit Detection Workbench — pure, dependency-free trial scoring/parsing.
 * TRUTH records describe transmitter intentions, never receiver detections. */
(function (root, factory) {
  var core = factory();
  if (typeof module !== 'undefined' && module.exports) module.exports = core;
  if (root) root.SQEWorkbenchCore = core;
})(typeof window === 'undefined' ? null : window, function () {
  'use strict';
  var CSV_HEADER = ['run_id','receiver','receiver_sha','emitter_sha','id',
    'emission','window','observed','latency_ms','notes'];
  var LABELS = ['NONE','FLOCK','DRONE','AXON','RAVEN','AIRTAG','TILE',
    'SAMSUNG_TAG','GOOGLE_TAG','META','IBEACON','SKIMMER','HACKER',
    'ALPR','CAMERA','RING','DEAUTH','EVILTWIN','UNKNOWN'];
  var HEX_SHA = /^[0-9a-f]{7,40}$/i;
  var CASE_ID = /^[A-Z0-9][A-Z0-9_-]{1,100}$/;
  function trim(x) { return x == null ? '' : String(x).trim(); }
  function parseTruth(line) {
    if (typeof line !== 'string' || !line.startsWith('TRUTH,') ||
        line.length > 16384) return null;
    var data;
    try { data = JSON.parse(line.slice(6)); }
    catch (_) { return null; }
    if (!data || typeof data !== 'object' ||
        !['BEGIN','END','ERROR'].includes(data.event) ||
        !CASE_ID.test(data.id) ||
        !Number.isSafeInteger(data.seq) || data.seq < 0 ||
        !Number.isSafeInteger(data.ms) || data.ms < 0) return null;
    if (data.expected != null && !/^[A-Z_]{1,40}$/.test(data.expected))
      return null;
    return { event:data.event, seq:data.seq, ms:data.ms,
      id:data.id, expected:data.expected || '',
      status:trim(data.status).toLowerCase(),
      carrier:trim(data.carrier).toUpperCase(), mac:trim(data.mac) };
  }
  // SquachWatch (master, 2026-11-11) responds to a serial "LOG" request:
  // [log] N rows, newest first
  // row,type,mac,rssi,hits,state,when
  // 0,DRONE,04:a8:5a:12:34:56,-48,1,here,+0:11
  // This is a *snapshot*, NOT a push stream of individual detections.
  var WATCH_MAC = /^[0-9a-f]{2}(?::[0-9a-f]{2}){5}$/i;
  function normalizeMac(value) {
    var mac=trim(value).toUpperCase();
    return WATCH_MAC.test(mac)?mac:'';
  }
  function parseWatchLogCount(line) {
    var m=/^\[log\] (0|[1-9][0-9]{0,2}) rows, newest first$/.exec(trim(line));
    if(!m)return null;
    var n=Number(m[1]);
    return n<=64?n:null;
  }
  function parseWatchLogRow(line) {
    if(typeof line!=='string'||line.length>320)return null;
    var x=line.trim().split(',');
    if(x.length!==7||!/^(0|[1-9]\d{0,2})$/.test(x[0]))return null;
    var type=trim(x[1]).toUpperCase().replace(/ /g,'');
    var mac=normalizeMac(x[2]);
    if(!mac||!LABELS.includes(type)||type==='NONE'||type==='UNKNOWN')return null;
    if(!/^-?\d{1,3}$/.test(x[3])||!/^\d{1,6}$/.test(x[4]))return null;
    var rssi=Number(x[3]),hits=Number(x[4]),state=trim(x[5]);
    if(rssi< -127||rssi>0||hits<0||hits>65535||
      !['here','gone','KEPT'].includes(state)||
      x[6].length>50||!x[6].trim())return null;
    return {index:Number(x[0]),type:type,mac:mac,rssi:rssi,hits:hits,
      state:state,when:trim(x[6])};
  }
  function newWatchLogParser() {
    var pending=null;
    return {
      start:function(requestedAt) {
        if(pending)return false;
        pending={requestedAt:requestedAt,expected:null,header:false,rows:[]};
        return true;
      },
      cancel:function(){pending=null;},
      waiting:function(){return !!pending;},
      read:function(line,now) {
        if(!pending)return null;
        // A new banner restarts only a response to our own pending request.
        var count=parseWatchLogCount(line);
        if(count!==null){
          pending.expected=count;
          pending.header=false;
          pending.rows=[];
          return null;
        }
        if(pending.expected===null)return null;
        if(!pending.header){
          if(trim(line)!=='row,type,mac,rssi,hits,state,when')return null;
          pending.header=true;
        }else{
          var row=parseWatchLogRow(line);
          if(!row||row.index!==pending.rows.length){
            pending=null;return null; // malformed or interleaved response: fail closed
          }
          pending.rows.push(row);
        }
        if(pending.rows.length===pending.expected){
          var completed={requestedAt:pending.requestedAt,finishedAt:now,
            rows:pending.rows};
          pending=null;
          return completed;
        }
        return null;
      }
    };
  }
  // A matching *type* alone is never proof: we require the exact MAC the
  // emitter reported via TRUTH BEGIN, a complete pre-run receiver baseline,
  // a later LOG snapshot, and a genuinely new/reappearing log entry.
  function matchWatchDetection(trial,before,after) {
    if(!trial||!before||!after||!normalizeMac(trial._mac))return null;
    if(!Number.isFinite(trial._startedAt)||
       before.finishedAt>trial._startedAt||
       after.requestedAt<trial._startedAt||
       trial._startedAt-before.finishedAt>5000)return null;
    var mac=normalizeMac(trial._mac);
    var recent=(after.rows||[]).filter(function(x){
      return x.mac===mac && x.state==='here';
    }).filter(function(x){
      var old=(before.rows||[]).find(function(y){
        return y.mac===x.mac && y.type===x.type;
      });
      // Ignore an unchanged preexisting device; it might have been in the
      // room before the test. Reappearance increments hits.
      return !old || (x.hits>old.hits && old.state!=='KEPT');
    });
    if(!recent.length)return null;
    var types=new Set(recent.map(function(x){return x.type;}));
    if(types.size>1)return {ambiguous:true,mac:mac,types:Array.from(types)};
    var item=recent[0];
    return {observed:item.type,mac:item.mac,rssi:item.rssi,
      hits:item.hits,when:item.when,requestedAt:after.requestedAt,
      finishedAt:after.finishedAt};
  }
  function status(row, catalog) {
    var emission = trim(row.emission).toLowerCase();
    var windowState = trim(row.window).toLowerCase();
    var observed = trim(row.observed).toUpperCase();
    if (emission === 'failed' || windowState === 'incomplete') return 'EXCLUDED';
    if (emission !== 'ok' || windowState !== 'complete' ||
        !observed || !HEX_SHA.test(trim(row.receiver_sha)) ||
        !HEX_SHA.test(trim(row.emitter_sha))) return 'UNTESTED';
    var c = catalog[trim(row.id)];
    if (!c || !trim(c.expected)) return 'UNTESTED';
    var expected = trim(c.expected).toUpperCase();
    if (expected === 'NONE') return observed === 'NONE' ? 'TN' : 'FP';
    if (observed === 'NONE') return 'FN';
    return observed === expected ? 'TP' : 'WRONG';
  }
  function pendingReason(row, catalog) {
    var result=status(row,catalog);
    if(result==='EXCLUDED')return 'Excluded: emission failed or listening window incomplete';
    if(result!=='UNTESTED')return '';
    if(trim(row.emission).toLowerCase()!=='ok')return 'Waiting for actual SquachEmit TRUTH END (ok)';
    if(trim(row.window).toLowerCase()!=='complete')return 'Waiting for a verified complete receiver window';
    if(!trim(row.observed))return 'No receiver observation recorded yet';
    var receiver=HEX_SHA.test(trim(row.receiver_sha));
    var emitter=HEX_SHA.test(trim(row.emitter_sha));
    if(!receiver && !emitter)return 'Missing/invalid SquachWatch and SquachEmit firmware SHAs';
    if(!receiver)return 'Missing/invalid SquachWatch firmware SHA';
    if(!emitter)return 'Missing/invalid SquachEmit firmware SHA';
    if(!catalog[trim(row.id)])return 'Unknown case ID';
    return 'Incomplete trial evidence';
  }
  function score(rows, catalog) {
    var count={TP:0,TN:0,FP:0,FN:0,WRONG:0,UNTESTED:0,EXCLUDED:0};
    var latency=[];
    (rows||[]).forEach(function (r) {
      var t=status(r,catalog); count[t]++;
      var ms=trim(r.latency_ms);
      if (t==='TP' && ms && isFinite(Number(ms)) && Number(ms)>=0)
        latency.push(Number(ms));
    });
    // A wrong-class alert is both a false negative and a false positive.
    var tp=count.TP, fp=count.FP+count.WRONG, fn=count.FN+count.WRONG,
        tn=count.TN;
    var precision=tp+fp ? tp/(tp+fp) : null;
    var recall=tp+fn ? tp/(tp+fn) : null;
    var specificity=tn+fp ? tn/(tn+fp) : null;
    latency.sort(function(a,b){return a-b;});
    var median=latency.length
      ? (latency[Math.floor((latency.length-1)/2)] +
         latency[Math.floor(latency.length/2)])/2 : null;
    return {count:count,total:rows.length,scored:tp+tn+count.FP+count.FN+count.WRONG,
      precision:precision,recall:recall,specificity:specificity,medianMs:median,
      latencyCount:latency.length};
  }
  function makeTrial(event, opts, catalog) {
    if (!event || event.event!=='BEGIN' || !catalog[event.id])
      throw new Error('A catalog-recognized TRUTH BEGIN record is required.');
    var o=opts||{};
    return {run_id:trim(o.runId),receiver:trim(o.receiver),
      receiver_sha:trim(o.receiverSha),emitter_sha:trim(o.emitterSha),
      id:event.id,emission:'pending',window:'',observed:'',
      latency_ms:'',notes:'Emitter reported BEGIN, seq '+event.seq+
      '; no receiver event has been verified.',seq:event.seq,
      started_ms:event.ms,ended_ms:null};
  }
  function applyEnd(row, event) {
    if (!row || !event || !['END','ERROR'].includes(event.event) ||
        row.id!==event.id || row.seq!==event.seq) return false;
    row.ended_ms=event.ms;
    // "ok" is emitter's own API status, not verification of RF reception.
    row.emission=(event.event==='END' && event.status==='ok')?'ok':'failed';
    row.notes='Emitter '+event.event+' ('+(event.status||'unspecified')+
      ') seq '+event.seq+'; receiver result requires explicit confirmation.';
    return true;
  }
  function csvEscape(value) {
    var s=trim(value);
    // Spreadsheet formula injection protection for user-supplied notes/IDs.
    if (/^[=+@\t]/.test(s) || /^-[^0-9]/.test(s)) s="'"+s;
    return /[",\r\n]/.test(s) ? '"'+s.replace(/"/g,'""')+'"' : s;
  }
  function exportCsv(rows) {
    var lines=[CSV_HEADER.join(',')];
    (rows||[]).forEach(function(r){
      lines.push(CSV_HEADER.map(function(k){
        // Python score_trials.py accepts blank/ok/failed; pending is an
        // internal UI state only, never a valid exported emission token.
        return csvEscape(k==='emission' && trim(r[k]).toLowerCase()==='pending'
          ? '' : r[k]);
      }).join(','));
    });
    return lines.join('\r\n')+'\r\n';
  }
  function parseCsv(data) {
    if (typeof data!=='string'|| data.length>4000000)
      throw new Error('CSV must be under 4 MB.');
    var all=[],row=[],val='',quoted=false;
    var src=data.replace(/^\uFEFF/,'');
    for(var i=0;i<src.length;i++){
      var c=src[i];
      if(quoted){
        if(c==='"'&&src[i+1]==='"'){val+='"';i++;}
        else if(c==='"')quoted=false;
        else val+=c;
      }else if(c==='"'){if(val)throw new Error('Invalid quote position.');quoted=true;}
      else if(c===','){row.push(val);val='';}
      else if(c==='\n'||c==='\r'){
        if(c==='\r'&&src[i+1]==='\n')i++;
        row.push(val);val='';
        if(row.length>1||row[0])all.push(row);
        row=[];
      }else val+=c;
    }
    if(quoted)throw new Error('Unclosed CSV quote.');
    if(row.length||val){row.push(val);all.push(row);}
    if(all.length===0)throw new Error('Empty CSV.');
    var header=all.shift();
    CSV_HEADER.forEach(function(k){if(!header.includes(k))throw new Error('Missing CSV column '+k);});
    if(all.length>5000)throw new Error('Import limited to 5,000 trials.');
    return all.map(function(cells,i){
      if(cells.length!==header.length)throw new Error('CSV field count at row '+(i+2));
      var data={};
      CSV_HEADER.forEach(function(k){data[k]=trim(cells[header.indexOf(k)]);});
      if(data.emission && !['ok','failed'].includes(data.emission))
        throw new Error('Invalid emission at row '+(i+2));
      if(data.window && !['complete','incomplete'].includes(data.window))
        throw new Error('Invalid window at row '+(i+2));
      if(data.latency_ms && (!isFinite(Number(data.latency_ms))||
          Number(data.latency_ms)<0))
        throw new Error('Invalid latency at row '+(i+2));
      return data;
    });
  }

  // Friendly labels describe the synthetic fixture, not proof of actual hardware.
  // Technical case IDs and expected receiver classifications never change.
  function signalName(c) {
    if(!c||!c.id)throw Error('Case has no signal ID');
    var id=trim(c.id), desc=trim(c.description), expected=trim(c.expected);
    var device='',method='';
    // Explicit names for ALL fixture families, including negative controls.
    // These are recognizable test labels, never identity/authenticity claims.
    if(id.startsWith('OUI-')) {
      device=desc.split('|')[0].trim();
      method='synthetic manufacturer Wi-Fi MAC beacon; NO Remote ID';
    } else if(id.startsWith('RID-')) {
      if(id.startsWith('RID-NEG-')) {
        device='OpenDroneID';method=desc+' (negative control)';
      } else if(/^RID-(?:WIFI-)?DJI-/.test(id)) {
        device=desc.split('|')[0].trim();
        method='synthetic '+(id.startsWith('RID-WIFI-')?'Wi-Fi':'BLE')+' Remote ID';
      } else if(/^RID-(?:AUTEL|PARROT)-/.test(id)) {
        device=desc.split('|')[0].trim().replace(/^LAB /,'');
        method='synthetic BLE Remote ID';
      } else {
        device=id.startsWith('RID-WIFI-PARROT')?'Parrot ANAFI / OpenDroneID':'OpenDroneID';
        method=desc+' (synthetic '+(c.carrier==='WIFI'?'Wi-Fi':'BLE')+')';
      }
    } else if(id.startsWith('BLE-HACKER-')) {
      device='Flipper Zero';
      method=id==='BLE-HACKER-NAME'?'advertised BLE name':
        id==='BLE-HACKER-MFG'?'manufacturer ID':
        'service UUID 0x'+(id==='BLE-HACKER-UUID'?'3081':
          id.slice('BLE-HACKER-'.length));
    } else if(id.startsWith('WIFI-HACKER-')) {
      device=id.startsWith('WIFI-HACKER-PAGER')?'Wi-Fi Pineapple Pager':
        id==='WIFI-HACKER-PINE'?'Wi-Fi Pineapple':'ESP Deauther';
      method=desc;
    } else if(id.startsWith('BLE-FLOCK-')) {
      device='Flock Safety';method=desc;
    } else if(id.startsWith('WIFI-FLOCK')) {
      device='Flock Safety';method=desc;
    } else if(id.startsWith('BLE-META-')) {
      device=id==='BLE-META-LUXOTTICA'?'Luxottica / Ray-Ban':
        id==='BLE-META-SNAP'?'Snap Spectacles':
        id==='BLE-META-UUID'?'Ray-Ban Meta':'Meta';
      method=desc;
    } else if(id.startsWith('BLE-RAVEN')) {
      device='Raven';method=desc;
    } else if(id.startsWith('BLE-AIRTAG')) {
      device='Apple AirTag';method='synthetic Find My manufacturer payload';
    } else if(id.startsWith('BLE-SAMSUNG')) {
      device='Samsung SmartTag';method=desc;
    } else if(id.startsWith('BLE-TILE')) {
      device='Tile tracker';method=desc;
    } else if(id.startsWith('BLE-GOOGLE')) {
      device='Google Find My tracker';method='generic Eddystone UUID (negative control; NOT a tracker)';
    } else if(id.startsWith('BLE-IBEACON')) {
      device='Apple iBeacon';method=desc;
    } else if(id.startsWith('BLE-DRONE')) {
      device='OpenDroneID';method='synthetic BLE Remote ID';
    } else if(id.startsWith('BLE-AXON')||id.startsWith('WIFI-AXON')) {
      device='Axon body camera';method=desc;
    } else if(id.startsWith('BLE-SKIM')||id==='BLE-SKIMMER') {
      device=(c.text||id==='BLE-SKIMMER'&&'HC-05')||'Bluetooth serial module';
      method='generic Bluetooth module; not proof of card skimmer — '+desc;
    } else if(id.startsWith('BLE-NEG-')) {
      var negName={
        'BLE-NEG-APPLE':'Apple Find My / iBeacon',
        'BLE-NEG-UUID':'Unknown BLE device',
        'BLE-NEG-NAME':'Ordinary BLE device',
        'BLE-NEG-IBEACON-SHORT':'Apple iBeacon',
        'BLE-NEG-DRONE-UUID':'OpenDroneID',
        'BLE-NEG-META-ADJ':'Meta',
        'BLE-NEG-SNAP-ADJ':'Snap Spectacles',
        'BLE-NEG-PENGUIN-9':'Flock Penguin',
        'BLE-NEG-BARE9':'Flock Safety',
        'BLE-NEG-RAVEN-3600':'Raven',
        'BLE-NEG-FLIP-MFG':'Flipper Zero',
        'BLE-NEG-JDY16-EXTRA':'JDY-16 Bluetooth module'
      };
      device=negName[id]||'Generic BLE device';
      method=desc+' (negative control)';
    } else if(id.startsWith('WIFI-NEG')) {
      device=id.includes('PAGER')?'Wi-Fi Pineapple Pager':
        id.includes('FLOCK')?'Flock Safety':
        id.includes('AXON')?'Axon body camera':'Ordinary Wi-Fi access point';
      method=desc+' (negative control)';
    } else {
      // Fail closed rather than silently displaying an anonymous case
      // when the firmware catalog grows beyond the reviewed 172 names.
      throw Error('No device name mapped for signal '+id);
    }
    return device+' — '+method;
  }
  function batchPlan(all,scope) {
    if(!['all','BLE','WIFI'].includes(scope))throw Error('Invalid batch scope');
    return (all||[]).filter(function(c){return scope==='all'||c.carrier===scope;})
      .slice().sort(function(a,b){
        return signalName(a).localeCompare(signalName(b))||a.id.localeCompare(b.id);
      });
  }
  return {CSV_HEADER:CSV_HEADER,LABELS:LABELS,parseTruth:parseTruth,
    normalizeMac:normalizeMac,parseWatchLogCount:parseWatchLogCount,
    parseWatchLogRow:parseWatchLogRow,newWatchLogParser:newWatchLogParser,
    matchWatchDetection:matchWatchDetection,signalName:signalName,batchPlan:batchPlan,
    status:status,pendingReason:pendingReason,score:score,makeTrial:makeTrial,applyEnd:applyEnd,
    exportCsv:exportCsv,parseCsv:parseCsv};
});
