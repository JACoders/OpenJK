#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/network/local_host"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/network/local_host"
BIN="${OPENJK_BIN:-$ROOT/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
EXPECTED=737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]] || exit 2
for players in 2 3 4; do
	"$ROOT/tests/splitscreen/gameplay/run_e2e.sh" --case "listen-${players}p-lan" \
		--players "$players" --vm native --timeout 100 --artifact-root "$RESULTS" \
		--hash "$BIN" \
		--remote-command "/bin/bash '$HERE/launch.sh' remote '$players'" \
		--client-command "/bin/bash '$HERE/launch.sh' host '$players'"
done
python3 "$HERE/validate.py" --results "$RESULTS"
