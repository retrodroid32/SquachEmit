#!/usr/bin/env python3
"""Compile *actual* SquachWatch source against synthetic fixture inputs.

Provide a local SquachWatch-CYD checkout. This script writes only to a temporary
build directory and does not modify SquachWatch's branch, files, or git state.

Covers pure signature lookup/Remote ID parsing, not the stateful deauth/evel
 twin detector, RF reception, or firmware GUI. Those require separate tests.
"""
import argparse
import json
import shutil
import subprocess
import tempfile
from pathlib import Path
from rid_vectors import advertisements

ROOT = Path(__file__).resolve().parents[1]

def array(s: bytes) -> str:
    return '{' + ','.join(f'0x{v:02x}' for v in s) + '}'

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('squachwatch', type=Path, help='Local SquachWatch-CYD checkout')
    args = parser.parse_args()
    fork = args.squachwatch.resolve()
    for target in ['include/signatures.h', 'src/signatures.cpp', 'src/remote_id.cpp']:
        if not (fork/target).is_file():
            parser.error(f'missing {fork/target}')
    if not shutil.which('g++'):
        parser.error('g++ required for native tests (Linux, WSL, MinGW)')
    cases = json.loads((ROOT/'tests'/'cases.json').read_text())
    offline = json.loads((ROOT/'tests'/'offline_vectors.json').read_text())
    lines = ['#include "signatures.h"', '#include "remote_id.h"',
             '#include <stdint.h>', '#include <stdio.h>', '#include <string.h>',
             'static int failed=0;',
             '#define CK(id,cond) do { if(!(cond)){ fprintf(stderr, "FAIL %s\\n", id); ++failed; } } while(0)',
             'int main() {']
    for i, s in enumerate(cases):
        id, kind, exp = s['id'], s['kind'], s['expected']
        if kind=='BleUuid':
            lines.append(f'CK("{id}", lookupUuid(0x{s["code"]:04x}) == DetectionType::{"UNKNOWN" if exp=="NONE" else exp});')
        elif kind in ('BleManufacturer','BleAppleNegative'):
            if exp not in ('IBEACON', 'AIRTAG', 'NONE'):
                lines.append(f'CK("{id}", lookupMfgId(0x{s["code"]:04x}) == DetectionType::{exp});')
            elif exp=='NONE':
                payload=b'\\x05\\xff\\x4c\\x00\\x01\\x10'
                lines.append(f'{{ const uint8_t p[]={array(payload)}; CK("{id}", !isAirTagPayload(p,sizeof(p))); }}')
            elif exp=='AIRTAG':
                payload=bytes.fromhex('07ff4c0012190102')
                lines.append(f'{{ const uint8_t p[]={array(payload)}; CK("{id}", isAirTagPayload(p,sizeof(p))); }}')
            elif exp=='IBEACON':
                payload=bytes.fromhex('4c000215') + b'SQUACHEMIT-TEST!' + bytes.fromhex('00010002c5')
                lines.append(f'{{ const uint8_t p[]={array(payload)}; CK("{id}", isIBeacon(p,sizeof(p))); }}')
        elif kind=='BleName':
            lines.append(f'CK("{id}", lookupBtName("{s["text"]}") == DetectionType::{"UNKNOWN" if exp=="NONE" else exp});')
        elif kind=='WifiAp':
            lines.append(f'CK("{id}", lookupSsid("{s["text"]}") == DetectionType::{"UNKNOWN" if exp=="NONE" else exp});')
    for i, vec in enumerate(offline):
        b=bytes.fromhex(vec['frame_hex'])
        id=vec['id']
        lines.append(f'{{ const uint8_t f[]={array(b)};')
        klass, exp=vec['class'], vec['expected']
        if klass=='WIFI_OUI':
            t='UNKNOWN' if exp=='NONE' else exp
            lines.append(f'CK("{id}", lookupOui(f+16)==DetectionType::{t});')
        elif klass=='WIFI_PROBE':
            lines.append(f'CK("{id}", isWifiWildcardProbe(f,sizeof(f)));')
            if exp=='FLOCK':
                lines.append('CK("FLOCK-WILDCARD-QCA", isFlockFirmwareWifiPrefix(f+10));')
            else:
                lines.append('CK("FLOCK-WILDCARD-NEG-SRC", !isFlockFirmwareWifiPrefix(f+10));')
        elif klass=='WIFI_PWNAGOTCHI':
            lines.append('char name[32]={};')
            lines.append(f'CK("{id}", pwnagotchiName(f,sizeof(f),name,sizeof(name)) && strcmp(name,"sqe-lab")==0);')
        elif klass=='WIFI_REMOTEID':
            want= 'true' if exp=='DRONE' else 'false'
            if id.endswith('NAN'):
                lines.append(f'CK("{id}", RemoteId::isWifiNanAction(f,sizeof(f))=={want});')
            else:
                lines.append(f'CK("{id}", RemoteId::isWifiBeacon(f,sizeof(f))=={want});')
        elif klass in ('WIFI_DEAUTH','WIFI_EVILTWIN_SEQUENCE'):
            # Stateful rules cannot be judged from one isolated frame.
            lines.append(f'CK("{id}-shape", sizeof(f)>=26);')
        lines.append('}')
    lines.append('RemoteId::Info rid;')
    for i,adv in enumerate(advertisements()):
        lines.append(f'{{ const uint8_t a[]={array(adv)}; CK("RID-BLE-{i}", RemoteId::merge(a,sizeof(a),rid,1000+{i}*1000)); }}')
    lines.extend(['CK("RID-fields", rid.haveBasic && rid.haveLoc && rid.haveOperator && strcmp(rid.serial,"SQUACHEMIT-TEST-0001")==0);',
                  'CK("RID-airframe", rid.uaType==2);',
                  'CK("RID-coordinates", rid.lat >40.7127f && rid.lat <40.7129f && rid.opLat >40.7579f && rid.opLat <40.7581f);',
                  'printf("Native SquachWatch signature and RID checks: %s\\n", failed ? "FAIL" : "PASS");',
                  'return failed ? 1 : 0;', '}'])
    with tempfile.TemporaryDirectory(prefix='squachemit-native-') as d:
        d=Path(d)
        (d/'test.cpp').write_text('\n'.join(lines)+'\n')
        cmd=['g++','-std=gnu++17','-O2','-I'+str(fork/'include'),str(d/'test.cpp'),
             str(fork/'src/signatures.cpp'),str(fork/'src/remote_id.cpp'),'-o',str(d/'verify')]
        subprocess.run(cmd, check=True)
        subprocess.run([str(d/'verify')], check=True)

if __name__=='__main__': main()
