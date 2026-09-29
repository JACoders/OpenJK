#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT/tests/splitscreen/external/macos_input_sim"

"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null
"$BIN" gamepad-demo "${1:-1000}"
echo "external SDL gamepad bridge probe passed"
