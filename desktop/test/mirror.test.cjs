'use strict';
const test=require('node:test');
const assert=require('node:assert/strict');
const {MirrorAssembler}=require('../public/mirror.js');
const row=(seq,y,bytes)=>'MIRROR ROW '+seq+' '+y+' '+Buffer.from(bytes).toString('base64');
function image(asm,seq,w,h,format,rows){
  assert.equal(asm.feed('MIRROR BEGIN '+seq+' '+w+' '+h+' '+format).kind,'begin');
  for(let y=0;y<h;y++){
    const record=asm.feed(row(seq,y,rows[y]));
    assert.equal(record.kind,'row');
    assert.equal(record.y,y);
  }
  return asm.feed('MIRROR END '+seq);
}
test('SquachWatch RGB332 pixels decode from real USB row, not invented UI',()=>{
  const asm=new MirrorAssembler();
  const result=image(asm,4,4,1,'RGB332',[[0xe0,0x1c,0x03,0xff]]);
  assert.equal(result.kind,'frame');
  assert.equal(result.frame.format,'RGB332');
  assert.deepEqual(Array.from(result.frame.pixels),[
    255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255
  ]);
  assert.equal(asm.pending,null);
});
test('SquachEmit RGB565 little-endian pixels decode faithfully',()=>{
  const asm=new MirrorAssembler();
  const result=image(asm,14,4,1,'RGB565',[
    [0x00,0xf8,0xe0,0x07,0x1f,0x00,0xff,0xff]
  ]);
  assert.equal(result.kind,'frame');
  assert.deepEqual(Array.from(result.frame.pixels),[
    255,0,0,255,0,255,0,255,0,0,255,255,255,255,255,255
  ]);
});
test('portrait 240x320 frame and alternating role formats are supported',()=>{
  const asm=new MirrorAssembler();
  const rows=Array.from({length:320},()=>Buffer.alloc(240,0xe0));
  const result=image(asm,25,240,320,'RGB332',rows);
  assert.equal(result.kind,'frame');
  assert.equal(result.frame.pixels.length,240*320*4);
  assert.equal(result.frame.pixels[0],255);
  const two=image(asm,26,1,1,'RGB565',[[0x1f,0x00]]);
  assert.equal(two.frame.pixels[2],255);
});
test('missing, duplicate, wrong-sequence and corrupt rows never publish a frame',()=>{
  const asm=new MirrorAssembler();
  asm.feed('MIRROR BEGIN 1 2 2 RGB332');
  assert.equal(asm.feed(row(1,0,[0,1])).kind,'row');
  assert.equal(asm.feed('MIRROR END 1').kind,'error');
  assert.equal(asm.pending,null);
  asm.feed('MIRROR BEGIN 2 1 1 RGB332');
  assert.equal(asm.feed(row(3,0,[0])).kind,'error');
  asm.feed('MIRROR BEGIN 4 1 1 RGB332');
  assert.equal(asm.feed(row(4,0,[0,1])).kind,'error');
  asm.feed('MIRROR BEGIN 5 1 1 RGB332');
  assert.equal(asm.feed('MIRROR ROW 5 0 not_base64!!').kind,'info');
  assert.equal(asm.feed('MIRROR ERROR FRAME_ORDER 5').kind,'error');
  asm.feed('MIRROR BEGIN 7 1 2 RGB332');
  assert.equal(asm.feed(row(7,0,[0])).kind,'row');
  assert.equal(asm.feed(row(7,0,[0])).kind,'error');
  assert.equal(asm.pending,null);
});
test('oversized dimensions and explicit stream errors fail closed',()=>{
  const asm=new MirrorAssembler();
  assert.equal(asm.feed('MIRROR BEGIN 1 999 320 RGB332').kind,'error');
  assert.equal(asm.feed('MIRROR END 1').kind,'error');
  assert.equal(asm.feed('MIRROR READY RGB332 320x240 TOUCH').kind,'info');
  assert.equal(asm.feed('MIRROR BEGIN 100 0 0 RGB565').kind,'error');
  asm.feed('MIRROR BEGIN 8 1 1 RGB565');
  assert.equal(asm.feed('MIRROR ERROR LCD_NOT_READY').kind,'error');
  assert.equal(asm.pending,null);
});
