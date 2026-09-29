#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/modes/individual"
RESULTS="${GP201_RESULTS:-$ROOT/tests/splitscreen/gameplay/results/modes/individual}"
BIN="${OPENJK_BIN:-$ROOT/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
expected=737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13
actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$expected" ]] || { echo "frozen binary mismatch: $actual" >&2; exit 2; }
for mode in ffa holocron jedimaster; do
	for players in 2 3 4; do
		"$ROOT/tests/splitscreen/gameplay/run_e2e.sh" \
			--case "${mode}-${players}p" --players "$players" --vm native --timeout 120 \
			--artifact-root "$RESULTS" --hash "$BIN" \
			--client-command "/bin/bash '$HERE/client_case.sh' '$mode' '$players'"
	done
done
python3 "$HERE/validate.py" --results "$RESULTS"
