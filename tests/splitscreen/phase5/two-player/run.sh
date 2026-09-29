#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-/tmp/openjk_phase5_duel}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
LOG="$HOMEPATH/base/qa-logs/duel.stdout.txt"
PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:-29205}"

mkdir -p "$HOMEPATH/base/splitqa" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/phase5/two-player/duel.cfg" "$HOMEPATH/base/splitqa/phase5_duel.cfg"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do cp "$module" "$HOMEPATH/base/"; done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
cp "$BUILD_DIR/codemp/rd-vanilla/rd-vanilla_arm64.dylib" "$(dirname "$BIN")/"
"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null

OPENJK_VIRTUAL_GAMEPADS=1 OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29196 \
	+set r_fullscreen 0 +set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/phase5_duel.cfg >"$LOG" 2>&1 &
game_pid=$!
trap 'kill "$game_pid" 2>/dev/null || true' EXIT

attack_sent=0
respawn_sent=0
for _ in $(seq 1 2200); do
	if [[ $attack_sent -eq 0 ]] && rg -q "Phase5Duel: READY_P1_ATTACK" "$LOG" 2>/dev/null; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" bridge-mouse left down wait 6000 bridge-mouse left up
		attack_sent=1
	fi
	if [[ $respawn_sent -eq 0 ]] && rg -q "Phase5Duel: READY_P2_RESPAWN" "$LOG" 2>/dev/null; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$PORT" "$SIM" gamepad 1 button 0 tap
		respawn_sent=1
	fi
	if ! kill -0 "$game_pid" 2>/dev/null; then break; fi
	sleep 0.1
done

wait "$game_pid"
trap - EXIT
[[ $attack_sent -eq 1 ]]
[[ $respawn_sent -eq 1 ]]
if rg -q "Split(NetLifecycle|NetStat)Assert: FAIL|SplitUIAssert: FAIL|SplitNetStagePair: FAIL|ERROR:|Sys_Error" "$LOG"; then
	rg "Split(NetLifecycle|NetStat)Assert|SplitUIAssert|SplitNetStagePair|Phase5Duel:|ERROR:|Sys_Error" "$LOG"
	exit 1
fi
[[ $(rg -c "SplitNetLifecycleAssert: PASS" "$LOG") -eq 5 ]]
[[ $(rg -c "SplitNetStatAssert: PASS" "$LOG") -eq 3 ]]
for shot in ready round_end respawned; do test -s "$HOMEPATH/base/screenshots/phase5_duel_${shot}.png"; done
echo "two-player Duel QA passed"
echo "screenshots: $HOMEPATH/base/screenshots"
echo "log: $LOG"
