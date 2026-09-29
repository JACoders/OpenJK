#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
CFG_DST="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/qa-logs/external_topmenu_probe.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29182}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_topmenu_probe.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29172 \
	+set r_fullscreen 0 +set in_joystick 1 +set developer 1 +exec splitqa/external_topmenu_probe.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

declare -A sent=()
send_once() {
	local marker=$1
	shift
	if rg -q "ExternalTopProbe: $marker" "$LOG" 2>/dev/null && [[ -z ${sent[$marker]+x} ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" "$@"
		sent[$marker]=1
	fi
}

for _ in $(seq 1 2400); do
	send_once READY_ABOUT gamepad 1 button 0 tap
	send_once READY_ABOUT_BACK gamepad 1 button 1 tap
	send_once READY_JOIN gamepad 1 button 14 tap wait 150 gamepad 1 button 0 tap
	send_once READY_JOIN_BACK gamepad 1 button 1 tap
	send_once READY_PROFILE gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 0 tap
	send_once READY_PROFILE_BACK gamepad 1 button 1 tap
	send_once READY_ADDBOT gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 0 tap
	send_once READY_ADDBOT_BACK gamepad 1 button 1 tap
	send_once READY_CONTROLS gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 0 tap
	send_once READY_CONTROLS_BACK gamepad 1 button 1 tap
	send_once READY_SETUP gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 0 tap
	send_once READY_SETUP_BACK gamepad 1 button 1 tap
	send_once READY_VOTE gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 0 tap
	send_once READY_VOTE_BACK gamepad 1 button 1 tap
	send_once READY_CALLVOTE gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 14 tap wait 150 gamepad 1 button 0 tap
	send_once READY_CALLVOTE_BACK gamepad 1 button 1 tap
	send_once READY_EXIT gamepad 1 button 13 tap wait 150 gamepad 1 button 0 tap
	send_once READY_EXIT_BACK gamepad 1 button 1 tap
	send_once READY_OWNER_LOCK_OPEN gamepad 1 button 0 tap
	send_once READY_OWNER_LOCK_INTRUDE gamepad 2 button 14 tap wait 150 gamepad 2 button 0 tap
	send_once READY_OWNER_SWITCH gamepad 2 button 6 tap
	send_once READY_OWNER_SWITCH_BACK gamepad 2 button 1 tap
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
rg "SplitUIAssert:" "$LOG"
if rg -q "SplitUIAssert: FAIL|SplitNetStateAssert: FAIL" "$LOG"; then
	exit 1
fi
