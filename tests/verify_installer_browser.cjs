/* Browser-side installer preflight regression. Node VM + fake DOM, no hardware. */
'use strict';
const assert=require('node:assert/strict');
const fs=require('node:fs');
const vm=require('node:vm');
const {createHash,webcrypto}=require('node:crypto');
const {setTimeout:pause}=require('node:timers/promises');
const code=fs.readFileSync('web-installer/app.js','utf8');
const ids=['board','build','buildDetails','confirm','notice','channelBadge','enterLab',
  'exitLab','installArea','installLocked','installButton','downloadBin','serialState',
  'connect','disconnect','serialLog','command','commandForm','send','clear','saveLog',
  'baudRate','customBaud','baudHint'];
function element(){
  return {
    value:'',textContent:'',className:'',hidden:false,disabled:false,checked:false,
    children:[],style:{},dataset:{},attrs:{},events:{},
    classList:{toggle(){}},
    addEventListener(k,fn){this.events[k]=fn;},
    replaceChildren(...a){this.children=a;this.value=a[0]?.value??'';},
    appendChild(n){this.children.push(n);},
    setAttribute(k,v){this.attrs[k]=v;},
    removeAttribute(k){delete this.attrs[k];},
    getAttribute(k){return this.attrs[k];},
    dispatch(k){assert(this.events[k],k+' listener');this.events[k]();}
  };
}
function firmware(board='cyd32-st7798',channel='lab'){
  const props={
    'cyd32-st7798':['cyd32','ESP32',0x1000],
    'c5-waveshare-28':['c5','ESP32-C5',0x2000],
    's3-devkit':['s3','ESP32-S3',0x0]
  }[board];
  const sha='a'.repeat(40),short=sha.slice(0,8),folder=channel+'/'+props[0]+'-'+short+'/';
  const names=['bootloader.bin','partitions.bin','otadata.bin','application.bin'];
  const offsets=[props[2],0x8000,0xe000,0x10000];
  const binaries={};
  const segments=names.map((name,i)=>{
    const bytes=Buffer.alloc(i===2?8192:i===3?4096:i===1?3072:100,0xa0+i);
    binaries['./'+folder+name]=bytes;
    return {name,path:folder+name,offset:offsets[i],bytes:bytes.length,
            sha256:createHash('sha256').update(bytes).digest('hex')};
  });
  const entry={
    id:channel+'-'+props[0]+'-'+short,board,channel,sha,short_sha:short,
    chipFamily:props[1],manifest:folder+'manifest.json',
    application:segments[3].path,sha256:segments[3].sha256,bytes:segments[3].bytes,
    segments,branch:'lab/preflight',created_at:'2026-11-11T00:00:00Z',
    title:'LAB verification',version_label:'preflight',summary:'Test only'
  };
  const manifest={builds:[{chipFamily:props[1],parts:names.map((name,i)=>
    ({path:name,offset:offsets[i]}))}]};
  return {entry,manifest,binaries};
}
function spawn(fixture,query='?lab=1'){
  const nodes=Object.fromEntries(ids.map(k=>[k,element()]));
  nodes.baudRate.value='115200';
  const map={'./catalog.json':{builds:[fixture.entry]},
             ['./'+fixture.entry.manifest]:fixture.manifest,...fixture.binaries};
  const doc={
    getElementById(id){assert(nodes[id],id);return nodes[id];},
    querySelectorAll(){return[];},
    createElement(){return element();}
  };
  async function fetch(url){
    const v=map[url];
    if(v===undefined)return {ok:false,status:404};
    return {ok:true,status:200,json:async()=>v,
      arrayBuffer:async()=>Buffer.isBuffer(v)?
        v.buffer.slice(v.byteOffset,v.byteOffset+v.byteLength):
        Buffer.from(JSON.stringify(v)).buffer};
  }
  const context={
    document:doc,window:{isSecureContext:true,addEventListener(){}},
    navigator:{serial:{}},location:{search:query},
    crypto:webcrypto,fetch,URLSearchParams,TextEncoder,TextDecoder,Blob,URL,
    setTimeout,console
  };
  vm.runInNewContext(code,context,{filename:'web-installer/app.js'});
  async function choose(){
    await pause(10); // catalog has loaded and board options exist
    nodes.build.value=fixture.entry.id;
    nodes.build.dispatch('change');
  }
  return {nodes,map,choose};
}
async function until(predicate,what){
  for(let n=0;n<90;n++){
    if(predicate())return;
    await pause(10);
  }
  throw new Error('timeout waiting for '+what);
}
async function run(){
  for(const [board,channel] of [
    ['cyd32-st7798','lab'],['c5-waveshare-28','lab'],
    ['s3-devkit','lab'],['cyd32-st7798','stable']]){
    const f=firmware(board,channel),t=spawn(f,channel==='lab'?'?lab=1':'');
    await t.choose();
    assert.equal(t.nodes.confirm.disabled,true,'must start locked');
    assert.equal(t.nodes.downloadBin.hidden,true,'download locked during verification');
    await until(()=>!t.nodes.confirm.disabled,board+' integrity success');
    assert.match(t.nodes.buildDetails.textContent,/INTEGRITY: PASS/);
    assert.equal(t.nodes.installArea.hidden,true,'board confirmation still mandatory');
    assert.equal(t.nodes.downloadBin.hidden,false);
    t.nodes.confirm.checked=true;
    t.nodes.confirm.dispatch('change');
    assert.equal(t.nodes.installArea.hidden,false);
    assert.equal(t.nodes.installButton.getAttribute('manifest'),'./'+f.entry.manifest);
  }
  const base=firmware();
  for(const part of base.entry.segments){
    const f=firmware(),path='./'+part.path;
    const buf=Buffer.from(f.binaries[path]);
    buf[0]^=0xff;
    f.binaries[path]=buf;
    const t=spawn(f);await t.choose();
    await until(()=>/INTEGRITY: FAILED/.test(t.nodes.buildDetails.textContent),
                part.name+' tamper blocked');
    assert.equal(t.nodes.confirm.disabled,true);
    assert.equal(t.nodes.downloadBin.hidden,true);
    assert.equal(t.nodes.installArea.hidden,true);
    assert.match(t.nodes.buildDetails.textContent,/SHA-256 mismatch/);
  }
  {
    const f=firmware();
    f.manifest.builds[0].chipFamily='ESP32-S3';
    const t=spawn(f);await t.choose();
    await until(()=>/INTEGRITY: FAILED/.test(t.nodes.buildDetails.textContent),
                'wrong chip blocked');
    assert.equal(t.nodes.confirm.disabled,true);
    assert.match(t.nodes.buildDetails.textContent,/chip family mismatch/);
  }
  {
    const f=firmware();
    f.manifest.builds[0].parts[0].offset=0;
    const t=spawn(f);await t.choose();
    await until(()=>/INTEGRITY: FAILED/.test(t.nodes.buildDetails.textContent),
                'wrong offset blocked');
    assert.equal(t.nodes.confirm.disabled,true);
    assert.match(t.nodes.buildDetails.textContent,/offsets or binary filenames mismatch/);
  }
  {
    const f=firmware();
    f.binaries['./'+f.entry.segments[3].path]=Buffer.alloc(30,0);
    const t=spawn(f);await t.choose();
    await until(()=>/INTEGRITY: FAILED/.test(t.nodes.buildDetails.textContent),
                'truncated binary blocked');
    assert.match(t.nodes.buildDetails.textContent,/size mismatch/);
  }
  console.log('PASS: four board/channel selections and corrupted binary, offset, chip, size preflight rejections');
}
run().catch(error=>{console.error(error);process.exitCode=1;});
