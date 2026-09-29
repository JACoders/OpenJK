#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/certification/4p"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
RUN_DIR="${OPENJK_E2E_RUN_DIR:?Phase 0 harness run directory required}"
HOST_LOG="$RUN_DIR/logs/client.log"
HOME_PATH="$RUN_DIR/fifth-home"
CFG="$HOME_PATH/base/splitqa/controlled_fifth_upstream.cfg"

# The independent client must not consume one of clientnums 0..3 before the
# four-player party has proved its stable identities.
for _ in $(seq 1 1800); do
	if rg -q 'GP5-03:FIFTH-READY' "$HOST_LOG" 2>/dev/null; then
		break
	fi
	sleep 0.1
done
rg -q 'GP5-03:FIFTH-READY' "$HOST_LOG"

mkdir -p "$HOME_PATH/base/splitqa" "$HOME_PATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOME_PATH" >/dev/null
sed "s/127\\.0\\.0\\.1:29973/127.0.0.1:${OPENJK_E2E_SERVER_PORT:?}/g" \
	"$HERE/controlled_fifth_upstream.cfg" >"$CFG"
cp "$BUILD/codemp/ui/uix86_64.dylib" "$HOME_PATH/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$HOME_PATH/base/"
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$HOME_PATH/base/"

exec "$BIN" \
	+set fs_basepath "$BASE" +set fs_homepath "$HOME_PATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "${OPENJK_E2E_REMOTE_PORT:?}" +set r_fullscreen 0 \
	+set r_mode 3 +set s_initsound 0 +set in_joystick 0 \
	+set developer 1 +set com_introplayed 1 \
	+exec splitqa/controlled_fifth_upstream.cfg
