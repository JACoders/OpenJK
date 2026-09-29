#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
CFG_DST="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/qa-logs/external_team_profile_probe.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29183}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_team_profile_probe.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=1 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29173 \
	+set r_fullscreen 0 +set in_joystick 1 \
	+exec splitqa/external_team_profile_probe.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

sent=0
for _ in $(seq 1 1200); do
	if rg -q "ExternalTeamProfileProbe: READY_BLUE" "$LOG" 2>/dev/null && [[ $sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			gamepad 1 button 0 down wait 220 gamepad 1 button 0 up wait 220 \
			gamepad 1 button 14 down wait 220 gamepad 1 button 14 up wait 220 \
			gamepad 1 button 14 down wait 220 gamepad 1 button 14 up wait 220 \
			gamepad 1 button 0 down wait 220 gamepad 1 button 0 up
		sent=1
		break
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
rg "Split(UI|NetStat|NetLifecycle)Assert:" "$LOG"
if rg -q "Split(UI|NetStat|NetLifecycle)Assert: FAIL" "$LOG"; then
	exit 1
fi
echo "external team profile controller QA passed"
echo "screenshot: $HOMEPATH/base/screenshots/splitqa_external_team_blue.png"
