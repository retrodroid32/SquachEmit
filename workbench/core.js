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
      lines.push(CSV_HEADER.map(function(k){return csvEscape(r[k]);}).join(','));
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
      if(data.emission && !['ok','failed','pending'].includes(data.emission))
        throw new Error('Invalid emission at row '+(i+2));
      if(data.window && !['complete','incomplete'].includes(data.window))
        throw new Error('Invalid window at row '+(i+2));
      if(data.latency_ms && (!isFinite(Number(data.latency_ms))||
          Number(data.latency_ms)<0))
        throw new Error('Invalid latency at row '+(i+2));
      return data;
    });
  }
  return {CSV_HEADER:CSV_HEADER,LABELS:LABELS,parseTruth:parseTruth,
    status:status,score:score,makeTrial:makeTrial,applyEnd:applyEnd,
    exportCsv:exportCsv,parseCsv:parseCsv};
});
