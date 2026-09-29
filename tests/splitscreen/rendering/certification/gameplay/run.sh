#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/rendering/certification/gameplay"
BUILD="$ROOT/build-x86_64"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOME_PATH="${GAMEPLAY_HOME:-/private/tmp/openjk-acceptance-gameplay}"
ARTIFACTS="$HERE/artifacts"
mkdir -p "$HOME_PATH/base/splitqa" "$HOME_PATH/base/screenshots" "$ARTIFACTS"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOME_PATH" >/dev/null
cp "$BUILD/codemp/ui/uix86_64.dylib" "$HOME_PATH/base/"
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$HOME_PATH/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$HOME_PATH/base/"
cp "$HERE/lightning_duel.cfg" "$HOME_PATH/base/splitqa/"
"$BIN" +set fs_basepath "$BASE" +set fs_homepath "$HOME_PATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29642 \
	+set r_fullscreen 0 +set r_mode -1 +set r_customwidth 1280 \
	+set r_customheight 720 +set s_initsound 0 +set in_joystick 1 \
	+exec splitqa/lightning_duel.cfg > "$ARTIFACTS/gameplay.stdout.txt" 2>&1 &
pid=$!
( sleep 90; kill "$pid" 2>/dev/null || true ) & watchdog=$!
set +e; wait "$pid"; status=$?; set -e
kill "$watchdog" 2>/dev/null || true
wait "$watchdog" 2>/dev/null || true
[[ $status -eq 0 ]]
cp "$HOME_PATH"/base/screenshots/acceptance_*.png "$ARTIFACTS/"
cp "$HOME_PATH/base/acceptance_lightning_duel_console.txt" "$ARTIFACTS/"
if grep -Eq "Split(NetLifecycle|NetStat|Profile|Input)Assert: FAIL" "$ARTIFACTS/gameplay.stdout.txt"; then
	exit 1
fi
grep -q "SplitInputSim: button device=controller1 player=2 button=0 pressed=1" "$ARTIFACTS/gameplay.stdout.txt"
grep -q "SplitInputSim: key device=keyboard player=1" "$ARTIFACTS/gameplay.stdout.txt"
grep -q "SplitNetLifecycleAssert: PASS player=2 expected=DEAD actual=DEAD" "$ARTIFACTS/gameplay.stdout.txt"
