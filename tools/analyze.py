#!/usr/bin/env python3
"""Score observed SquachWatch detections against independent SquachEmit truth.

Never interpret an unfilled observation as a missed detection. Fill NONE for a
complete run where no detection appeared, otherwise leave blank / untested.
"""
import argparse
import csv
import json
import math
import statistics
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def main():
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument('observations', type=Path, nargs='?', default=ROOT/'tests'/'observations.csv')
    cli.add_argument('--output', type=Path, default=ROOT/'tests'/'report.md')
    args = cli.parse_args()
    cases = {s['id']: s for s in json.loads((ROOT/'tests'/'cases.json').read_text())}
    rows = list(csv.DictReader(args.observations.open(newline='', encoding='utf-8')))
    seen = set()
    counters = dict(tp=0, tn=0, fp=0, fp_negative=0, fn=0, wrong=0, untested=0)
    latencies = []
    report = ['# SquachEmit validation results', '',
              'Ground truth comes from the explicitly selected SquachEmit case.',
              'A blank observation is **untested**, never an automatic failure.', '',
              '| Case | Expected | Observed | Result | Latency (ms) |',
              '|---|---|---|---|---:|']
    for row in rows:
        sid = row.get('id', '').strip().upper()
        if sid not in cases:
            raise SystemExit(f'unknown scenario: {sid!r}')
        if sid in seen:
            raise SystemExit(f'duplicate observation for {sid}')
        seen.add(sid)
        expected = cases[sid]['expected']
        observed = row.get('observed', '').strip().upper()
        valid_labels = {x['expected'] for x in cases.values()} | {'NONE'}
        # Other SquachWatch DetectionTypes are legitimate wrong-class answers.
        valid_labels |= {'RING','ALPR','CAMERA','DEAUTH','EVILTWIN','IBEACON'}
        if observed and observed not in valid_labels:
            raise SystemExit(f'unknown observed label {sid}: {observed}')
        if not observed:
            counters['untested'] += 1; result = 'UNTESTED'
        elif expected == 'NONE':
            if observed == 'NONE': counters['tn'] += 1; result = 'TN'
            else: counters['fp'] += 1; counters['fp_negative'] += 1; result = 'FP'
        elif observed == 'NONE':
            counters['fn'] += 1; result = 'FN'
        elif observed != expected:
            counters['wrong'] += 1; result = 'WRONG'; counters['fp'] += 1; counters['fn'] += 1
        else:
            counters['tp'] += 1; result = 'TP'
        raw_latency = row.get('latency_ms', '').strip()
        if raw_latency:
            try: latency = float(raw_latency)
            except ValueError: raise SystemExit(f'bad latency: {sid}')
            if not math.isfinite(latency) or latency < 0: raise SystemExit(f'bad latency: {sid}')
            if result == 'TP': latencies.append(latency)
        report.append(f"| {sid} | {expected} | {observed or '—'} | {result} | {raw_latency or '—'} |")
    if seen != set(cases):
        raise SystemExit(f'missing rows for: {", ".join(sorted(set(cases)-seen))}')
    p = counters['tp']/(counters['tp']+counters['fp']) if counters['tp']+counters['fp'] else float('nan')
    r = counters['tp']/(counters['tp']+counters['fn']) if counters['tp']+counters['fn'] else float('nan')
    spec = counters['tn']/(counters['tn']+counters['fp_negative']) if counters['tn']+counters['fp_negative'] else float('nan')
    def fmt(x): return 'n/a' if math.isnan(x) else f'{100*x:.1f}%'
    report.extend(['', '## Scoring', '',
        f"- Tested: {len(rows)-counters['untested']} of {len(rows)}; untested: {counters['untested']}",
        f"- TP {counters['tp']}; TN {counters['tn']}; FP {counters['fp']}; FN {counters['fn']}; wrong-class {counters['wrong']}",
        f'- Precision: {fmt(p)}', f'- Recall (detection sensitivity): {fmt(r)}',
        f'- Specificity: {fmt(spec)}',
        f"- Median TP detection latency: {statistics.median(latencies):.0f} ms" if latencies else '- Median TP detection latency: n/a',
        '', 'These metrics score *synthetic signatures*, not the ability to discover real products.',
        'Do not mix offline-only vectors with on-air observations in this report.', ''])
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text('\n'.join(report), encoding='utf-8')
    print(f"Wrote {args.output} ({len(rows)-counters['untested']} tested / {len(rows)} total); precision={fmt(p)}, recall={fmt(r)}")

if __name__ == '__main__': main()
