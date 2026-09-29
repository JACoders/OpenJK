#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
CFG_DST="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/qa-logs/external_character_probe.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29182}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_character_probe.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29172 \
	+set r_fullscreen 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/external_character_probe.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

right_sent=0
down_sent=0
left_sent=0
up_sent=0
close_sent=0
open_top_sent=0
reopen_sent=0
join_sent=0
for _ in $(seq 1 1400); do
	if rg -q "ExternalCharacterProbe: READY_RIGHT" "$LOG" 2>/dev/null && [[ $right_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 14 down wait 300 gamepad 1 button 14 up
		right_sent=1
	fi
	if rg -q "ExternalCharacterProbe: READY_DOWN" "$LOG" 2>/dev/null && [[ $down_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 12 down wait 300 gamepad 1 button 12 up
		down_sent=1
	fi
	if rg -q "ExternalCharacterProbe: READY_LEFT" "$LOG" 2>/dev/null && [[ $left_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 13 down wait 300 gamepad 1 button 13 up
		left_sent=1
	fi
	if rg -q "ExternalCharacterProbe: READY_UP" "$LOG" 2>/dev/null && [[ $up_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 11 down wait 300 gamepad 1 button 11 up
		up_sent=1
	fi
	if rg -q "ExternalCharacterProbe: READY_CLOSE" "$LOG" 2>/dev/null && [[ $close_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 1 down wait 300 gamepad 1 button 1 up
		close_sent=1
	fi
	if rg -q "ExternalCharacterProbe: READY_OPEN_TOP" "$LOG" 2>/dev/null && [[ $open_top_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 6 down wait 300 gamepad 1 button 6 up
		open_top_sent=1
	fi
	if rg -q "ExternalCharacterProbe: READY_REOPEN" "$LOG" 2>/dev/null && [[ $reopen_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 14 down wait 300 gamepad 1 button 14 up wait 240 \
			gamepad 1 button 14 down wait 300 gamepad 1 button 14 up wait 240 \
			gamepad 1 button 0 down wait 300 gamepad 1 button 0 up
		reopen_sent=1
	fi
	if rg -q "ExternalCharacterProbe: READY_JOIN" "$LOG" 2>/dev/null && [[ $join_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 6 down wait 300 gamepad 1 button 6 up
		join_sent=1
		break
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
rg "ui_splitScreenP[1-4]Model|SplitProfileAssert:|SplitUIStatus:" "$LOG"
if rg -q "Split(Profile|UI|NetLifecycle)Assert: FAIL" "$LOG"; then
	exit 1
fi
