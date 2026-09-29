#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
HARNESS="$ROOT/tests/splitscreen/gameplay/run_e2e.sh"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/modes/ctf_cty"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
requested="${1:-all}"

mkdir -p "$RESULTS"
case "$requested" in
	all) modes=(ctf cty); counts=(2 3 4) ;;
	ctf|cty) modes=("$requested"); counts=(2 3 4) ;;
	ctf-2|ctf-3|ctf-4|cty-2|cty-3|cty-4)
		modes=("${requested%-*}")
		counts=("${requested#*-}")
		;;
	*) echo "usage: $0 [all|ctf|cty|ctf-2|ctf-3|ctf-4|cty-2|cty-3|cty-4]" >&2; exit 2 ;;
esac

for mode in "${modes[@]}"; do
	for players in "${counts[@]}"; do
		output=$("$HARNESS" \
			--case "objective-${mode}-${players}p" --players "$players" --timeout 150 \
			--artifact-root "$RESULTS/runs" \
			--hash "$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64" \
			--hash "$BUILD_DIR/codemp/ui/uix86_64.dylib" \
			--hash "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" \
			--hash "$BUILD_DIR/codemp/cgame/cgamex86_64.dylib" \
			--hash "$HERE/objective_lifecycle.cfg" --hash "$HERE/objective_${players}p.cfg" \
			--hash "$HERE/launch_game.sh" --hash "$HERE/analyze.py" \
			--client-command "'$HERE/launch_game.sh' $players $mode")
		manifest=$(printf '%s\n' "$output" | tail -1)
		printf '%s\n' "$manifest" >"$RESULTS/${mode}-${players}p-manifest-path.txt"
	done
done

PYTHONDONTWRITEBYTECODE=1 python3 "$HERE/analyze.py"
