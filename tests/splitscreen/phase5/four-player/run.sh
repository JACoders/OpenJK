#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-/tmp/openjk_phase5_4p}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
LOG="$HOMEPATH/base/qa-logs/combat.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29204}"

mkdir -p "$HOMEPATH/base/splitqa" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/phase5/four-player/combat.cfg" "$HOMEPATH/base/splitqa/phase5_4p_combat.cfg"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do cp "$module" "$HOMEPATH/base/"; done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
cp "$BUILD_DIR/codemp/rd-vanilla/rd-vanilla_arm64.dylib" "$(dirname "$BIN")/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29194 \
	+set r_fullscreen 0 +set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/phase5_4p_combat.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

declare -A sent=()
send_once() {
	local marker=$1
	shift
	if rg -q "Phase5FourCombat: $marker" "$LOG" 2>/dev/null && [[ -z ${sent[$marker]+x} ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" "$@"
		sent[$marker]=1
	fi
}

for _ in $(seq 1 3200); do
	send_once READY_P1_KILL_P2 bridge-mouse left down wait 6000 bridge-mouse left up
	send_once READY_RESPAWN_P2 gamepad 1 button 0 tap
	send_once READY_P2_KILL_P3 gamepad 1 button 0 down wait 6000 gamepad 1 button 0 up
	send_once READY_RESPAWN_P3 gamepad 2 button 0 tap
	send_once READY_P3_KILL_P4 gamepad 2 button 0 down wait 6000 gamepad 2 button 0 up
	send_once READY_RESPAWN_P4 gamepad 3 button 0 tap
	send_once READY_P4_KILL_P1 gamepad 3 button 0 down wait 6000 gamepad 3 button 0 up
	send_once READY_RESPAWN_P1 bridge-mouse-move -10000 -10000 wait 120 bridge-mouse-move 38 219 wait 120 bridge-mouse left tap
	send_once READY_ALL_ATTACK \
		bridge-mouse left down \
		gamepad 1 axis 1 -24000 gamepad 1 button 0 down \
		gamepad 2 axis 1 -22000 gamepad 2 button 0 down \
		gamepad 3 axis 1 -20000 gamepad 3 button 0 down \
		wait 6000 \
		bridge-mouse left up \
		gamepad 1 button 0 up gamepad 1 axis 1 0 \
		gamepad 2 button 0 up gamepad 2 axis 1 0 \
		gamepad 3 button 0 up gamepad 3 axis 1 0
	send_once READY_ALL_FORCE \
		gamepad 1 button 10 tap gamepad 2 button 10 tap gamepad 3 button 10 tap wait 500 \
		gamepad 1 button 10 tap gamepad 2 button 10 tap gamepad 3 button 10 tap
	if ! kill -0 "$game_pid" 2>/dev/null; then break; fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
for marker in READY_P1_KILL_P2 READY_RESPAWN_P2 READY_P2_KILL_P3 READY_RESPAWN_P3 READY_P3_KILL_P4 READY_RESPAWN_P4 READY_P4_KILL_P1 READY_RESPAWN_P1 READY_ALL_ATTACK READY_ALL_FORCE; do
	rg -q "Phase5FourCombat: $marker" "$LOG"
done
if rg -q "Split(NetLifecycle|NetStat)Assert: FAIL|SplitInputAssertCmd: FAIL|SplitUIAssert: FAIL|SplitNetStagePair: FAIL" "$LOG"; then
	rg "Split(NetLifecycle|NetStat|Input)Assert|SplitUIAssert|SplitNetStagePair|Phase5FourCombat:" "$LOG"
	exit 1
fi
[[ $(rg -c "SplitNetLifecycleAssert: PASS" "$LOG") -ge 16 ]]
echo "four-player combat QA passed"
echo "screenshots: $HOMEPATH/base/screenshots"
echo "log: $LOG"
