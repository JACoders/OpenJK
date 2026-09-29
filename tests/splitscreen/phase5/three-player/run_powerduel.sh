#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-/tmp/openjk_phase5_powerduel}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
LOG="$HOMEPATH/base/qa-logs/powerduel.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29206}"

mkdir -p "$HOMEPATH/base/splitqa" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/phase5/three-player/powerduel.cfg" "$HOMEPATH/base/splitqa/phase5_powerduel.cfg"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do cp "$module" "$HOMEPATH/base/"; done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
cp "$BUILD_DIR/codemp/rd-vanilla/rd-vanilla_arm64.dylib" "$(dirname "$BIN")/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=2 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29197 \
	+set r_fullscreen 0 +set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/phase5_powerduel.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

sent=0
for _ in $(seq 1 2200); do
	if [[ $sent -eq 0 ]] && rg -q "Phase5PowerDuel: READY_ALL_ATTACK" "$LOG" 2>/dev/null; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			bridge-mouse left down \
			gamepad 1 axis 1 -24000 gamepad 1 button 0 down \
			gamepad 2 axis 1 -22000 gamepad 2 button 0 down \
			wait 5000 \
			bridge-mouse left up \
			gamepad 1 button 0 up gamepad 1 axis 1 0 \
			gamepad 2 button 0 up gamepad 2 axis 1 0
		sent=1
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then break; fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
[[ $sent -eq 1 ]]

if rg -q "Split(NetLifecycle|NetStat)Assert: FAIL|SplitInputAssertCmd: FAIL|SplitUIAssert: FAIL|ERROR:|Sys_Error" "$LOG"; then
	rg "Split(NetLifecycle|NetStat|Input)Assert|SplitUIAssert|SplitNet party:|ERROR:|Sys_Error" "$LOG"
	exit 1
fi
[[ $(rg -c "SplitNetLifecycleAssert: PASS" "$LOG") -eq 3 ]]
[[ $(rg -c "SplitNetStatAssert: PASS" "$LOG") -eq 3 ]]
[[ $(rg -c "SplitInputAssertCmd: PASS" "$LOG") -eq 6 ]]
test -s "$HOMEPATH/base/screenshots/phase5_powerduel_active.png"
test -s "$HOMEPATH/base/screenshots/phase5_powerduel_input.png"
echo "three-player Power Duel QA passed"
echo "screenshots: $HOMEPATH/base/screenshots"
echo "log: $LOG"
