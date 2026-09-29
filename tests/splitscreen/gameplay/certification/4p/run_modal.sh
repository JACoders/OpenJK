#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/certification/4p"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
EXPECTED="${GP5_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
GENERIC="$ROOT/tests/splitscreen/gameplay/modal_ownership"

python3 "$ROOT/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" \
	--expected-client-hash "$EXPECTED"
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]] || {
	echo "hash gate failed" >&2
	exit 2
}
OPENJK_BUILD_DIR="$BUILD" OPENJK_BIN="$BIN" "$GENERIC/run.sh" 4
report="$ROOT/tests/splitscreen/gameplay/results/modal_ownership/4p-report.json"
python3 - "$report" <<'PY'
import json
import sys
from pathlib import Path

report = json.loads(Path(sys.argv[1]).read_text())
assert report["schema_version"] == 2
assert report["players"] == 4
assert report["passed"] is True
assert len(report["cases"]) >= 8
assert all(case["passed"] is True for case in report["cases"])
assert {case["owner"] for case in report["cases"]} == {2, 3, 4}
assert {"top", "score", "console", "chat"} <= {
    case["surface"] for case in report["cases"]
}
PY
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]]
printf '%s\n' "$report"
