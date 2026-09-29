#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
players="${1:?player count required}"
mode="${2:?mode required}"
home="${OPENJK_E2E_HOMEPATH:?Phase 0 harness homepath required}"

case "$players" in
	2|3|4) ;;
	*) echo "players must be 2, 3, or 4" >&2; exit 2 ;;
esac

case "$mode" in
	ctf) gametype=8 ;;
	cty) gametype=9 ;;
	*) echo "mode must be ctf or cty" >&2; exit 2 ;;
esac

mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$HERE"/objective_*.cfg "$home/base/splitqa/"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$BUILD_DIR"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" "$home/base/"

set +e
"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set in_joystick 1 +set developer 1 +set com_introplayed 1 \
	+set g_gametype "$gametype" +set ui_splitScreenPlayerCount "$players" \
	+exec "splitqa/objective_${players}p.cfg"
status=$?
set -e

find "$home" -type f -path '*/screenshots/objective_*.png' \
	-exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \; 2>/dev/null || true
find "$home" -type f -name "objective_console.txt" \
	-exec cp {} "$OPENJK_E2E_RUN_DIR/" \;
exit "$status"
