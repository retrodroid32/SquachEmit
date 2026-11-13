/* Real USB LCD-frame decoder. No simulated graphics and no network access.
 * SquachWatch PR #225: RGB332 sprite rows.
 * SquachEmit CYD32 LAB: RGB565 TFT GRAM rows (little endian).
 * Frame is only published after every row in one matching sequence arrives.
 */
(function(root,factory){
  'use strict';
  const codec=factory();
  if(typeof module!=='undefined'&&module.exports)module.exports=codec;
  if(root)root.SQEMirrorCodec=codec;
})(typeof window==='undefined'?null:window,function(){
  'use strict';
  function MirrorAssembler(){this.pending=null;}
  MirrorAssembler.prototype.reset=function(){this.pending=null;};
  MirrorAssembler.prototype.feed=function(line){
    line=String(line||'').trim();
    if(/^MIRROR ERROR(?: |$)/.test(line)){
      this.reset();
      return {kind:'error',message:line};
    }
    let m=/^MIRROR BEGIN (\d+) (\d{1,3}) (\d{1,3}) (RGB332|RGB565)$/.exec(line);
    if(m){
      const sequence=Number(m[1]),width=Number(m[2]),height=Number(m[3]);
      if(!Number.isSafeInteger(sequence)||width<1||height<1||
         width>480||height>480||width*height>230400){
        this.reset();return {kind:'error',message:'Invalid MIRROR image dimensions'};
      }
      this.pending={sequence,width,height,format:m[4],seen:new Uint8Array(height),
        rows:0,pixels:new Uint8ClampedArray(width*height*4)};
      return {kind:'begin',sequence,width,height,format:m[4]};
    }
    m=/^MIRROR ROW (\d+) (\d{1,3}) ([A-Za-z0-9+/]+={0,2})$/.exec(line);
    if(m){
      const f=this.pending,sequence=Number(m[1]),y=Number(m[2]);
      if(!f||f.sequence!==sequence||y>=f.height||f.seen[y]){
        this.reset();return {kind:'error',message:'Out-of-sequence or duplicate LCD row'};
      }
      const expected=f.width*(f.format==='RGB565'?2:1);
      let binary;
      try{binary=atob(m[3]);}catch(_){
        this.reset();return {kind:'error',message:'Invalid LCD row base64'};
      }
      if(binary.length!==expected){
        this.reset();return {kind:'error',message:'Incomplete LCD row'};
      }
      for(let x=0;x<f.width;x++){
        const p=(y*f.width+x)*4;
        if(f.format==='RGB565'){
          const w=binary.charCodeAt(2*x)|(binary.charCodeAt(2*x+1)<<8);
          const r=(w>>11)&31,g=(w>>5)&63,b=w&31;
          f.pixels[p]=(r*255/31)|0;
          f.pixels[p+1]=(g*255/63)|0;
          f.pixels[p+2]=(b*255/31)|0;
        }else{
          const w=binary.charCodeAt(x);
          f.pixels[p]=(((w>>5)&7)*255/7)|0;
          f.pixels[p+1]=(((w>>2)&7)*255/7)|0;
          f.pixels[p+2]=((w&3)*255/3)|0;
        }
        f.pixels[p+3]=255;
      }
      f.seen[y]=1;f.rows++;
      return {kind:'row',sequence,y,rows:f.rows,total:f.height};
    }
    m=/^MIRROR END (\d+)$/.exec(line);
    if(m){
      const f=this.pending;
      if(!f||f.sequence!==Number(m[1])||f.rows!==f.height){
        this.reset();return {kind:'error',message:'Incomplete or mismatched LCD frame'};
      }
      this.reset();
      return {kind:'frame',frame:{sequence:f.sequence,width:f.width,
        height:f.height,format:f.format,pixels:f.pixels}};
    }
    return {kind:'info',message:line};
  };
  return {MirrorAssembler};
});
