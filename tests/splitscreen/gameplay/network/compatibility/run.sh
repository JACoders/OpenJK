#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/network/compatibility"
HARNESS="$ROOT/tests/splitscreen/gameplay/run_e2e.sh"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
EXPECTED="737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$EXPECTED" ]] || { echo "frozen binary mismatch: $actual" >&2; exit 2; }
mkdir -p "$RESULTS"

for pure in 0 1; do
	for players in 2 3 4; do
		case_name="stock-$([[ "$pure" == 1 ]] && echo pure || echo nonpure)-${players}p"
		output=$("$HARNESS" \
			--case "$case_name" --players "$players" --vm native --timeout 120 \
			--artifact-root "$RESULTS/runs" \
			--hash "$BIN" \
			--hash "$BUILD/codemp/ui/uix86_64.dylib" \
			--hash "$BUILD/codemp/game/jampgamex86_64.dylib" \
			--hash "$BUILD/codemp/cgame/cgamex86_64.dylib" \
			--hash "$HERE/generate.py" --hash "$HERE/launch_game.sh" \
			--client-command "'$HERE/launch_game.sh' '$players' '$pure'")
		manifest="$(printf '%s\n' "$output" | tail -1)"
		printf '%s\n' "$manifest" >"$RESULTS/${case_name}.latest"
	done
done

PYTHONDONTWRITEBYTECODE=1 python3 "$HERE/analyze.py" --results "$RESULTS"
PYTHONDONTWRITEBYTECODE=1 python3 "$HERE/validate.py" --root "$ROOT" --results "$RESULTS"
