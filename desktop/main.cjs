'use strict';
/*
 * SquachEmit Workbench Desktop
 * Local-only Electron wrapper around the existing, independently tested
 * Detection Workbench. USB serial connections remain Web Serial sessions
 * opened by explicit user clicks. Camera access is optional and local.
 */
const {app,BrowserWindow,dialog,session,shell}=require('electron');
const fs=require('node:fs');
const http=require('node:http');
const path=require('node:path');

let server=null;
let windowRef=null;
let appOrigin='';
const contentTypes={
  '.html':'text/html; charset=utf-8',
  '.css':'text/css; charset=utf-8',
  '.js':'text/javascript; charset=utf-8',
  '.json':'application/json; charset=utf-8'
};
function contentPaths(){
  if(app.isPackaged){
    return {
      workbench:path.join(process.resourcesPath,'workbench'),
      panel:path.join(process.resourcesPath,'desktop-panel')
    };
  }
  return {
    workbench:path.resolve(__dirname,'..','workbench'),
    panel:path.join(__dirname,'public')
  };
}
function resolveResource(urlPath,roots){
  const pathname=urlPath.split('?')[0];
  const name=decodeURIComponent(pathname);
  if(name==='/workbench/' || name==='/workbench/index.html')
    return {file:path.join(roots.workbench,'index.html'),html:true};
  if(name==='/workbench/cases.json')
    return {file:app.isPackaged ?
      path.join(roots.workbench,'cases.json'):
      path.resolve(__dirname,'..','tests','cases.json')};
  if(/^\/workbench\/(?:app|core)\.js$/.test(name) ||
     name==='/workbench/style.css')
    return {file:path.join(roots.workbench,path.basename(name))};
  if(name==='/desktop/controller.js' || name==='/desktop/controller.css' ||
      name==='/desktop/mirror.js' || name==='/desktop/native-emitter.js')
    return {file:path.join(roots.panel,path.basename(name))};
  return null;
}
function serve(req,res,roots){
  if(req.method!=='GET'&&req.method!=='HEAD'){
    res.writeHead(405,{'Allow':'GET, HEAD'});res.end();return;
  }
  let resource;
  try{resource=resolveResource(new URL(req.url,'http://localhost').pathname,roots);}
  catch(_){res.writeHead(400);res.end();return;}
  if(!resource){res.writeHead(404);res.end('Not found');return;}
  fs.readFile(resource.file,(err,body)=>{
    if(err){res.writeHead(404);res.end('Not found');return;}
    if(resource.html){
      const html=body.toString('utf8')
        .replace(/href="\.\.\/"/g,
          'href="https://retrodroid32.github.io/SquachEmit/" target="_blank" rel="noopener noreferrer"')
        .replace('</head>',
          '  <link rel="stylesheet" href="/desktop/controller.css">\n'+
          '  <script src="/desktop/mirror.js" defer></script>\n'+
           '  <script src="/desktop/native-emitter.js" defer></script>\n'+
           '  <script src="/desktop/controller.js" defer></script>\n</head>');
      body=Buffer.from(html,'utf8');
    }
    const type=contentTypes[path.extname(resource.file)]||'application/octet-stream';
    const headers={
      'Content-Type':type,'Content-Length':body.length,
      'Cache-Control':'no-store','X-Content-Type-Options':'nosniff',
      'Referrer-Policy':'no-referrer',
      'Permissions-Policy':'camera=(self), microphone=(), geolocation=()',
      'Content-Security-Policy':
        "default-src 'self'; script-src 'self'; style-src 'self'; "+
        "img-src 'self' data: blob:; media-src 'self' blob:; "+
        "connect-src 'self'; object-src 'none'; base-uri 'self'; frame-ancestors 'none'"
    };
    res.writeHead(200,headers);
    if(req.method==='HEAD')res.end();
    else res.end(body);
  });
}
function sameAppOrigin(input){
  try{return new URL(input).origin===appOrigin;}catch(_){return false;}
}
function enablePermissions(win){
  const ses=session.defaultSession;
  ses.setPermissionCheckHandler((_webContents,permission,origin)=>{
    return sameAppOrigin(origin) && (permission==='serial'||permission==='media');
  });
  ses.setPermissionRequestHandler((webContents,permission,callback,details)=>{
    const url=(details&&details.requestingUrl)||webContents.getURL();
    const camera=permission==='media' &&
      Array.isArray(details&&details.mediaTypes) &&
      details.mediaTypes.includes('video') &&
      !details.mediaTypes.includes('audio');
    callback(sameAppOrigin(url) && (permission==='serial'||camera));
  });
  ses.on('select-serial-port',(event,ports,webContents,callback)=>{
    event.preventDefault();
    if(webContents!==win.webContents || !ports.length){
      callback('');return;
    }
    const candidates=ports.slice(0,12);
    const options=candidates.map((p,i)=>{
      const label=p.displayName||p.portName||('USB Serial '+(i+1));
      return label+' ['+(p.portName||p.portId)+']';
    }).concat('Cancel');
    void dialog.showMessageBox(win,{
      type:'question',title:'Select USB serial device',
      message:'Choose the SquachEmit or SquachWatch COM port',
      detail:'Use the Workbench connect button for the correct role. '+
        'SquachEmit defaults to 115200 baud; SquachWatch commonly uses 2000000 baud.',
      buttons:options,cancelId:options.length-1,noLink:true
    }).then(({response})=>{
      callback(response>=0&&response<candidates.length ?
        candidates[response].portId:'');
    }).catch(()=>callback(''));
  });
}
async function createWindow(){
  const roots=contentPaths();
  server=http.createServer((req,res)=>serve(req,res,roots));
  await new Promise((resolve,reject)=>{
    server.once('error',reject);
    server.listen(0,'127.0.0.1',resolve);
  });
  appOrigin='http://127.0.0.1:'+server.address().port;
  windowRef=new BrowserWindow({
    width:1750,height:1050,minWidth:1100,minHeight:720,
    backgroundColor:'#080f1d',show:false,
    title:'SquachEmit Workbench · Desktop Control Center',
    autoHideMenuBar:true,
    webPreferences:{
      nodeIntegration:false,contextIsolation:true,sandbox:true,
      webSecurity:true
    }
  });
  enablePermissions(windowRef);
  windowRef.webContents.setWindowOpenHandler(({url})=>{
    if(/^https:\/\/github\.com\/retrodroid32\//.test(url)||
       /^https:\/\/retrodroid32\.github\.io\/SquachEmit\//.test(url))
      void shell.openExternal(url);
    return {action:'deny'};
  });
  windowRef.webContents.on('will-navigate',(event,target)=>{
    if(!sameAppOrigin(target))event.preventDefault();
  });
  await windowRef.loadURL(appOrigin+'/workbench/?desktop=1');
  windowRef.once('ready-to-show',()=>windowRef.show());
  // When the renderer cannot load, still show a window with diagnostics.
  if(!windowRef.isVisible())windowRef.show();
}
app.setAppUserModelId('com.retrodroid32.squachemit.workbench');
app.whenReady().then(createWindow).catch(error=>{
  dialog.showErrorBox('SquachEmit Desktop startup failed',String(error));
  app.quit();
});
app.on('window-all-closed',()=>app.quit());
app.on('before-quit',()=>{if(server){server.close();server=null;}});
