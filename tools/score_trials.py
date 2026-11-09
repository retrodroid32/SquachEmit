#!/usr/bin/env python3
"""Score replicated physical SquachWatch observations without inferring successes.

CSV rows are manually verified trials, not inferred from a transmitter TRUTH line.
Emissions that failed or lacked a complete listening window are excluded.
Blank observations remain UNTESTED, not false negatives.
"""
from __future__ import annotations

import argparse
import csv
import json
import math
import statistics
from collections import Counter, defaultdict
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECEIVERS = {"CYD-2.8", "CYD-3.2"}
MORE_LABELS = {"ALPR", "CAMERA", "DEAUTH", "EVILTWIN", "RING"}


def classify(row: dict, expected: str) -> str:
    emission = row.get("emission", "").strip().lower()
    window = row.get("window", "").strip().lower()
    observed = row.get("observed", "").strip().upper()
    if emission not in ("", "ok", "failed"):
        raise ValueError(f"invalid emission value {emission!r}")
    if window not in ("", "complete", "incomplete"):
        raise ValueError(f"invalid window value {window!r}")
    if emission == "failed" or window == "incomplete":
        return "EXCLUDED"
    if emission != "ok" or window != "complete" or not observed:
        return "UNTESTED"
    if expected == "NONE":
        return "TN" if observed == "NONE" else "FP"
    if observed == "NONE":
        return "FN"
    return "TP" if observed == expected else "WRONG"


def score_rows(rows: list[dict], cases: dict) -> list[dict]:
    output, seen = [], set()
    labels = {x["expected"] for x in cases.values()} | MORE_LABELS | {"NONE"}
    for index, row in enumerate(rows, 2):
        trial = dict(row)
        board = (trial.get("receiver") or "").strip()
        sid = (trial.get("id") or "").strip().upper()
        run_id = (trial.get("run_id") or "").strip()
        if board not in RECEIVERS or sid not in cases or not run_id:
            raise ValueError(f"row {index}: receiver, id or run_id missing/unknown")
        unique = (board, run_id, sid)
        if unique in seen:
            raise ValueError(f"row {index}: duplicate trial key {unique}")
        seen.add(unique)
        observed = (trial.get("observed") or "").strip().upper()
        if observed and observed not in labels:
            raise ValueError(f"row {index}: unknown detector label {observed}")
        raw_latency = (trial.get("latency_ms") or "").strip()
        latency = None
        if raw_latency:
            try:
                latency = float(raw_latency)
            except ValueError as exc:
                raise ValueError(f"row {index}: invalid latency") from exc
            if not math.isfinite(latency) or latency < 0:
                raise ValueError(f"row {index}: invalid latency")
        result = classify(trial, cases[sid]["expected"])
        if result in {"TP", "TN", "FN", "FP", "WRONG"}:
            # Hardware and code versions are required for defensible measurements.
            if not (trial.get("receiver_sha") or "").strip():
                raise ValueError(f"row {index}: receiver_sha required for counted trial")
            if not (trial.get("emitter_sha") or "").strip():
                raise ValueError(f"row {index}: emitter_sha required for counted trial")
        if result != "TP" and latency is not None:
            raise ValueError(f"row {index}: latency only applies to true-positive trials")
        trial.update(receiver=board, id=sid, observed=observed,
                     expected=cases[sid]["expected"], result=result, latency_ms=latency)
        output.append(trial)
    return output


def summarize(records: list[dict]) -> dict:
    counts = Counter(row["result"] for row in records)
    tp, tn = counts["TP"], counts["TN"]
    fp = counts["FP"] + counts["WRONG"]
    fn = counts["FN"] + counts["WRONG"]
    latencies = sorted(x["latency_ms"] for x in records if x["result"] == "TP" and x["latency_ms"] is not None)
    def ratio(a, b):
        return round(100.0 * a / b, 2) if b else None
    return {
        "trial_count": len(records),
        "counted": tp + tn + counts["FP"] + counts["FN"] + counts["WRONG"],
        "untested": counts["UNTESTED"],
        "excluded": counts["EXCLUDED"],
        "tp": tp, "tn": tn, "fp": fp, "fn": fn, "wrong_class": counts["WRONG"],
        "precision_pct": ratio(tp, tp + fp),
        "recall_pct": ratio(tp, tp + fn),
        "specificity_pct": ratio(tn, tn + fp),
        "median_latency_ms": round(statistics.median(latencies), 1) if latencies else None,
        "p95_latency_ms": round(latencies[max(0, math.ceil(len(latencies) * 0.95) - 1)], 1) if latencies else None,
    }


def render_md(records: list[dict]) -> str:
    def fmt(n):
        return "n/a" if n is None else str(n)
    lines = [
        "# SquachEmit physical verification: replicated trials",
        "", "Only documented completed receiver windows with working emission are scored.",
        "**TRUTH BEGIN on the emitter does not prove radio delivery or receiver detection.**",
        "", "| Receiver | Counted | Untested | Excluded | TP | TN | FP | FN | Wrong class | Precision | Recall | Specificity | Median ms | P95 ms |",
        "|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|",
    ]
    for board in sorted(RECEIVERS):
        m = summarize([x for x in records if x["receiver"] == board])
        lines.append(f'| {board} | {m["counted"]} | {m["untested"]} | {m["excluded"]} | {m["tp"]} | {m["tn"]} | {m["fp"]} | {m["fn"]} | {m["wrong_class"]} | {fmt(m["precision_pct"])}% | {fmt(m["recall_pct"])}% | {fmt(m["specificity_pct"])}% | {fmt(m["median_latency_ms"])} | {fmt(m["p95_latency_ms"])} |')
    lines += [
        "", "## By receiver / scenario", "",
        "| Receiver | Case | Expected | TP | TN | FP | FN | Wrong | Untested | Excluded |",
        "|---|---|---|---:|---:|---:|---:|---:|---:|---:|",
    ]
    groups = defaultdict(list)
    for item in records:
        groups[(item["receiver"], item["id"], item["expected"])].append(item)
    for (board, sid, expected), items in sorted(groups.items()):
        c = Counter(x["result"] for x in items)
        lines.append(f'| {board} | {sid} | {expected} | {c["TP"]} | {c["TN"]} | {c["FP"]} | {c["FN"]} | {c["WRONG"]} | {c["UNTESTED"]} | {c["EXCLUDED"]} |')
    lines += ["", "Synthetic lab accuracy only; not validation against real surveillance hardware.", ""]
    return "\n".join(lines)


def main() -> None:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("trials", type=Path, nargs="?", default=ROOT / "tests" / "trials.csv")
    cli.add_argument("--output", type=Path, default=ROOT / "tests" / "trials-report.md")
    cli.add_argument("--json", type=Path, default=None, help="Optional machine-readable report")
    args = cli.parse_args()
    cases = {x["id"]: x for x in json.loads((ROOT / "tests" / "cases.json").read_text())}
    with args.trials.open(newline="", encoding="utf-8") as stream:
        records = score_rows(list(csv.DictReader(stream)), cases)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(render_md(records), encoding="utf-8")
    if args.json:
        args.json.parent.mkdir(parents=True, exist_ok=True)
        data = {"cohort": "synthetic RF only", "per_receiver": {
            board: summarize([r for r in records if r["receiver"] == board])
            for board in sorted(RECEIVERS)}, "trials": records}
        args.json.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Scored {len(records)} provided trial rows; saved {args.output}")


if __name__ == "__main__":
    main()
