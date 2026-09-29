#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
BUILD="$ROOT/build-x86_64"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
UPSTREAM="${OPENJK_UPSTREAM_BUILD:-/tmp/openjk_upstream_vanilla_2ba50212/build-arm64-vanilla2}"
UPSTREAM_SERVER="$UPSTREAM/openjkded.arm64"
UPSTREAM_CLIENT="$UPSTREAM/openjk.arm64.app/Contents/MacOS/openjk.arm64"
EXPECTED="737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/certification/controls"
RUN_ROOT="${GP5_CONTROLS_RUN_ROOT:-$(mktemp -d /private/tmp/openjk-gp5-controls.XXXXXX)}"
LOCAL_HOME="$RUN_ROOT/local"
SERVER_HOME="$RUN_ROOT/server"
REMOTE_HOME="$RUN_ROOT/remote"
CANDIDATE_HOME="$RUN_ROOT/candidate"
LOGS="$RUN_ROOT/logs"
SERVER_PORT=29830
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

actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$EXPECTED" ]] || {
	echo "frozen candidate mismatch before run: $actual" >&2
	exit 2
}
for artifact in \
	"$BIN" \
	"$BUILD/codemp/ui/uix86_64.dylib" \
	"$BUILD/codemp/game/jampgamex86_64.dylib" \
	"$BUILD/codemp/cgame/cgamex86_64.dylib" \
	"$UPSTREAM_SERVER" "$UPSTREAM_CLIENT" \
	"$UPSTREAM/codemp/game/jampgamearm64.dylib" \
	"$UPSTREAM/codemp/cgame/cgamearm64.dylib" \
	"$UPSTREAM/codemp/ui/uiarm64.dylib"; do
	[[ -f "$artifact" ]] || { echo "missing control artifact: $artifact" >&2; exit 2; }
done

mkdir -p \
	"$LOCAL_HOME/base/splitqa" "$LOCAL_HOME/base/screenshots" \
	"$SERVER_HOME/base" \
	"$REMOTE_HOME/base/splitqa" "$REMOTE_HOME/base/screenshots" \
	"$CANDIDATE_HOME/base/splitqa" "$CANDIDATE_HOME/base/screenshots" \
	"$LOGS" "$RESULTS"

"$ROOT/tests/splitscreen/install_assets.sh" "$LOCAL_HOME" >/dev/null
"$ROOT/tests/splitscreen/install_assets.sh" "$CANDIDATE_HOME" >/dev/null
cp "$HERE/cfg/one_player_modes.cfg" "$LOCAL_HOME/base/splitqa/"
cp "$HERE/cfg/candidate_online.cfg" "$CANDIDATE_HOME/base/splitqa/"
cp "$HERE/cfg/upstream_online.cfg" "$REMOTE_HOME/base/splitqa/"
cp "$BUILD/codemp/ui/uix86_64.dylib" "$LOCAL_HOME/base/"
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$LOCAL_HOME/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$LOCAL_HOME/base/"
cp "$BUILD/codemp/ui/uix86_64.dylib" "$CANDIDATE_HOME/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$CANDIDATE_HOME/base/"
cp "$UPSTREAM/codemp/game/jampgamearm64.dylib" "$SERVER_HOME/base/"
cp "$UPSTREAM/codemp/cgame/cgamearm64.dylib" "$REMOTE_HOME/base/"
cp "$UPSTREAM/codemp/ui/uiarm64.dylib" "$REMOTE_HOME/base/"

"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$LOCAL_HOME" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port 29831 +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+set com_introplayed 1 +exec splitqa/one_player_modes.cfg \
	>"$LOGS/local.log" 2>&1

"$UPSTREAM_SERVER" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$SERVER_HOME" \
	+set vm_game 0 +set dedicated 1 +set net_port "$SERVER_PORT" \
	+set sv_pure 0 +set sv_maxclients 8 +set g_maxConnPerIP 8 \
	+set g_password "" +set logfile 2 \
	+set g_gametype 0 +set fraglimit 20 +map mp/ffa3 \
	>"$LOGS/server.log" 2>&1 &
server_pid=$!
for _ in $(seq 1 150); do
	rg -q "Game Initialization" "$LOGS/server.log" 2>/dev/null && break
	kill -0 "$server_pid" 2>/dev/null || {
		echo "upstream control server exited" >&2
		exit 1
	}
	sleep 0.1
done
rg -q "Game Initialization" "$LOGS/server.log"

"$UPSTREAM_CLIENT" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$REMOTE_HOME" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port 29832 +set name GP5_Upstream +set password "" \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 \
	+set com_introplayed 1 +connect "127.0.0.1:$SERVER_PORT" \
	+exec splitqa/upstream_online.cfg >"$LOGS/remote.log" 2>&1 &
remote_pid=$!

"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$CANDIDATE_HOME" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port 29833 +set name GP5_Candidate +set password "" \
	+set cl_splitScreen 0 +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set developer 1 +set com_introplayed 1 \
	+connect "127.0.0.1:$SERVER_PORT" +exec splitqa/candidate_online.cfg \
	>"$LOGS/candidate.log" 2>&1

wait "$remote_pid"
remote_pid=""
cleanup
server_pid=""

final_hash="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$final_hash" == "$EXPECTED" ]] || {
	echo "frozen candidate mismatch after run: $final_hash" >&2
	exit 2
}

PYTHONDONTWRITEBYTECODE=1 python3 "$HERE/validate.py" \
	--run-root "$RUN_ROOT" --results "$RESULTS" \
	--candidate-hash "$final_hash"
echo "$RUN_ROOT"
