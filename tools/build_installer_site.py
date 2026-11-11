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
    'cyd32-st7798': {'chip': 'ESP32', 'label': 'CYD 3.2 ST7798 ESP32-2432S032R'},
}
ALLOWED_BRANCH = re.compile(r'^(main|lab/[A-Za-z0-9._/-]{1,110})$')
SHA = re.compile(r'^[a-f0-9]{40}$')

NOTES_PATH = Path(__file__).resolve().with_name('firmware_build_notes.json')


def load_build_notes(path: Path = NOTES_PATH) -> dict:
    """Curated, SHA-pinned labels; never guess feature parity from build age."""
    data = json.loads(path.read_text(encoding='utf-8'))
    if data.get('schema') != 1 or not isinstance(data.get('builds'), dict):
        raise ValueError('invalid firmware_build_notes.json schema')
    notes = data['builds']
    for commit, entry in notes.items():
        if not SHA.fullmatch(commit) or not isinstance(entry, dict):
            raise ValueError('invalid commit in build note catalog')
        for field, limit in (('title', 110), ('version', 40), ('summary', 600)):
            value = entry.get(field)
            if not isinstance(value, str) or not value.strip() or len(value) > limit:
                raise ValueError(f'invalid build note {commit[:8]} {field}')
        features = entry.get('highlights', [])
        if (not isinstance(features, list) or len(features) > 7
            or not all(isinstance(f, str) and 0 < len(f) <= 260 for f in features)):
            raise ValueError(f'invalid highlights in build note {commit[:8]}')
    return notes


def describe_source_profile(profile: dict, board: str, source_sha: str) -> dict:
    """Describe a mainline artifact from metadata fetched at its exact CI commit.

    The firmware source fingerprint is enforced by host CI before publishing.
    """
    if not isinstance(profile, dict) or profile.get('schema') != 1:
        raise ValueError('invalid source firmware-profile schema')
    if profile.get('source_commit_sha') != source_sha:
        raise ValueError('source firmware-profile ref differs from CI artifact')
    if profile.get('scenario_count') != 172:
        raise ValueError('source firmware-profile scenario count mismatch')
    builds = profile.get('builds')
    if not isinstance(builds, dict) or set(builds) != set(BOARDS):
        raise ValueError('source firmware profile must name all three boards')
    record = builds.get(board)
    if not isinstance(record, dict):
        raise ValueError('source firmware profile missing board')
    for field, limit in (('title', 110), ('version', 40), ('summary', 600)):
        val = record.get(field)
        if not isinstance(val, str) or not val.strip() or len(val) > limit:
            raise ValueError(f'invalid source firmware-profile {board}: {field}')
    features = record.get('highlights')
    if (not isinstance(features, list) or not features or len(features) > 7
        or any(not isinstance(s, str) or not s.strip() or len(s) > 260 for s in features)):
        raise ValueError('invalid source firmware profile highlights')
    return {'title': record['title'],
            'version_label': f"{record['version']} · {source_sha[:8]}",
            'summary': record['summary'], 'highlights': list(features),
            'notes_verified_for_sha': True}


def describe_build(sha: str, branch: str, board: str, notes: dict) -> dict:
    """Known SHAs get exact features. Unknown SHAs never inherit old notes."""
    if sha in notes:
        note = notes[sha]
        return {'title': note['title'], 'version_label': note['version'],
                'summary': note['summary'], 'highlights': list(note['highlights']),
                'notes_verified_for_sha': True}
    label = {
        'lab/drone-rid-profiles-': 'Drone Remote ID LAB (review exact changes)',
        'lab/rf-fixtures-reporting-': 'RF Detection LAB (review exact changes)',
        'lab/esp32c5-waveshare-touch-': 'C5 Touchscreen LAB (review exact changes)',
        'lab/cyd32-st7798-': 'CYD 3.2 ST7798 LAB (review exact changes)',
    }
    title = next((name for prefix, name in label.items() if branch.startswith(prefix)),
                 f"SquachEmit {BOARDS[board]['label']} — build {sha[:8]} (needs changelog)")
    version = ('MAINLINE' if branch == 'main' else 'LAB') + f' · build {sha[:8]}'
    return {'title': title, 'version_label': version,
            'summary': f'Source build {sha[:8]} on {branch} has no reviewed feature description. '
                       'Check its CI run and exact source diff before flashing; capabilities are not asserted.',
            'highlights': [], 'notes_verified_for_sha': False}


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


