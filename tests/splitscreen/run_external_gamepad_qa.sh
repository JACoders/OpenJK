#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
MODULE_ARCH="${OPENJK_MODULE_ARCH:-arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
CFG_DST="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/qa-logs/external_gamepad_bridge.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29180}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_gamepad_bridge.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/ui${MODULE_ARCH}.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*"${MODULE_ARCH}".dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgame${MODULE_ARCH}.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" \
	+set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port 29170 +set r_fullscreen 0 +set in_joystick 1 \
	+exec splitqa/external_gamepad_bridge.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

ready=0
for _ in $(seq 1 300); do
	if rg -q "ExternalGamepadBridge: READY" "$LOG" 2>/dev/null; then
		ready=1
		break
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done
if [[ $ready -ne 1 ]]; then
	echo "OpenJK did not reach the external gamepad bridge checkpoint" >&2
	wait "$game_pid" || true
	exit 1
fi

OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
	gamepad 1 axis 1 -24000 \
	gamepad 2 axis 0 -20000 \
	gamepad 3 button 0 down \
	wait 4000 \
	gamepad 1 axis 1 0 \
	gamepad 2 axis 0 0 \
	gamepad 3 button 0 up

wait "$game_pid"
trap - EXIT

rg -q "External gamepad bridge: 3 SDL gamepads listening" "$LOG"
rg -q "SplitInputAssertCmd: PASS player=2.*expectedForward=-998" "$LOG"
rg -q "SplitInputAssertCmd: PASS player=3.*expectedRight=-998" "$LOG"
rg -q "SplitInputAssertCmd: PASS player=4.*expectedButtons=1" "$LOG"
if rg -q "SplitInputAssertCmd: FAIL" "$LOG"; then
	echo "external gamepad bridge assertion failed" >&2
	exit 1
fi

echo "external gamepad bridge QA passed"
echo "log: $LOG"
