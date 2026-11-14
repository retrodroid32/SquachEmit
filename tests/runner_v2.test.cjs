'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const v2=require('../workbench/runner-v2.js');
const ids=Array.from({length:172},(_,i)=>'BLE-CASE-'+String(i+1).padStart(3,'0'));
const batch=()=>({queue:ids.map(id=>({id})),index:11,completed:11,
  duration:8,gap:5,failures:1,unverified:2});
function store(){const map=new Map();return {getItem:k=>map.get(k)||null,
  setItem:(k,v)=>map.set(k,v),removeItem:k=>map.delete(k)};}
test('valid 172-case checkpoint round trip does not hold consent or radio handles',()=>{
  const s=store();v2.save(s,batch());const x=v2.read(s,ids);
  assert.equal(x.ids.length,172);assert.equal(x.index,11);
  assert.deepEqual(Object.keys(x).sort(),
    ['completed','duration','failures','gap','ids','index','unverified','version']);
  assert.equal('active' in x,false);assert.equal('authorized' in x,false);
  assert.equal('receiverEvidence' in x,false);
});
test('reject unknown, malformed, missing and non-current catalogs',()=>{
  const s=store();v2.save(s,batch());
  assert.equal(v2.read(s,ids.slice(0,50)),null);
  const x=v2.createSnapshot(batch());x.ids[4]='../../not-an-id';
  assert.equal(v2.validateSnapshot(x,ids),null);
  s.setItem(v2.KEY,'{broken json');assert.equal(v2.read(s,ids),null);
});
test('reject invalid durations, oversized queues and impossible progress',()=>{
  for(const patch of [{index:173},{completed:12},{duration:2},{gap:31},
    {index:-1},{queue:[]}]) {
    assert.throws(()=>v2.createSnapshot({...batch(),...patch}));
  }
  assert.throws(()=>v2.createSnapshot({...batch(),queue:[...ids,'BLE-EXTRA']}));
});
test('finished or corrupt checkpoints never automatically restart',()=>{
  const s=store();const b=batch();b.index=172;b.completed=172;
  v2.save(s,b);assert.equal(v2.read(s,ids),null);
  v2.clear(s);assert.equal(v2.read(s,ids),null);
});
test('unavailable storage does not crash read/clear',()=>{
  const bad={getItem(){throw Error('denied')},removeItem(){throw Error('denied')}};
  assert.equal(v2.read(bad,ids),null);assert.doesNotThrow(()=>v2.clear(bad));
});
