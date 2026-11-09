#!/usr/bin/env python3
"""Build a static ESP Web Tools site from already-built, trusted CI binaries.

Invoked by GitHub Actions; download CI artifacts *only* from this repository's
successful runs. Binary artifacts are treated as data, never executed.
No firmware is compiled, signed, or marked hardware-verified by this script.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import re
import shutil
import struct
from pathlib import Path

SOURCE = Path(__file__).resolve().parent.parent / 'web-installer'
BOARDS = {
    'c5-waveshare-28': {'chip': 'ESP32-C5', 'label': 'Waveshare ESP32-C5 Touch LCD 2.8'},
    's3-devkit': {'chip': 'ESP32-S3', 'label': 'ESP32-S3 DevKitC-1'},
}
ALLOWED_BRANCH = re.compile(r'^(main|lab/[A-Za-z0-9._/-]{1,110})$')
SHA = re.compile(r'^[a-f0-9]{40}$')


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def require_binary(path: Path, minimum: int = 32, magic: bytes | None = None) -> bytes:
    if not path.is_file() or path.stat().st_size < minimum:
        raise ValueError(f'missing or short firmware input: {path}')
    b = path.read_bytes()
    if magic is not None and not b.startswith(magic):
        raise ValueError(f'invalid firmware header for {path.name}')
    return b


def verify_partitions(data: bytes, expected_app: int = 0x10000) -> None:
    if data[:2] != b'\xaa\x50':
        raise ValueError('partition table magic not found')
    parts = []
    for pos in range(0, len(data) - 31, 32):
        row = data[pos:pos + 32]
        if row[:2] != b'\xaa\x50':
            break
        _, typ, subtype, off, size, label, _ = struct.unpack('<HBBII16sI', row)
        parts.append((typ, subtype, off, size, label.rstrip(b'\x00').decode('ascii', errors='replace')))
    if not any(typ == 0 and off == expected_app and size > 0 for typ, sub, off, size, label in parts):
        raise ValueError(f'expected application partition at {expected_app:#x} absent')
    if not any(typ == 1 and sub == 0 and off == 0xe000 for typ, sub, off, size, label in parts):
        raise ValueError('expected otadata partition at 0xE000 absent')


def load_source(src: dict, output: Path) -> dict:
    board = src['board']
    if board not in BOARDS:
        raise ValueError(f'unsupported board: {board}')
    sha = src['sha'].lower()
    if not SHA.fullmatch(sha) or not ALLOWED_BRANCH.fullmatch(src['branch']):
        raise ValueError(f'invalid trusted source ref: {src}')
    if src.get('channel') != 'lab':
        raise ValueError('only lab builds are currently eligible; hardware-approved releases require separate approval')
    created_at = src['created_at']
    if not re.fullmatch(r'20\d{2}-\d\d-\d\dT\d\d:\d\d:\d\dZ', created_at):
        raise ValueError('invalid run creation timestamp')
    input_dir = Path(src['path']).resolve()
    if not input_dir.is_dir():
        raise ValueError(f'artifact directory missing: {input_dir}')
    short = sha[:8]
    label = 'c5' if board == 'c5-waveshare-28' else 's3'
    folder = Path('lab') / (label + '-' + short)
    dest = output / folder
    dest.mkdir(parents=True, exist_ok=True)
    if board == 'c5-waveshare-28':
        boot = require_binary(input_dir / 'SquachEmit.ino.bootloader.bin', magic=b'\xe9')
        partitions = require_binary(input_dir / 'SquachEmit.ino.partitions.bin', magic=b'\xaa\x50')
        app = require_binary(input_dir / 'SquachEmit.ino.bin', 4096, b'\xe9')
        merged = require_binary(input_dir / 'SquachEmit.ino.merged.bin', 0x10000 + len(app))
        if merged[0x2000:0x2000 + len(boot)] != boot:
            raise ValueError('C5 bootloader offset mismatch: expected 0x2000')
        if merged[0x8000:0x8000 + len(partitions)] != partitions:
            raise ValueError('C5 partition offset mismatch: expected 0x8000')
        if merged[0x10000:0x10000 + len(app)] != app:
            raise ValueError('C5 application offset mismatch: expected 0x10000')
        ota = merged[0xe000:0x10000]
        boot_offset = 0x2000
    else:
        boot = require_binary(input_dir / 'bootloader.bin', magic=b'\xe9')
        partitions = require_binary(input_dir / 'partitions.bin', magic=b'\xaa\x50')
        app = require_binary(input_dir / 'firmware.bin', 4096, b'\xe9')
        ota = require_binary(input_dir / 'ota_init.bin', 8192)
        boot_offset = 0x0
    verify_partitions(partitions)
    if len(ota) != 8192 or ota == b'\xff' * len(ota):
        raise ValueError('initial OTA selection block missing or invalid')
    parts = [('bootloader.bin',boot_offset,boot),('partitions.bin',0x8000,partitions),
             ('otadata.bin',0xe000,ota),('application.bin',0x10000,app)]
    checksums=[]
    for filename, off, data in parts:
        (dest / filename).write_bytes(data)
        checksums.append(f'{sha256(data)}  {filename}')
    (dest / 'SHA256SUMS.txt').write_text('\n'.join(checksums)+'\n', encoding='utf-8')
    manifest={
        'name': f'SquachEmit LAB · {BOARDS[board]["label"]} · {short}',
        'version': f'lab-{short}',
        'new_install_prompt_erase': True,
        'builds': [{'chipFamily':BOARDS[board]['chip'],
                    'parts':[{'path':name,'offset':off} for name,off,_ in parts]}],
    }
    (dest / 'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n', encoding='utf-8')
    return {'id':f'lab-{label}-{short}', 'board':board,'channel':'lab','sha':sha,
            'short_sha':short,'branch':src['branch'],'created_at':created_at,
            'run_id':src['run_id'],'manifest':str(folder/'manifest.json'),
            'application':str(folder/'application.bin'),'sha256':sha256(app),
            'bytes':len(app), 'chipFamily':BOARDS[board]['chip']}


def build(source_manifest: Path, output: Path) -> dict:
    data=json.loads(source_manifest.read_text(encoding='utf-8'))
    if not isinstance(data.get('sources'),list): raise ValueError('sources must be a list')
    if output.exists(): shutil.rmtree(output)
    output.mkdir(parents=True)
    for filename in ('index.html','style.css','app.js'):
        shutil.copyfile(SOURCE/filename, output/filename)
    (output/'.nojekyll').write_bytes(b'')
    builds=[]
    for src in data['sources']:
        builds.append(load_source(src,output))
    builds.sort(key=lambda x:x['created_at'],reverse=True)
    catalog={'schema':1,'project':'SquachEmit','generated_from':'successful GitHub CI artifacts',
             'builds':builds,'production_policy':'No production release before approved physical validation'}
    (output/'catalog.json').write_text(json.dumps(catalog,indent=2)+'\n',encoding='utf-8')
    if not builds: raise ValueError('refusing to publish a Pages site with no validated CI source artifacts')
    print(f'Prepared {len(builds)} unverified LAB builds: '+', '.join(x['id'] for x in builds))
    return catalog

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--sources',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    build(args.sources,args.output)
