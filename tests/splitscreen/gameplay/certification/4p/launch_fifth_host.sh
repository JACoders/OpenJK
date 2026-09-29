#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/certification/4p"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOME_PATH="${OPENJK_E2E_HOMEPATH:?Phase 0 harness homepath required}"
CFG="$HOME_PATH/base/splitqa/controlled_fifth_host.cfg"

mkdir -p "$HOME_PATH/base/splitqa" "$HOME_PATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOME_PATH" >/dev/null
sed "s/127\\.0\\.0\\.1:29973/127.0.0.1:${OPENJK_E2E_SERVER_PORT:?}/g" \
	"$HERE/controlled_fifth_host.cfg" >"$CFG"
cp "$BUILD/codemp/ui/uix86_64.dylib" "$HOME_PATH/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$HOME_PATH/base/"
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$HOME_PATH/base/"

set +e
"$BIN" \
	+set fs_basepath "$BASE" +set fs_homepath "$HOME_PATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "${OPENJK_E2E_CLIENT_PORT:?}" +set r_fullscreen 0 \
	+set r_mode 3 +set s_initsound 0 +set in_joystick 1 \
	+set developer 1 +set com_introplayed 1 \
	+exec splitqa/controlled_fifth_host.cfg
status=$?
set -e

find "$HOME_PATH/base/screenshots" -type f -name 'gp5_4p_with_remote_fifth.png' \
	-exec cp {} "${OPENJK_E2E_SCREENSHOTS:?}/" \;
exit "$status"
