#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/rendering/certification/intermission"
BUILD="$ROOT/build-x86_64"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOME_PATH="${INTERMISSION_HOME:-/private/tmp/openjk-r3-02-intermission}"
PORT="${INTERMISSION_PORT:-29532}"
ARTIFACTS="$HERE/artifacts"

mkdir -p "$HOME_PATH/base/splitqa" "$HOME_PATH/base/screenshots" "$ARTIFACTS"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOME_PATH" >/dev/null
cp "$BUILD/codemp/ui/uix86_64.dylib" "$HOME_PATH/base/"
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$HOME_PATH/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$HOME_PATH/base/"
cp "$HERE/intermission_4p.cfg" "$HOME_PATH/base/splitqa/"

"$BIN" +set fs_basepath "$BASE" +set fs_homepath "$HOME_PATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port "$PORT" \
	+set r_fullscreen 0 +set r_mode -1 +set r_customwidth 1280 \
	+set r_customheight 720 +set s_initsound 0 +set in_joystick 1 \
	+exec splitqa/intermission_4p.cfg > "$ARTIFACTS/intermission.stdout.txt" 2>&1 &
game_pid=$!
( sleep 90; kill "$game_pid" 2>/dev/null || true ) &
watchdog_pid=$!
set +e
wait "$game_pid"
status=$?
set -e
kill "$watchdog_pid" 2>/dev/null || true
wait "$watchdog_pid" 2>/dev/null || true
if [[ $status -ne 0 ]]; then
	echo "intermission run failed or timed out: $status" >&2
	exit "$status"
fi

cp "$HOME_PATH/base/screenshots/cert_intermission_before.png" "$ARTIFACTS/"
cp "$HOME_PATH/base/screenshots/cert_intermission_4p.png" "$ARTIFACTS/"
cp "$HOME_PATH/base/cert_intermission_console.txt" "$ARTIFACTS/"
grep -Eq "Kill limit hit|hit the kill limit" "$ARTIFACTS/intermission.stdout.txt"
if grep -Eq "Split(NetLifecycle|NetStat|NetStagePair)Assert: FAIL" "$ARTIFACTS/intermission.stdout.txt"; then
	echo "intermission lifecycle assertion failed" >&2
	exit 1
fi
for player in 1 2 3 4; do
	grep -q "SplitNetLifecycleAssert: PASS player=$player expected=INTERMISSION actual=INTERMISSION" \
		"$ARTIFACTS/intermission.stdout.txt"
done
python3 "$HERE/assert_intermission.py" "$ARTIFACTS/cert_intermission_before.png" \
	"$ARTIFACTS/cert_intermission_4p.png" | tee "$ARTIFACTS/oracle.txt"
