#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
CFG_DST="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/qa-logs/external_menu_probe.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29181}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_menu_probe.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29171 \
	+set r_fullscreen 0 +set in_joystick 1 +set developer 1 +exec splitqa/external_menu_probe.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

top_sent=0
profile_sent=0
character_sent=0
saber_open_sent=0
saber_change_sent=0
force_open_sent=0
force_change_sent=0
keyboard_open_sent=0
keyboard_type_sent=0
cvars_sent=0
cheats_sent=0
multi_profile_sent=0
join_sent=0
spectate_sent=0
controls_sent=0
controls_back_sent=0
console_sent=0
for _ in $(seq 1 1800); do
	if rg -q "ExternalMenuProbe: READY_TOP" "$LOG" 2>/dev/null; then
		if [[ $top_sent -eq 0 ]]; then
			OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 6 down wait 600 gamepad 1 button 6 up
			top_sent=1
		fi
	fi
	if rg -q "ExternalMenuProbe: READY_PROFILE" "$LOG" 2>/dev/null; then
		if [[ $profile_sent -eq 0 ]]; then
			OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
				gamepad 1 button 14 tap wait 180 \
				gamepad 1 button 14 tap wait 180 \
				gamepad 1 button 0 tap
			profile_sent=1
		fi
	fi
	if rg -q "ExternalMenuProbe: READY_CHARACTER" "$LOG" 2>/dev/null && [[ $character_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 14 tap
		character_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_SABER_OPEN" "$LOG" 2>/dev/null && [[ $saber_open_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 9 tap
		saber_open_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_SABER_CHANGE" "$LOG" 2>/dev/null && [[ $saber_change_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 12 tap wait 180 gamepad 1 button 12 tap wait 180 gamepad 1 button 0 tap
		saber_change_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_FORCE_OPEN" "$LOG" 2>/dev/null && [[ $force_open_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 10 tap
		force_open_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_FORCE_CHANGE" "$LOG" 2>/dev/null && [[ $force_change_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 12 tap wait 180 gamepad 1 button 12 tap wait 180 \
			gamepad 1 button 0 tap wait 180 gamepad 1 button 6 tap
		force_change_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_KEYBOARD_OPEN" "$LOG" 2>/dev/null && [[ $keyboard_open_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 2 tap
		keyboard_open_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_KEYBOARD_TYPE" "$LOG" 2>/dev/null && [[ $keyboard_type_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 0 tap wait 150 \
			gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 \
			gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 \
			gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 0 tap
		keyboard_type_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_CVARS" "$LOG" 2>/dev/null && [[ $cvars_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 2 tap wait 180 \
			gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 \
			gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 0 tap
		cvars_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_CHEATS" "$LOG" 2>/dev/null && [[ $cheats_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 1 tap wait 180 gamepad 1 button 2 tap wait 180 \
			gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 gamepad 1 button 12 tap wait 120 \
			gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 14 tap wait 120 gamepad 1 button 0 tap
		cheats_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_MULTI_PROFILE" "$LOG" 2>/dev/null && [[ $multi_profile_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 1 tap wait 180 gamepad 1 button 1 tap wait 180 \
			gamepad 1 button 14 tap wait 180 gamepad 2 button 14 tap
		multi_profile_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_JOIN" "$LOG" 2>/dev/null && [[ $join_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 0 tap
		join_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_SPECTATE" "$LOG" 2>/dev/null && [[ $spectate_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 3 tap
		spectate_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_CONTROLS" "$LOG" 2>/dev/null && [[ $controls_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 14 tap wait 180 \
			gamepad 1 button 14 tap wait 180 \
			gamepad 1 button 14 tap wait 180 \
			gamepad 1 button 14 tap wait 180 \
			gamepad 1 button 0 tap
		controls_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_CONTROLS_BACK" "$LOG" 2>/dev/null && [[ $controls_back_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 1 tap wait 180 gamepad 1 button 1 tap
		controls_back_sent=1
	fi
	if rg -q "ExternalMenuProbe: READY_CONSOLE" "$LOG" 2>/dev/null && [[ $console_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 4 down wait 120 gamepad 1 button 6 down wait 500 \
			gamepad 1 button 6 up wait 120 gamepad 1 button 4 up
		console_sent=1
		break
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
rg "SplitUIStatus:" "$LOG"
rg -q "SplitNetStateAssert: PASS player=2 expectedTeam=SPECTATOR" "$LOG"
if rg -q "SplitUIAssert: FAIL|SplitNetStateAssert: FAIL" "$LOG"; then
	exit 1
fi
