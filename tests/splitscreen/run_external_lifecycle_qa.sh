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
LOG="$HOMEPATH/base/qa-logs/external_lifecycle_probe.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29184}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_lifecycle_probe.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/ui${MODULE_ARCH}.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*"${MODULE_ARCH}".dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgame${MODULE_ARCH}.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29174 \
	+set r_fullscreen 0 +set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/external_lifecycle_probe.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

respawn_all_sent=0
respawn_p2_sent=0
spectate_p3_sent=0
join_p3_sent=0
for _ in $(seq 1 1400); do
	if rg -q "ExternalLifecycleProbe: READY_RESPAWN_ALL" "$LOG" 2>/dev/null && [[ $respawn_all_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			bridge-mouse-move -10000 -10000 wait 120 bridge-mouse-move 38 219 wait 120 \
			bridge-mouse left tap wait 300 \
			gamepad 1 button 0 down wait 350 gamepad 1 button 0 up wait 100 \
			gamepad 2 button 0 down wait 350 gamepad 2 button 0 up wait 100 \
			gamepad 3 button 0 down wait 350 gamepad 3 button 0 up wait 800 \
			bridge-mouse left down wait 3000 bridge-mouse left up
		respawn_all_sent=1
	fi
	if rg -q "ExternalLifecycleProbe: READY_RESPAWN_P2" "$LOG" 2>/dev/null && [[ $respawn_p2_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 0 down wait 400 gamepad 1 button 0 up
		respawn_p2_sent=1
	fi
	if rg -q "ExternalLifecycleProbe: READY_SPECTATE_P3" "$LOG" 2>/dev/null && [[ $spectate_p3_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 2 button 3 tap
		spectate_p3_sent=1
	fi
	if rg -q "ExternalLifecycleProbe: READY_JOIN_P3" "$LOG" 2>/dev/null && [[ $join_p3_sent -eq 0 ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" \
			wait 5200 gamepad 2 button 14 tap wait 180 gamepad 2 button 0 tap wait 300 gamepad 2 button 0 tap
		join_p3_sent=1
		break
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
if rg -q "Split(NetLifecycle|Input)Assert.*: FAIL" "$LOG"; then
	rg "Split(NetLifecycle|Input)Assert|ExternalLifecycleProbe:" "$LOG"
	exit 1
fi
for marker in READY_RESPAWN_ALL READY_RESPAWN_P2 READY_SPECTATE_P3 READY_JOIN_P3; do
	rg -q "ExternalLifecycleProbe: $marker" "$LOG"
done
[[ $(rg -c "SplitNetLifecycleAssert: PASS" "$LOG") -ge 18 ]]
echo "external split-screen lifecycle QA passed"
echo "log: $LOG"
