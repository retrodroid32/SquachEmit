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
  assert.equal(asm.feed('MIRROR ROW 5 0 not_base64!!').kind,'error');
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

test('RGB565 changed-row delta is lossless and preserves every other real pixel',()=>{
  const asm=new MirrorAssembler();
  const first=image(asm,8,3,2,'RGB565',[
    [0x00,0xf8,0xe0,0x07,0x1f,0x00],
    [0xff,0xff,0x00,0x00,0xe0,0x07]
  ]);
  const baseline=Array.from(first.frame.pixels);
  assert.equal(asm.feed('MIRROR DELTA 9 8 3 2 RGB565').kind,'begin');
  assert.equal(asm.feed(row(9,1,[0x00,0x00,0xff,0xff,0x1f,0x00])).kind,'row');
  const updated=asm.feed('MIRROR END 9');
  assert.equal(updated.kind,'frame');
  assert.equal(updated.frame.delta,true);
  assert.equal(updated.frame.changedRows,1);
  assert.deepEqual(Array.from(updated.frame.pixels.slice(0,12)),baseline.slice(0,12));
  assert.deepEqual(Array.from(updated.frame.pixels.slice(12)),[
    0,0,0,255,255,255,255,255,0,0,255,255
  ]);
  assert.deepEqual(Array.from(first.frame.pixels),baseline,'previous full frame remains unchanged');
  assert.equal(asm.feed('MIRROR DELTA 10 9 3 2 RGB565').kind,'begin');
  const noChange=asm.feed('MIRROR END 10');
  assert.equal(noChange.kind,'frame');
  assert.equal(noChange.frame.changedRows,0);
  assert.deepEqual(Array.from(noChange.frame.pixels),Array.from(updated.frame.pixels));
});
test('delta to missing baseline or wrong size fails closed; full frame restores sync',()=>{
  const asm=new MirrorAssembler();
  assert.equal(asm.feed('MIRROR DELTA 11 10 1 1 RGB565').kind,'error');
  image(asm,14,2,1,'RGB565',[[0x00,0xf8,0x1f,0x00]]);
  assert.equal(asm.feed('MIRROR DELTA 15 12 2 1 RGB565').kind,'error');
  assert.equal(asm.feed('MIRROR DELTA 15 14 3 1 RGB565').kind,'error');
  assert.equal(asm.feed('MIRROR DELTA 13 14 2 1 RGB565').kind,'error');
  const fixed=image(asm,16,1,1,'RGB565',[[0xe0,0x07]]);
  assert.equal(fixed.frame.pixels[1],255);
  assert.equal(asm.feed('MIRROR DELTA 17 16 1 1 RGB565').kind,'begin');
  assert.equal(asm.feed('MIRROR ROW 17 0 malformed!').kind,'error');
  assert.equal(asm.last,null,'corrupt delta invalidates host baseline, so stale pixels cannot return');
  assert.equal(asm.feed('MIRROR DELTA 18 16 1 1 RGB565').kind,'error');
  asm.reset();
  assert.equal(asm.feed('MIRROR DELTA 19 18 1 1 RGB565').kind,'error',
    'reconnecting requires fresh full keyframe');
});

test('full baseline waits for every authentic row and reports progress without publishing partial pixels',()=>{
  const asm=new MirrorAssembler();
  assert.equal(asm.feed('MIRROR BEGIN 32 2 3 RGB332').kind,'begin');
  assert.equal(asm.last,null);
  assert.equal(asm.feed('MIRROR PROGRESS 32 0/3').kind,'progress');
  assert.equal(asm.feed(row(32,0,[0xe0,0x03])).kind,'row');
  assert.equal(asm.feed('MIRROR PROGRESS 32 2/3').kind,'progress');
  assert.equal(asm.last,null);
  assert.equal(asm.feed(row(32,1,[0x1c,0xff])).kind,'row');
  assert.equal(asm.feed(row(32,2,[0xe0,0xe0])).kind,'row');
  const done=asm.feed('MIRROR END 32');
  assert.equal(done.kind,'frame');
  assert.equal(done.frame.changedRows,3);
  assert.equal(asm.feed('MIRROR DELTA 33 32 2 3 RGB332').kind,'begin');
  assert.equal(asm.feed(row(33,1,[0,0])).kind,'row');
  assert.equal(asm.feed('MIRROR END 33').kind,'frame');
});

test('invalid baseline progress, malformed rows, and dimension changes revoke trusted pixels',()=>{
  const asm=new MirrorAssembler();
  image(asm,10,2,1,'RGB332',[[0xe0,0x03]]);
  assert.equal(asm.feed('MIRROR DELTA 11 10 2 1 RGB332').kind,'begin');
  assert.equal(asm.feed('MIRROR PROGRESS 11 2/1').kind,'error');
  asm.invalidate();
  assert.equal(asm.last,null);
  assert.equal(asm.feed('MIRROR DELTA 12 10 2 1 RGB332').kind,'error');
  image(asm,13,2,1,'RGB332',[[0x00,0xff]]);
  assert.equal(asm.feed('MIRROR BEGIN 14 240 320 RGB332').kind,'begin');
  assert.equal(asm.last,null,'rotation cannot display the old complete frame');
  assert.equal(asm.feed('MIRROR ROW 14 0 invalid!').kind,'error');
  assert.equal(asm.last,null);
});
