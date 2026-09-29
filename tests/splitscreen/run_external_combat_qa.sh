#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
CFG_DST="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/qa-logs/external_combat_probe.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29185}"

mkdir -p "$CFG_DST" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/external_combat_probe.cfg" "$CFG_DST/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29175 \
	+set r_fullscreen 0 +set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/external_combat_probe.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

declare -A sent=()
send_once() {
	local marker=$1
	shift
	if rg -q "ExternalCombatProbe: $marker" "$LOG" 2>/dev/null && [[ -z ${sent[$marker]+x} ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" "$@"
		sent[$marker]=1
	fi
}

for _ in $(seq 1 1600); do
	send_once READY_P1_ATTACK bridge-mouse left down wait 6000 bridge-mouse left up
	send_once READY_P2_RESPAWN gamepad 1 button 0 tap
	send_once READY_P2_ATTACK gamepad 1 button 0 down wait 6000 gamepad 1 button 0 up
	send_once READY_P1_RESPAWN bridge-mouse-move -10000 -10000 wait 120 bridge-mouse-move 38 219 wait 120 bridge-mouse left tap
	send_once READY_P2_FORCE gamepad 1 button 10 tap wait 500 gamepad 1 button 10 tap wait 500 gamepad 1 button 10 tap
	if ! kill -0 "$game_pid" 2>/dev/null; then
		break
	fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
rg "Split(NetLifecycle|NetStat|Input)Assert|SplitNetStagePair|ExternalCombatProbe:" "$LOG"
for marker in READY_P1_ATTACK READY_P2_RESPAWN READY_P2_ATTACK READY_P1_RESPAWN READY_P2_FORCE; do
	rg -q "ExternalCombatProbe: $marker" "$LOG"
done
if rg -q "Split(NetLifecycle|NetStat|ServerCmd)Assert: FAIL|SplitInputAssertCmd: FAIL|SplitUIAssert: FAIL|SplitNetStagePair: FAIL" "$LOG"; then
	exit 1
fi
echo "external split-screen combat QA passed"
echo "log: $LOG"
