#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
home="${OPENJK_E2E_RUN_DIR:?Phase 0 run directory required}/server-home"

mkdir -p "$home/base"
cp "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" "$home/base/"

"$BUILD_DIR/openjkded.x86_64" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$home" +set vm_game 0 \
	+set net_port "$OPENJK_E2E_SERVER_PORT" +set dedicated 2 +set logfile 2 \
	+set sv_pure 0 +set sv_maxclients 12 +set g_maxConnPerIP 12 \
	+set sv_reconnectlimit 0 +set g_password none \
	+set g_gametype 0 +set fraglimit 0 \
	+set sv_hostname GP3_WAN_Churn +map mp/ffa3 &
server_pid=$!

# The Phase 0 harness intentionally stops support processes after the client
# exits. Translate that expected teardown into a clean support-process result.
shutdown() {
	kill -TERM "$server_pid" 2>/dev/null || true
	wait "$server_pid" 2>/dev/null || true
	exit 0
}
trap shutdown TERM INT
wait "$server_pid"