def load_source(src: dict, output: Path, notes: dict | None = None) -> dict:
    board = src['board']
    if board not in BOARDS:
        raise ValueError(f'unsupported board: {board}')
    sha = src['sha'].lower()
    if not SHA.fullmatch(sha) or not ALLOWED_BRANCH.fullmatch(src['branch']):
        raise ValueError(f'invalid trusted source ref: {src}')
    channel = src.get('channel')
    if channel not in ('lab', 'stable'):
        raise ValueError('unsupported firmware channel')
    if channel == 'stable' and src['branch'] != 'main':
        raise ValueError('public release must originate from successful main CI')
    created_at = src['created_at']
    if not re.fullmatch(r'20\d{2}-\d\d-\d\dT\d\d:\d\d:\d\dZ', created_at):
        raise ValueError('invalid run creation timestamp')
    input_dir = Path(src['path']).resolve()
    if not input_dir.is_dir():
        raise ValueError(f'artifact directory missing: {input_dir}')
    short = sha[:8]
    label = {'c5-waveshare-28':'c5',
             's3-devkit':'s3',
             'cyd32-st7798':'cyd32'}[board]
    folder = Path(channel) / (label + '-' + short)
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
        # Classic ESP32 WROOM bootloader begins at 0x1000, unlike S3
        # bootloader at 0x0 and C5 bootloader at 0x2000.
        boot_offset = 0x1000 if board == 'cyd32-st7798' else 0x0
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
    # Current-main CI artifacts are not automatically the older tagged
    # v1.0.0 release. Derive version, title and changes from the exact source.
    description = (describe_source_profile(src['source_profile'], board, sha)
                   if src.get('source_profile') is not None
                   else describe_build(sha, src['branch'], board,
                                       notes if notes is not None else load_build_notes()))
    manifest={
        'name': description['title'],
        'version': description['version_label'],
        'new_install_prompt_erase': True,
        'builds': [{'chipFamily':BOARDS[board]['chip'],
                    'parts':[{'path':name,'offset':off} for name,off,_ in parts]}],
    }
    (dest / 'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n', encoding='utf-8')
    # Integrity manifest: the browser must verify *all four* flash segments.
    # The human-readable application hash alone cannot protect bootloader,
    # partition table or OTA selection bytes from a truncated Pages upload.
    segments = [
        {'name':filename, 'path':str(folder / filename), 'offset':offset,
         'sha256':sha256(data), 'bytes':len(data)}
        for filename,offset,data in parts
    ]
    return {'id':f'{channel}-{label}-{short}', 'board':board,'channel':channel,'sha':sha,
            'short_sha':short,'branch':src['branch'],'created_at':created_at,
            'run_id':src['run_id'],'manifest':str(folder/'manifest.json'),
            'application':str(folder/'application.bin'),'sha256':sha256(app),
            'bytes':len(app), 'chipFamily':BOARDS[board]['chip'],
            'segments':segments, **description}


def build(source_manifest: Path, output: Path) -> dict:
    data=json.loads(source_manifest.read_text(encoding='utf-8'))
    if not isinstance(data.get('sources'),list): raise ValueError('sources must be a list')
    if output.exists(): shutil.rmtree(output)
    output.mkdir(parents=True)
    for filename in ('index.html','style.css','app.js'):
        shutil.copyfile(SOURCE/filename, output/filename)
    (output/'.nojekyll').write_bytes(b'')
    builds=[]
    notes = load_build_notes()
    for src in data['sources']:
        builds.append(load_source(src,output,notes))
    builds.sort(key=lambda x:x['created_at'],reverse=True)
    catalog={'schema':1,'project':'SquachEmit','generated_from':'successful GitHub CI artifacts',
             'builds':builds,'production_policy':'Public v1.0.0 approved by owner; physical hardware verification remains pending'}
    (output/'catalog.json').write_text(json.dumps(catalog,indent=2)+'\n',encoding='utf-8')
    if not builds: raise ValueError('refusing to publish a Pages site with no validated CI source artifacts')
    print(f'Prepared {len(builds)} firmware builds (public and LAB; hardware verification pending): '+', '.join(x['id'] for x in builds))
    return catalog

if __name__=='__main__':
    p=argparse.ArgumentParser()
    p.add_argument('--sources',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    build(args.sources,args.output)
