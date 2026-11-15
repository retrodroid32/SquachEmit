/* Authoritative PARTIAL SquachWatch USB UI v2 model.
 * Source contract: retrodroid32/SquachWatch-CYD PR #237.
 * NO invented screen widgets: MAIN has actual COUNTS, DEVICES LOG has RAM ROWs.
 * Other screens MUST use authentic MIRROR pixel fallback.
 */
(function(root,factory){
  'use strict';const lib=factory();
  if(typeof module==='object'&&module.exports)module.exports=lib;
  if(root)root.SQEWatchNativeV2=lib;
})(typeof window==='undefined'?null:window,function(){
  'use strict';
  const groups=[
    [1,'FLOCK'],[2,'AXON'],[3,'META'],[4,'SKIMMER'],
    [5,'RAVEN'],[6,'TRACKER'],[7,'DRONE'],[8,'ALPR'],
    [9,'CAMERA'],[17,'HACKER'],[14,'DEAUTH'],[16,'IBEACON']
  ];
  const groupIds=new Set(groups.map(g=>g[0]));
  function decodeHex(s){
    if(s==='-')return '';
    if(!/^(?:[A-Fa-f0-9]{2}){1,19}$/.test(s))return null;
    let out='';
    for(let i=0;i<s.length;i+=2){
      const b=parseInt(s.slice(i,i+2),16);
      out+=b>=32&&b<127?String.fromCharCode(b):'·';
    }
    return out;
  }
  const uint=(str,max)=>/^\d+$/.test(str)&&Number(str)<=max?Number(str):null;
  function state(line){
    const a=String(line).split(' ');
    if(a.length!==11||a[0]!=='UI'||a[1]!=='STATE'||a[2]!=='2')return null;
    const fields=[uint(a[3],65535),uint(a[4],41),uint(a[5],480),
      uint(a[6],480),uint(a[7],3),uint(a[8],4294967295),
      uint(a[9],1),uint(a[10],2)];
    if(fields.some(v=>v===null)||fields[2]<1||fields[3]<1)return null;
    const [seq,screen,width,height,rotation,uptime,privacy,kind]=fields;
    if((screen===1&&kind!==1)||(screen!==1&&kind===1)||
       (screen!==3&&kind===2))return null;
    return {seq,screen,width,height,rotation,uptime,privacy:!!privacy,kind};
  }
  function counts(line){
    const a=String(line).split(' ');
    if(a.length<5||a.length>16||a.slice(0,3).join(' ')!=='UI COUNTS 2')return null;
    const seq=uint(a[3],65535);if(seq===null)return null;
    const entries=[],seen=new Set();
    for(const piece of a.slice(4)){
      const m=/^(\d{1,2}):(\d{1,10}):(\d{1,10})$/.exec(piece);
      if(!m)return null;
      const [id,fresh,total]=[uint(m[1],32),uint(m[2],4294967295),uint(m[3],4294967295)];
      if(id===null||fresh===null||total===null||!groupIds.has(id)||seen.has(id)||fresh>total)return null;
      seen.add(id);entries.push({id,label:groups.find(x=>x[0]===id)[1],fresh,total});
    }
    if(entries.length<11||entries.length>12)return null;
    if(entries.some((x,i)=>x.id!==groups[i][0]))return null;
    return {seq,entries};
  }
  function create(){
    let stateNow=null,countsNow=null,rows=[],list=null;
    function reset(){stateNow=null;countsNow=null;rows=[];list=null;}
    function feed(line){
      if(line==='UI CAPS 2 SQUACHWATCH STATE COUNTS RECENT_LOG TOUCH PIXEL_FALLBACK PARTIAL')
        return {kind:'caps'};
      if(line==='UI READY 2 SQUACHWATCH TOUCH PARTIAL')return {kind:'ready'};
      if(/^UI ERROR /.test(line)){list=null;return {kind:'error',reason:line.slice(9)};}
      if(/^UI STATUS 2 /.test(line))return {kind:'status',text:line};
      const st=state(line);
      if(st){
        if(stateNow&&(st.screen!==stateNow.screen||st.width!==stateNow.width||
           st.height!==stateNow.height||st.rotation!==stateNow.rotation||
           st.privacy!==stateNow.privacy||st.kind!==stateNow.kind)){
          countsNow=null;rows=[];list=null;
        }
        stateNow=st;
        if(st.kind===0){countsNow=null;rows=[];}
        return {kind:'state',state:st};
      }
      const co=counts(line);
      if(co){
        if(!stateNow||stateNow.kind!==1)return {kind:'ignored'};
        countsNow=co.entries;
        return {kind:'counts',counts:co.entries};
      }
      let m=/^UI LIST 2 (\d{1,5}) (\d{1,3}) (\d{1,2}) (\d{1,2}) (\d{1,2})$/.exec(line);
      if(m){
        const [seq,category,start,total,count]=m.slice(1).map(Number);
        if(!stateNow||stateNow.kind!==2||seq<1||seq>65535||
           start>63||total>64||count>8||start+count>total||
           list&&list.pending)return {kind:'ignored'};
        list={seq,category,start,total,count,pending:true,items:[]};
        return {kind:'list-start'};
      }
      m=/^UI ROW 2 (\d{1,5}) (\d{1,2}) (\d{1,2}) ([01]) (-?\d{1,3}) (\d{1,10}) (\d{1,10}) ([01]) ([A-Fa-f0-9Xx:]{17}) ([A-Fa-f0-9]{2,38}|-)$/.exec(line);
      if(m){
        if(!list||!list.pending||Number(m[1])!==list.seq)return {kind:'ignored'};
        const rank=Number(m[2]),type=Number(m[3]),rssi=Number(m[5]);
        const name=decodeHex(m[10]);
        if(rank!==list.start+list.items.length||type>32||rssi< -127||rssi>20||name===null||
           !/^(?:[A-Fa-f0-9Xx]{2}:){5}[A-Fa-f0-9Xx]{2}$/.test(m[9])){
          list=null;return {kind:'error',reason:'INVALID_ROW'};
        }
        list.items.push({rank,type,fresh:m[4]==='1',rssi,age:Number(m[6]),
          firstSeen:Number(m[7]),restored:m[8]==='1',mac:m[9],name});
        if(list.items.length>list.count){list=null;return {kind:'error',reason:'EXTRA_ROW'};}
        return {kind:'row'};
      }
      m=/^UI END 2 (\d{1,5})$/.exec(line);
      if(m){
        if(!list||Number(m[1])!==list.seq||list.items.length!==list.count){
          list=null;return {kind:'error',reason:'INCOMPLETE_LIST'};
        }
        rows=list.items.slice();
        const page={category:list.category,start:list.start,total:list.total,rows};
        list=null;
        return {kind:'list',page};
      }
      return {kind:'ignored'};
    }
    function snapshot(){return {state:stateNow,counts:countsNow,rows:rows.slice()};}
    return Object.freeze({feed,snapshot,reset});
  }
  function render(ctx,model){
    if(!model?.state||!ctx)return false;
    const {state:s,counts:items,rows}=model;
    if(s.kind===0)return false;
    const w=s.width,h=s.height;
    if(ctx.canvas){ctx.canvas.width=w;ctx.canvas.height=h;}
    ctx.fillStyle='#0A1020';ctx.fillRect(0,0,w,h);
    const scale=Math.min(w/320,h/240);
    ctx.save();ctx.scale(scale,scale);
    const put=(x,y,str,size=12,color='#DFE9F3')=>{
      ctx.fillStyle=color;ctx.font='bold '+size+'px monospace';
      ctx.textBaseline='top';ctx.fillText(String(str),x,y,302);
    };
    put(11,9,'SQUACHWATCH · NATIVE V2',14,'#62E3CE');
    put(11,30,s.kind===1?'MAIN · VERIFIED COUNTER MODEL':'LOG · DEVICES IN RAM',12,'#B7C8DD');
    put(11,47,'PARTIAL MODEL · NOT AN LCD SCREENSHOT',10,'#E9BD63');
    if(s.kind===1){
      if(!items)put(13,93,'Waiting for UI COUNTS...',13,'#DEDEDE');
      else items.forEach((o,i)=>{
        const col=i%3,row=Math.floor(i/3),x=10+col*103,y=69+row*37;
        ctx.fillStyle='#192B41';ctx.fillRect(x,y,99,33);
        put(x+4,y+3,o.label,10,'#65D9F1');
        put(x+4,y+17,o.fresh+' / '+o.total,11,'#F3F6FF');
      });
    }else{
      if(!rows.length)put(12,99,'No complete recent-row snapshot yet',12);
      rows.slice(0,6).forEach((o,i)=>{
        const y=68+i*25;
        ctx.fillStyle=i%2?'#101E32':'#18283F';ctx.fillRect(9,y,301,24);
        put(12,y+2,o.name||o.mac,11,o.fresh?'#78E0B7':'#A9B7C9');
        put(12,y+13,'T'+o.type+' · '+o.rssi+' dBm · '+o.age+'s'+
          (o.restored?' · restored':''),9,'#B4C2D1');
      });
    }
    put(11,220,'USB STATE · '+w+'×'+h+' · ROT '+s.rotation+
      (s.privacy?' · PRIVACY':''),9,'#9EB5CB');
    ctx.restore();
    return true;
  }
  return Object.freeze({create,state,counts,decodeHex,render,groups});
});
