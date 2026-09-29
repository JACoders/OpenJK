#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
QA_ROOT="${OPENJK_VANILLA_QA_ROOT:-/tmp/openjk_vanilla_network_qa}"
UPSTREAM_BUILD="${OPENJK_VANILLA_BUILD:-/tmp/openjk_upstream_vanilla_2ba50212/build-arm64-vanilla2}"
SPLIT_BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
VANILLA_SERVER="${OPENJK_VANILLA_SERVER:-$UPSTREAM_BUILD/openjkded.arm64}"
VANILLA_CLIENT="${OPENJK_VANILLA_CLIENT:-$UPSTREAM_BUILD/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
VANILLA_GAME="${OPENJK_VANILLA_GAME:-$UPSTREAM_BUILD/codemp/game/jampgamearm64.dylib}"
VANILLA_CGAME="${OPENJK_VANILLA_CGAME:-$UPSTREAM_BUILD/codemp/cgame/cgamearm64.dylib}"
VANILLA_UI="${OPENJK_VANILLA_UI:-$UPSTREAM_BUILD/codemp/ui/uiarm64.dylib}"

SERVER_HOME="$QA_ROOT/server"
SPLIT_HOME="$QA_ROOT/split"
REMOTE_HOME="$QA_ROOT/remote"
LOG_DIR="$QA_ROOT/logs"
SERVER_LOG="$LOG_DIR/server.log"
SPLIT_LOG="$LOG_DIR/split.log"
REMOTE_LOG="$LOG_DIR/remote.log"
server_pid=""
remote_pid=""

cleanup() {
	if [[ -n "$remote_pid" ]] && kill -0 "$remote_pid" 2>/dev/null; then
		kill "$remote_pid" 2>/dev/null || true
		wait "$remote_pid" 2>/dev/null || true
	fi
	if [[ -n "$server_pid" ]] && kill -0 "$server_pid" 2>/dev/null; then
		kill "$server_pid" 2>/dev/null || true
		wait "$server_pid" 2>/dev/null || true
	fi
}
trap cleanup EXIT

for required in "$SPLIT_BIN" "$VANILLA_SERVER" "$VANILLA_CLIENT" "$VANILLA_GAME" "$VANILLA_CGAME" "$VANILLA_UI"; do
	if [[ ! -f "$required" ]]; then
		echo "Missing vanilla-network QA artifact: $required" >&2
		exit 2
	fi
done

mkdir -p "$SERVER_HOME/base" "$SPLIT_HOME/base/splitqa" "$REMOTE_HOME/base/splitqa" "$LOG_DIR"
"$ROOT/tests/splitscreen/install_assets.sh" "$SPLIT_HOME" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/vanilla_party_four.cfg" "$SPLIT_HOME/base/splitqa/"
cp "$ROOT/tests/splitscreen/cfg/vanilla_remote_fifth.cfg" "$REMOTE_HOME/base/splitqa/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$SPLIT_HOME/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do
	cp "$cgame_module" "$SPLIT_HOME/base/"
done
cp "$VANILLA_GAME" "$SERVER_HOME/base/"
cp "$VANILLA_CGAME" "$REMOTE_HOME/base/"
cp "$VANILLA_UI" "$REMOTE_HOME/base/"

"$VANILLA_SERVER" \
	+set fs_basepath "$BASEPATH" \
	+set fs_homepath "$SERVER_HOME" \
	+set vm_game 0 \
	+set net_port 29220 \
	+set dedicated 2 \
	+set sv_pure 0 \
	+set sv_maxclients 8 \
	+set g_maxConnPerIP 8 \
	+set g_password none \
	+set sv_hostname SplitQA_Vanilla \
	+set rconPassword splitqa \
	+set g_gametype 0 \
	+set fraglimit 20 \
	+map mp/ffa3 >"$SERVER_LOG" 2>&1 &
server_pid=$!

for _ in $(seq 1 100); do
	if grep -q "Game Initialization" "$SERVER_LOG" 2>/dev/null; then
		break
	fi
	if ! kill -0 "$server_pid" 2>/dev/null; then
		echo "Vanilla server exited during startup" >&2
		tail -n 120 "$SERVER_LOG" >&2
		exit 1
	fi
	sleep 0.1
done
grep -q "VM_Create: jampgamearm64.dylib succeeded" "$SERVER_LOG"

"$VANILLA_CLIENT" \
	+set fs_basepath "$BASEPATH" \
	+set fs_homepath "$REMOTE_HOME" \
	+set vm_game 0 \
	+set vm_cgame 0 \
	+set vm_ui 0 \
	+set net_port 29225 \
	+set password "" \
	+set r_fullscreen 0 \
	+set r_mode -1 \
	+set r_customwidth 1280 \
	+set r_customheight 720 \
	+set s_initsound 0 \
	+set logfile 2 \
	+exec splitqa/vanilla_remote_fifth.cfg >"$REMOTE_LOG" 2>&1 &
remote_pid=$!

"$SPLIT_BIN" \
	+set fs_basepath "$BASEPATH" \
	+set fs_homepath "$SPLIT_HOME" \
	+set vm_game 0 \
	+set vm_cgame 0 \
	+set vm_ui 0 \
	+set net_port 29230 \
	+set password "" \
	+set r_fullscreen 0 \
	+set r_mode -1 \
	+set r_customwidth 1280 \
	+set r_customheight 720 \
	+set s_initsound 0 \
	+set in_joystick 1 \
	+set logfile 2 \
	+exec splitqa/vanilla_party_four.cfg >"$SPLIT_LOG" 2>&1

wait "$remote_pid"
remote_pid=""

if grep -Eiq "segmentation|assertion|fatal|recursive error|ERROR:" "$SPLIT_LOG" "$REMOTE_LOG" "$SERVER_LOG"; then
	echo "Vanilla-network QA emitted an error marker" >&2
	exit 1
fi
if grep -Eq "Split(UI|NetLifecycle|NetStat)Assert: FAIL|SplitNetStagePair: FAIL" "$SPLIT_LOG"; then
	echo "Vanilla-network QA emitted a split-screen assertion failure" >&2
	exit 1
fi

grep -q "SplitNet party: all 4 local players active" "$SPLIT_LOG"
for player in 1 2 3 4; do
	grep -Eq "SplitNetLifecycleAssert: PASS player=$player expected=ALIVE actual=ALIVE" "$SPLIT_LOG"
done
for name in QA_Vanilla_P1 QA_Vanilla_P2 QA_Vanilla_P3 QA_Vanilla_P4 QA_Vanilla_Remote5; do
	grep -q "$name" "$SERVER_LOG"
done
grep -q "players : 5 humans" "$SPLIT_LOG"
grep -Eq 'ChangeTeam: 0 .*QA_Vanilla_Remote5.*SPECTATOR -> FREE' "$SERVER_LOG"
grep -q "VM_Create: cgamearm64.dylib succeeded" "$REMOTE_LOG"
grep -q "VM_Create: uiarm64.dylib succeeded" "$REMOTE_LOG"
test -f "$SPLIT_HOME/base/screenshots/splitqa_vanilla_party_four.png"
test -f "$REMOTE_HOME/base/screenshots/splitqa_vanilla_remote_fifth.png"

echo "passed: four split-screen clients and one untouched upstream client on an untouched upstream server"
echo "split screenshot: $SPLIT_HOME/base/screenshots/splitqa_vanilla_party_four.png"
echo "remote screenshot: $REMOTE_HOME/base/screenshots/splitqa_vanilla_remote_fifth.png"
echo "logs: $LOG_DIR"
