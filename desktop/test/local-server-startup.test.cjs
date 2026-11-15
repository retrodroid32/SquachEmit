'use strict';
const test=require('node:test'),assert=require('node:assert/strict');
const fs=require('node:fs'),path=require('node:path'),vm=require('node:vm');
const desktop=path.resolve(__dirname,'..'),root=path.resolve(desktop,'..');
const main=fs.readFileSync(path.join(desktop,'main.cjs'),'utf8');
const html=fs.readFileSync(path.join(root,'workbench/index.html'),'utf8');
const start=main.indexOf('function resolveResource(');
const end=main.indexOf('\nfunction serve(',start);
assert.ok(start>0&&end>start,'resource router must remain testable');
// Evaluate the exact router, without starting Electron or opening a browser.
const context={path,__dirname:desktop,app:{isPackaged:false}};
vm.runInNewContext(main.slice(start,end)+'\nthis.resolveResource=resolveResource;',context);
const resolve=context.resolveResource;
test('every Workbench HTML JS dependency is served by Electron local HTTP router',()=>{
 const scripts=[...html.matchAll(/<script[^>]+src="([^"]+)"/g)].map(x=>x[1]);
 assert.ok(scripts.some(x=>x.includes('runner-v2.js')),
   'suite controller must be present in the actual Workbench HTML');
 for(const src of scripts){
   const url='/workbench/'+src.replace(/^\.\//,'');
   const resource=resolve(url,{workbench:path.join(root,'workbench'),panel:path.join(desktop,'public')});
   assert.ok(resource,'LOCAL HTTP 404 for required startup script: '+url);
   assert.ok(fs.existsSync(resource.file),'Missing startup dependency: '+resource.file);
 }
});
test('all native mirror scripts are served, in dependency order',()=>{
 const inject=main.slice(main.indexOf(".replace('</head>'"),main.indexOf("body=Buffer.from(html"));
 for(const route of ['mirror.js','native-emitter.js','native-watch-v2.js','controller.js']){
   assert.ok(inject.includes('/desktop/'+route),'Missing injected UI dependency '+route);
   const resource=resolve('/desktop/'+route,{
     workbench:path.join(root,'workbench'),panel:path.join(desktop,'public')});
   assert.ok(resource&&fs.existsSync(resource.file),'Unserved remote panel dependency '+route);
 }
 assert.ok(inject.indexOf('native-watch-v2.js')<inject.indexOf('controller.js'),
   'Native v2 protocol must load before controller mounts right rail');
});
test('unknown file paths remain blocked by local allowlist',()=>{
 for(const url of ['/desktop/../main.cjs','/workbench/nope.js',
   '/workbench/../../desktop/main.cjs','/desktop/secret.json']){
   assert.equal(resolve(url,{workbench:root,panel:desktop}),null,url);
 }
});
