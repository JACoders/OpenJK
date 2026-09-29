#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="$BUILD/openjkded.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
home="$OPENJK_E2E_HOMEPATH/server"
mkdir -p "$home/base"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$home/base/"
server_pid=""
stop_server() {
	[[ -z "$server_pid" ]] || kill -TERM "$server_pid" 2>/dev/null || true
	[[ -z "$server_pid" ]] || wait "$server_pid" 2>/dev/null || true
	exit 0
}
trap stop_server TERM INT
for generation in 1 2; do
	"$BIN" +set fs_basepath "$BASE" +set fs_homepath "$home" \
		+set vm_game 0 +set net_port "$OPENJK_E2E_SERVER_PORT" \
		+set rconpassword GP3Recovery +set g_password "" \
		+set g_gametype 0 +set g_antiFakePlayer 0 +set sv_maxclients 8 \
		+map mp/ffa3 &
	server_pid=$!
	wait "$server_pid"
	server_pid=""
	[[ "$generation" -eq 2 ]] || sleep 2
done
