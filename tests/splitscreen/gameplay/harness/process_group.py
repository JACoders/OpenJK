#!/usr/bin/env python3
"""Run one command in a new session so the harness can tear down only its tree."""

import os
import subprocess
import sys


def main() -> None:
    if len(sys.argv) < 3:
        raise SystemExit("usage: process_group.py STATUS_FILE COMMAND [ARG ...]")
    status_file = sys.argv[1]
    os.setsid()
    try:
        completed = subprocess.run(sys.argv[2:], check=False)
        code = completed.returncode
    except BaseException:
        code = 125
        raise
    finally:
        with open(status_file, "w", encoding="ascii") as handle:
            handle.write(f"{code}\n")
    raise SystemExit(code)


if __name__ == "__main__":
    main()
