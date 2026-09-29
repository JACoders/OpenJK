#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
QA_ROOT="${OPENJK_ONLINE_2P_ROOT:-$(mktemp -d /private/tmp/openjk-online-2p.XXXXXX)}"
UPSTREAM_BUILD="${OPENJK_VANILLA_BUILD:-/tmp/openjk_upstream_vanilla_2ba50212/build-arm64-vanilla2}"
SPLIT_BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
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
		echo "Missing online two-player QA artifact: $required" >&2
		exit 2
	fi
done

if rg -q 'splitscreen_setup|splitnet_applyprofile|set (ui_splitScreenP[12]Model|ui_splitScreenP[12]ForcePowers|model|forcepowers)|^(connect|splitnet_party_connect)[[:space:]]' \
	"$ROOT/tests/splitscreen/cfg/routed_online_2p_acceptance.cfg"; then
	echo "Online two-player acceptance must select profiles and join through the visible UI" >&2
	exit 2
fi

mkdir -p \
	"$SERVER_HOME/base" \
	"$SPLIT_HOME/base/splitqa" \
	"$SPLIT_HOME/base/screenshots" \
	"$REMOTE_HOME/base/splitqa" \
	"$REMOTE_HOME/base/screenshots" \
	"$LOG_DIR"
"$ROOT/tests/splitscreen/install_assets.sh" "$SPLIT_HOME" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/routed_online_2p_acceptance.cfg" "$SPLIT_HOME/base/splitqa/"
cp "$ROOT/tests/splitscreen/cfg/online_remote_third.cfg" "$REMOTE_HOME/base/splitqa/"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$SPLIT_HOME/base/"
cp "$BUILD_DIR"/codemp/cgame/cgame*x86_64.dylib "$SPLIT_HOME/base/"
cp "$VANILLA_GAME" "$SERVER_HOME/base/"
cp "$VANILLA_CGAME" "$REMOTE_HOME/base/"
cp "$VANILLA_UI" "$REMOTE_HOME/base/"

"$VANILLA_SERVER" \
	+set fs_basepath "$BASEPATH" \
	+set fs_homepath "$SERVER_HOME" \
	+set vm_game 0 \
	+set net_port 29240 \
	+set dedicated 1 \
	+set sv_pure 0 \
	+set sv_maxclients 8 \
	+set g_maxConnPerIP 8 \
	+set g_password none \
	+set sv_hostname "SplitQA Network 2P + Remote" \
	+set rconPassword splitqa \
	+set g_gametype 0 \
	+set fraglimit 20 \
	+map mp/ffa3 >"$SERVER_LOG" 2>&1 &
server_pid=$!

for _ in $(seq 1 100); do
	if rg -q "Game Initialization" "$SERVER_LOG" 2>/dev/null; then
		break
	fi
	if ! kill -0 "$server_pid" 2>/dev/null; then
		echo "Vanilla server exited during startup" >&2
		tail -n 120 "$SERVER_LOG" >&2
		exit 1
	fi
	sleep 0.1
done
rg -q "VM_Create: jampgamearm64.dylib succeeded" "$SERVER_LOG"

"$VANILLA_CLIENT" \
	+set fs_basepath "$BASEPATH" \
	+set fs_homepath "$REMOTE_HOME" \
	+set vm_game 0 \
	+set vm_cgame 0 \
	+set vm_ui 0 \
	+set net_port 29241 \
	+set password "" \
	+set r_fullscreen 0 \
	+set r_mode 3 \
	+set s_initsound 0 \
	+set logfile 2 \
	+exec splitqa/online_remote_third.cfg >"$REMOTE_LOG" 2>&1 &
remote_pid=$!

for _ in $(seq 1 300); do
	if rg -q "ChangeTeam: .*Online_Remote3.*SPECTATOR -> FREE" "$SERVER_LOG" 2>/dev/null; then
		break
	fi
	if ! kill -0 "$remote_pid" 2>/dev/null; then
		echo "Remote vanilla client exited before joining" >&2
		tail -n 120 "$REMOTE_LOG" >&2
		exit 1
	fi
	sleep 0.1
