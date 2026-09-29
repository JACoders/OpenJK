#!/usr/bin/env python3
"""Small deterministic worker used by the isolated-runner self-tests."""

import argparse
import os
import signal
import socket
import sys
import time


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "behavior",
        choices=("success", "failure", "assertion_failure", "hang", "listen"),
    )
    parser.add_argument("--port", type=int, default=0)
    args = parser.parse_args()
    print(f"worker={args.behavior} pid={os.getpid()}", flush=True)
    if args.behavior == "success":
        return 0
    if args.behavior == "failure":
        return 23
    if args.behavior == "assertion_failure":
        print(
            "SplitInputAssertCmd: FAIL player=2 expectedForward=127 actualForward=0",
            flush=True,
        )
        return 0
    if args.behavior == "listen":
        listener = socket.socket()
        listener.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        try:
            listener.bind(("127.0.0.1", args.port))
            listener.listen()
            print(f"listening={args.port}", flush=True)
        except PermissionError:
            # A restricted runner can still exercise deadline and process-group
            # teardown even when its sandbox prohibits opening the test socket.
            listener.close()
            print(f"listening=restricted port={args.port}", flush=True)
    signal.signal(signal.SIGTERM, lambda *_: sys.exit(0))
    while True:
        time.sleep(0.1)


if __name__ == "__main__":
    raise SystemExit(main())
