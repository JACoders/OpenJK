#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
BIN="$ROOT/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
RUN_HOME="${LIFECYCLE_HOME:-/private/tmp/openjk-r2-02-lifecycle}"
PORT="${LIFECYCLE_PORT:-29322}"
ARTIFACTS="$ROOT/tests/splitscreen/rendering/certification/lifecycle/artifacts"

mkdir -p "$RUN_HOME/base/splitqa" "$RUN_HOME/base/screenshots" "$ARTIFACTS"
"$ROOT/tests/splitscreen/install_assets.sh" "$RUN_HOME" >/dev/null
cp "$ROOT/build-x86_64/codemp/ui/uix86_64.dylib" "$RUN_HOME/base/"
cp "$ROOT/build-x86_64/codemp/game/jampgamex86_64.dylib" "$RUN_HOME/base/"
cp "$ROOT"/build-x86_64/codemp/cgame/cgame*x86_64.dylib "$RUN_HOME/base/"
cp "$ROOT/tests/splitscreen/cfg/respawn_flow.cfg" "$RUN_HOME/base/splitqa/"
cp "$ROOT/tests/splitscreen/cfg/join_spectate_flow.cfg" "$RUN_HOME/base/splitqa/"
cp "$ROOT/tests/splitscreen/rendering/certification/lifecycle/map_team_matrix.cfg" "$RUN_HOME/base/splitqa/"
cp "$ROOT/tests/splitscreen/rendering/certification/lifecycle/respawn_edge_matrix.cfg" "$RUN_HOME/base/splitqa/"

cases=(respawn_flow join_spectate_flow map_team_matrix)
if [[ $# -gt 0 ]]; then
	cases=("$@")
fi
failures=0
for case_name in "${cases[@]}"; do
	log="$ARTIFACTS/${case_name}.stdout.txt"
	"$BIN" \
		+set fs_basepath "$BASE" \
		+set fs_homepath "$RUN_HOME" \
		+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
		+set net_port "$PORT" +set r_fullscreen 0 +set r_mode 3 \
		+set s_initsound 0 +set in_joystick 1 +set com_introplayed 1 \
		+exec "splitqa/${case_name}.cfg" >"$log" 2>&1
	if grep -Eq "Split(NetLifecycle|NetStat|State)Assert: FAIL|ERROR:|recursive error|assertion failed" "$log"; then
		echo "$case_name: FAIL" >&2
		failures=$((failures + 1))
		continue
	fi
	echo "$case_name: PASS"
done

cp "$RUN_HOME"/base/screenshots/*.png "$ARTIFACTS/" 2>/dev/null || true
cp "$RUN_HOME"/base/*.txt "$ARTIFACTS/" 2>/dev/null || true

if [[ $failures -ne 0 ]]; then
	echo "$failures lifecycle case(s) failed" >&2
	exit 1
fi
