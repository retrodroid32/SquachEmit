#!/usr/bin/env python3
"""Capture SquachEmit's timestamped TRUTH stream over USB serial to a log."""
import argparse
from datetime import datetime, timezone
from pathlib import Path
import serial

p = argparse.ArgumentParser()
p.add_argument('port', help='COM8 or /dev/ttyACM0')
p.add_argument('--baud', type=int, default=115200)
p.add_argument('--output', type=Path)
a = p.parse_args()
filename = a.output or Path('capture-' + datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ') + '.log')
with serial.Serial(a.port, a.baud, timeout=0.5) as s, filename.open('a', encoding='utf-8') as f:
    print('Logging to', filename, '| Ctrl+C to stop')
    try:
        while True:
            b = s.readline()
            if not b: continue
            line = b.decode('utf-8', errors='replace').rstrip()
            f.write(datetime.now(timezone.utc).isoformat() + ' ' + line + '\n')
            f.flush()
            print(line)
    except KeyboardInterrupt: pass
