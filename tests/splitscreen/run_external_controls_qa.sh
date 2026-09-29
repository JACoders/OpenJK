#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
CFG_DST="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/qa-logs/external_controls_probe.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29183}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_controls_probe.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29173 \
	+set r_fullscreen 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/external_controls_probe.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

open_sent=0
interaction_sent=0
weapons_sent=0
force1_sent=0
force2_sent=0
mouse_sent=0
other_sent=0
bind_sent=0
back_sent=0
for _ in $(seq 1 1200); do
	if rg -q "ExternalControlsProbe: READY_OPEN" "$LOG" 2>/dev/null && [[ $open_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 14 tap wait 100 gamepad 1 button 14 tap wait 100 \
			gamepad 1 button 14 tap wait 100 gamepad 1 button 14 tap wait 100 gamepad 1 button 0 tap wait 250 \
			gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap wait 100 \
			gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap
		open_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_INTERACTION" "$LOG" 2>/dev/null && [[ $interaction_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap
		interaction_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_WEAPONS" "$LOG" 2>/dev/null && [[ $weapons_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap
		weapons_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_FORCE1" "$LOG" 2>/dev/null && [[ $force1_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap
		force1_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_FORCE2" "$LOG" 2>/dev/null && [[ $force2_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap
		force2_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_MOUSE" "$LOG" 2>/dev/null && [[ $mouse_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap
		mouse_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_OTHER" "$LOG" 2>/dev/null && [[ $other_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 12 tap wait 100 gamepad 1 button 0 tap
		other_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_BIND" "$LOG" 2>/dev/null && [[ $bind_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 11 tap wait 100 gamepad 1 button 11 tap wait 100 \
			gamepad 1 button 11 tap wait 100 gamepad 1 button 11 tap wait 100 \
			gamepad 1 button 11 tap wait 100 gamepad 1 button 0 tap wait 150 \
			gamepad 1 button 14 tap wait 150 gamepad 1 button 11 tap wait 150 \
			gamepad 1 button 0 tap wait 150 \
			gamepad 1 button 9 tap
		bind_sent=1
	fi
	if rg -q "ExternalControlsProbe: READY_BACK" "$LOG" 2>/dev/null && [[ $back_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 1 tap
		back_sent=1
		break
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
rg -q "SplitUIAssert: PASS cvar=ui_splitScreenMenuMode expected=controls" "$LOG"
rg -q "SplitUIAssert: PASS cvar=ui_splitScreenMenuMode expected=top" "$LOG"
if rg -q "SplitUIAssert: FAIL" "$LOG"; then
	exit 1
fi

echo "external stock controls QA passed"
echo "log: $LOG"
