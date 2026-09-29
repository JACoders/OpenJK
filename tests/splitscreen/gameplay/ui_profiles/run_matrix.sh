#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/ui_profiles"
RESULTS="${GP1_RESULTS:-$ROOT/tests/splitscreen/gameplay/results/ui_profiles}"
BIN="${OPENJK_BIN:-$ROOT/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"

python3 "$HERE/validate.py"
for players in 2 3 4; do
	"$ROOT/tests/splitscreen/gameplay/run_e2e.sh" \
		--case "${players}p" --players "$players" --vm native --timeout 240 \
		--artifact-root "$RESULTS" --hash "$BIN" \
		--client-command "/bin/bash '$HERE/client_case.sh' $players"
done
python3 "$HERE/validate.py" --results "$RESULTS"
