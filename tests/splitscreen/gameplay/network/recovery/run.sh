#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/network/recovery"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/network/recovery"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
EXPECTED="737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]] || exit 2
mkdir -p "$RESULTS"
index="$RESULTS/manifests.txt"
players_to_run=(2 3 4)
if (( $# )); then
	players_to_run=()
	for player in "$@"; do
		[[ "$player" =~ ^[234]$ ]] || {
			echo "usage: run.sh [2] [3] [4]" >&2
			exit 2
		}
		players_to_run+=("$player")
	done
fi
if (( ${#players_to_run[@]} == 3 )); then
	: >"$index"
else
	touch "$index"
fi
printf -v server_command '%q' "$HERE/server_driver.sh"
printf -v client_command '%q' "$HERE/client_driver.sh"
for players in "${players_to_run[@]}"; do
	set +e
	output="$("$ROOT/tests/splitscreen/gameplay/run_e2e.sh" \
		--case "gp3-03-recovery-${players}p" --players "$players" --vm native \
		--server-command "$server_command" \
		--client-command "$client_command" \
		--timeout 240 --artifact-root "$RESULTS/runs" \
		--hash "$BIN" \
		--hash "$BUILD/openjkded.x86_64" \
		--hash "$BUILD/codemp/ui/uix86_64.dylib" \
		--hash "$BUILD/codemp/game/jampgamex86_64.dylib" \
		--hash "$BUILD/codemp/cgame/cgamex86_64.dylib")"
	code=$?
	set -e
	manifest="$(printf '%s\n' "$output" | tail -n 1)"
	[[ -s "$manifest" ]] || { echo "missing manifest for ${players}p" >&2; exit 1; }
	index_tmp="$index.tmp.$$"
	awk -F '\t' -v player="$players" '$1 != player' "$index" >"$index_tmp"
	printf '%s\t%s\t%s\n' "$players" "$code" "$manifest" >>"$index_tmp"
	mv "$index_tmp" "$index"
done
python3 "$HERE/analyze.py" --index "$index" --output "$RESULTS/matrix.json"
python3 "$HERE/validate.py"
