#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
python3 -m unittest discover \
	-s "$ROOT/tests/splitscreen/gameplay/regressions/rules" \
	-p 'test_*.py' -v
