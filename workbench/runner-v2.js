/* SquachEmit Test Suite 2.0 — pure, side-effect-free checkpoint contract.
 * No serial access. Restoring a checkpoint NEVER authorizes radio transmission.
 * The operator must explicitly confirm a new run in the Workbench.
 */
(function(root,factory){
  const api=factory();
  if(typeof module==='object'&&module.exports)module.exports=api;
  if(root)root.SQETestSuiteV2=api;
})(typeof window==='undefined'?null:window,function(){
  'use strict';
  const MAX_CASES=172;
  const CASE_RE=/^[A-Z0-9][A-Z0-9_-]{1,100}$/;
  const VERSION=1;
  const KEY='sqe-test-suite-v2-checkpoint';
  function createSnapshot(batch) {
    if(!batch||!Array.isArray(batch.queue)||!batch.queue.length||
       batch.queue.length>MAX_CASES)throw Error('Invalid test queue');
    const ids=batch.queue.map(c=>typeof c==='string'?c:c&&c.id);
    if(ids.some(id=>typeof id!=='string'||!CASE_RE.test(id)))throw Error('Invalid case ID');
    const index=Number(batch.index),completed=Number(batch.completed);
    if(!Number.isInteger(index)||index<0||index>ids.length||
       !Number.isInteger(completed)||completed<0||completed>index)
      throw Error('Invalid checkpoint progress');
    const duration=Number(batch.duration),gap=Number(batch.gap);
    if(!Number.isInteger(duration)||duration<3||duration>30||
       !Number.isInteger(gap)||gap<3||gap>30)
      throw Error('Invalid timing');
    return {version:VERSION,ids,index,completed,duration,gap,
      // No radio consent, hardware handles, receiver evidence or auto-resume flags.
      failures:Math.max(0,Math.min(1000,Number(batch.failures)||0)),
      unverified:Math.max(0,Math.min(1000,Number(batch.unverified)||0))};
  }
  function validateSnapshot(snapshot,knownIds) {
    if(!snapshot||snapshot.version!==VERSION||!Array.isArray(snapshot.ids)||
       snapshot.ids.length<1||snapshot.ids.length>MAX_CASES||
       !Array.isArray(knownIds))return null;
    const known=new Set(knownIds);
    if(snapshot.ids.some(id=>typeof id!=='string'||!CASE_RE.test(id)||!known.has(id)))
      return null;
    if(!Number.isInteger(snapshot.index)||snapshot.index<0||
       snapshot.index>=snapshot.ids.length||
       !Number.isInteger(snapshot.completed)||snapshot.completed<0||
       snapshot.completed>snapshot.index)return null;
    try {
      return createSnapshot({queue:snapshot.ids,index:snapshot.index,
        completed:snapshot.completed,duration:snapshot.duration,gap:snapshot.gap,
        failures:snapshot.failures,unverified:snapshot.unverified});
    }catch(_){return null;}
  }
  function save(storage,batch) {
    const data=createSnapshot(batch);
    storage.setItem(KEY,JSON.stringify(data));
    return data;
  }
  function read(storage,knownIds) {
    try {
      const raw=storage.getItem(KEY);
      if(!raw||raw.length>30000)return null;
      return validateSnapshot(JSON.parse(raw),knownIds);
    }catch(_){return null;}
  }
  function clear(storage) {
    try{storage.removeItem(KEY);}catch(_){}
  }
  return Object.freeze({KEY,createSnapshot,validateSnapshot,save,read,clear});
});
