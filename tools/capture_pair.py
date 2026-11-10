#!/usr/bin/env python3
"""Capture timestamped transmitter + receiver serial evidence in one NDJSON.

This is passive for the SquachWatch receiver. By default it does not start
SquachEmit RF; --case explicitly runs one bounded scenario on the emitter.
NO detector classification is inferred from free-form receiver log text.
"""
from __future__ import annotations

import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import queue
import re
import threading
import time


def valid_case(value: str) -> str:
    if not re.fullmatch(r"[A-Z0-9][A-Z0-9_-]{2,47}", value):
        raise ValueError("case must be a known SquachEmit scenario ID (LIST)")
    return value


def make_line(source: str, raw: bytes, timestamp_ns: int, utc: str) -> dict:
    return {"type": "serial", "source": source,
            "host_monotonic_ns": timestamp_ns,
            "host_utc": utc,
            "line": raw.decode("utf-8", "replace").rstrip("\r\n")[:2048]}


def read_port(name, port, events: queue.Queue, stop: threading.Event) -> None:
    while not stop.is_set():
        try:
            raw = port.readline()
        except Exception as exc:
            events.put({"type": "reader_error", "source": name, "error": str(exc)})
            stop.set()
            break
        if raw:
            # Sample both clocks immediately after the line is read.
            events.put(make_line(name, raw, time.monotonic_ns(),
                                 datetime.now(timezone.utc).isoformat()))


def main() -> None:
    cli = argparse.ArgumentParser(description=__doc__)
    cli.add_argument("--emitter", required=True, help="SquachEmit COM port")
    cli.add_argument("--receiver", required=True, help="SquachWatch COM port")
    cli.add_argument("--emitter-baud", type=int, default=115200)
    cli.add_argument("--receiver-baud", type=int, default=2000000)
    cli.add_argument("--seconds", type=float, default=45,
                     help="Recording time, normally longer than the RF case")
    cli.add_argument("--case", type=valid_case, default=None,
                     help="Optional explicit one-shot RUN case; default passive")
    cli.add_argument("--case-duration", type=int, default=12,
                     help="3..30 seconds, only used when --case is present")
    cli.add_argument("--output", type=Path, default=Path("squachemit-paired.ndjson"))
    args = cli.parse_args()
    if args.emitter == args.receiver:
        cli.error("emitter and receiver must be different serial ports")
    if args.seconds <= 0 or args.seconds > 3600:
        cli.error("--seconds must be within 0..3600")
    if args.case_duration < 3 or args.case_duration > 30:
        cli.error("--case-duration must be 3..30 seconds")
    try:
        import serial
    except ImportError:
        cli.error("pyserial missing: pip install -r requirements-host.txt")

    args.output.parent.mkdir(parents=True, exist_ok=True)
    stop = threading.Event()
    events = queue.Queue()
    # Capture both streams using the same host monotonic clock.
    with serial.Serial(args.emitter, args.emitter_baud, timeout=0.3) as tx, \
         serial.Serial(args.receiver, args.receiver_baud, timeout=0.3) as rx, \
         args.output.open("w", encoding="utf-8") as out:
        def emit(event):
            out.write(json.dumps(event, ensure_ascii=False) + "\n")
            out.flush()
        emit({"type": "session", "host_utc": datetime.now(timezone.utc).isoformat(),
              "emitter_baud": args.emitter_baud, "receiver_baud": args.receiver_baud,
              "emitter_port": args.emitter, "receiver_port": args.receiver,
              "case": args.case, "case_duration_seconds": args.case_duration if args.case else None})
        workers = [threading.Thread(target=read_port, args=(name, port, events, stop),
                                    daemon=True) for name, port in (("emitter", tx), ("receiver", rx))]
        for worker in workers:
            worker.start()
        # Native USB can reset ESP32 on open, so allow setup/boot to finish.
        started = False
        start_at = time.monotonic()
        try:
            while not stop.is_set() and time.monotonic() - start_at < args.seconds:
                if args.case and not started and time.monotonic() - start_at >= 2.5:
                    tx.write(f"RUN {args.case} {args.case_duration}\n".encode("ascii"))
                    emit({"type": "command", "host_utc": datetime.now(timezone.utc).isoformat(),
                          "command": f"RUN {args.case} {args.case_duration}"})
                    started = True
                try:
                    emit(events.get(timeout=0.2))
                except queue.Empty:
                    pass
        except KeyboardInterrupt:
            emit({"type": "interrupted", "host_utc": datetime.now(timezone.utc).isoformat()})
        finally:
            if started:
                # Never leave a test on-air when the controller exits early.
                try:
                    tx.write(b"STOP\n")
                except Exception:
                    pass
            stop.set()
            for worker in workers:
                worker.join(timeout=1)
            while not events.empty():
                emit(events.get_nowait())
    print(f"Paired serial evidence saved: {args.output}")
    print("Review actual SquachWatch detections before scoring; no results were inferred.")


if __name__ == "__main__":
    main()
