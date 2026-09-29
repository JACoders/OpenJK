#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
QA_ROOT="${OPENJK_STOCK_QA_ROOT:-/tmp/openjk_stock_regression}"
SP_BIN="${OPENJK_SP_BIN:-$BUILD_DIR/openjk_sp.arm64.app/Contents/MacOS/openjk_sp.arm64}"
MP_BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
SP_HOME="$QA_ROOT/sp"
MP_HOME="$QA_ROOT/mp"
COLD_HOME="$QA_ROOT/cold"
LOG_DIR="$QA_ROOT/logs"

mkdir -p "$SP_HOME/base/splitqa" "$MP_HOME/base/splitqa" "$COLD_HOME/base/splitqa" "$LOG_DIR"
cp "$ROOT/tests/splitscreen/cfg/single_player_smoke.cfg" "$SP_HOME/base/splitqa/"
cp "$ROOT/tests/splitscreen/cfg/one_player_mp_smoke.cfg" "$MP_HOME/base/splitqa/"
cp "$ROOT/tests/splitscreen/cfg/cold_start_probe.cfg" "$COLD_HOME/base/splitqa/"
cp "$BUILD_DIR/code/game/jagamearm64.dylib" "$SP_HOME/base/"
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$MP_HOME/base/"
cp "$BUILD_DIR/codemp/cgame/cgamearm64.dylib" "$MP_HOME/base/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$MP_HOME/base/"
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$COLD_HOME/base/"
cp "$BUILD_DIR/codemp/cgame/cgamearm64.dylib" "$COLD_HOME/base/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$COLD_HOME/base/"

"$SP_BIN" +set fs_basepath "$BASEPATH" +set fs_homepath "$SP_HOME" \
	+set vm_game 0 +set r_fullscreen 0 +set s_initsound 0 \
	+exec splitqa/single_player_smoke.cfg >"$LOG_DIR/single_player.log" 2>&1

"$MP_BIN" +set fs_basepath "$BASEPATH" +set fs_homepath "$MP_HOME" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29240 \
	+set r_fullscreen 0 +set s_initsound 0 \
	+exec splitqa/one_player_mp_smoke.cfg >"$LOG_DIR/one_player_mp.log" 2>&1

"$MP_BIN" +set fs_basepath "$BASEPATH" +set fs_homepath "$COLD_HOME" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29241 \
	+set r_fullscreen 0 +set s_initsound 0 +set com_introplayed 1 \
	+exec splitqa/cold_start_probe.cfg >"$LOG_DIR/cold_start.log" 2>&1

if grep -Eq "Segmentation fault|Assertion.*failed|recursive error|ERROR:|Sys_Error" "$LOG_DIR"/*.log; then
	echo "stock regression QA emitted an error marker" >&2
	exit 1
fi
grep -q "Wrote screenshots/splitqa_single_player.png" "$LOG_DIR/single_player.log"
grep -q "Wrote screenshots/splitqa_one_player_mp.png" "$LOG_DIR/one_player_mp.log"
grep -q "Wrote screenshots/splitqa_cold_start.png" "$LOG_DIR/cold_start.log"
test -s "$SP_HOME/base/screenshots/splitqa_single_player.png"
test -s "$MP_HOME/base/screenshots/splitqa_one_player_mp.png"
test -s "$COLD_HOME/base/screenshots/splitqa_cold_start.png"
if grep -q "SplitNet party:" "$LOG_DIR/one_player_mp.log"; then
	echo "ordinary one-player MP unexpectedly started a split-screen party" >&2
	exit 1
fi

echo "stock single-player, one-player MP, and cold-start QA passed"
echo "screenshots: $QA_ROOT"
echo "logs: $LOG_DIR"