done
rg -q "ChangeTeam: .*Online_Remote3.*SPECTATOR -> FREE" "$SERVER_LOG"

"$SPLIT_BIN" \
	+set fs_basepath "$BASEPATH" \
	+set fs_homepath "$SPLIT_HOME" \
	+set vm_game 0 \
	+set vm_cgame 0 \
	+set vm_ui 0 \
	+set net_port 29242 \
	+set password "" \
	+set r_fullscreen 0 \
	+set r_mode 3 \
	+set s_initsound 0 \
	+set in_joystick 1 \
	+set logfile 2 \
	+set developer 1 \
	+set com_introplayed 1 \
	+exec splitqa/routed_online_2p_acceptance.cfg >"$SPLIT_LOG" 2>&1

if rg -n "Split(UI|Input|NetLifecycle|NetStat|Profile)Assert: FAIL|@@@TOO_MANY_INFO|ERROR:|recursive error|segmentation|assertion" \
	"$SPLIT_LOG" "$REMOTE_LOG" "$SERVER_LOG"; then
	echo "Online two-player acceptance emitted a failure marker" >&2
	exit 1
fi

required_split_patterns=(
	"SplitUIAssert: PASS cvar=ui_splitScreenSessionType expected=server actual=server"
	"SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=join_pending actual=join_pending"
	"SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active"
	"SplitUIAssert: PASS cvar=ui_splitScreenP2Joined expected=1 actual=1"
	"SplitNet party: all 2 local players active"
	"SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE"
	"SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE"
	"SplitInputAssertCmd: PASS player=2 expectedForward=127"
	"SplitInputAssertCmd: PASS player=2 .*expectedButtons=1 actualButtons=1"
	"players : 3 humans"
)
for pattern in "${required_split_patterns[@]}"; do
	if ! rg -q "$pattern" "$SPLIT_LOG"; then
		echo "Missing split-client evidence: $pattern" >&2
		exit 1
	fi
done

for name in Online_Remote3 Online_P1_Kyle Online_P2_Desann; do
	if ! rg -q "$name" "$SERVER_LOG"; then
		echo "Server never observed participant: $name" >&2
		exit 1
	fi
done

# A secondary split client must release its server slot immediately on quit.
# Connectionless disconnect packets are ignored by vanilla servers, so this
# also guards the reliable netchan teardown path used by public servers.
rg -q "SplitNet P2: sent reliable disconnect" "$SPLIT_LOG"
rg -q 'ClientDisconnect: .*"Online_P2_Desann' "$SERVER_LOG"

rg -q "VM_Create: cgamearm64.dylib succeeded" "$REMOTE_LOG"
rg -q "VM_Create: uiarm64.dylib succeeded" "$REMOTE_LOG"

for frame in \
	online2_chain_01_main_menu.png \
	online2_chain_03_split_screen.png \
	online2_chain_04_server_party.png \
	online2_chain_05_player_setup.png \
	online2_selection_p1_kyle_dark.png \
	online2_selection_p2_desann.png \
	online2_selection_p2_light.png \
	online2_browser_remote_waiting.png \
	online2_gameplay_both_alive.png \
	online2_scoreboard_three_humans.png \
	online2_controller_p2_attack.png; do
	if [[ ! -s "$SPLIT_HOME/base/screenshots/$frame" ]]; then
		echo "Missing split screenshot: $frame" >&2
		exit 1
	fi
done

python3 "$ROOT/tests/splitscreen/assert_screenshot.py" \
	"$SPLIT_HOME/base/screenshots/online2_gameplay_both_alive.png" 2

echo "passed: two routed split-screen clients joined an upstream dedicated server with an upstream remote client"
echo "root: $QA_ROOT"
echo "split screenshots: $SPLIT_HOME/base/screenshots"
echo "logs: $LOG_DIR"
