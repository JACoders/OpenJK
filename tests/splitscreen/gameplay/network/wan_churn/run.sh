#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
HARNESS="$ROOT/tests/splitscreen/gameplay/run_e2e.sh"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/network/wan_churn"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
EXPECTED="737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
BIN="$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
requested="${1:-all}"

actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$EXPECTED" ]] || { echo "frozen binary mismatch: $actual" >&2; exit 2; }
mkdir -p "$RESULTS"

case "$requested" in
	all) counts=(2 3 4) ;;
	2|3|4) counts=("$requested") ;;
	*) echo "usage: $0 [all|2|3|4]" >&2; exit 2 ;;
esac

for players in "${counts[@]}"; do
	output=$("$HARNESS" \
		--case "wan-churn-${players}p" --players "$players" --timeout 180 \
		--artifact-root "$RESULTS/runs" \
		--hash "$BIN" --hash "$BUILD_DIR/openjkded.x86_64" \
		--hash "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" \
		--hash "$BUILD_DIR/codemp/ui/uix86_64.dylib" \
		--hash "$BUILD_DIR/codemp/cgame/cgamex86_64.dylib" \
		--hash "$HERE/generate_cfg.py" --hash "$HERE/launch_server.sh" \
		--hash "$HERE/launch_game.sh" --hash "$HERE/analyze.py" \
		--server-command "'$HERE/launch_server.sh'" \
		--client-command "'$HERE/launch_game.sh' $players")
	printf '%s\n' "$output" | tail -1 >"$RESULTS/${players}p-manifest-path.txt"
done

if [[ "$requested" == all ]]; then
	PYTHONDONTWRITEBYTECODE=1 python3 "$HERE/analyze.py"
fi
