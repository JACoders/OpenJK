#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
players="${1:?players required}"
pure="${2:?pure flag required}"
home="${OPENJK_E2E_HOMEPATH:?Phase 0 home required}"

mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
PYTHONDONTWRITEBYTECODE=1 python3 "$HERE/generate.py" "$players" "$pure" \
	>"$home/base/splitqa/compatibility.cfg"
cp "$BUILD/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$home/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$home/base/"

set +e
"$BIN" \
	+set fs_basepath "$BASE" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set in_joystick 1 +set com_introplayed 1 \
	+exec splitqa/compatibility.cfg
status=$?
set -e

find "$home" -type f -path '*/screenshots/gp3_compat_*.png' \
	-exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \; 2>/dev/null || true
find "$home" -type f -name gp3_compat_console.txt \
	-exec cp {} "$OPENJK_E2E_RUN_DIR/" \; 2>/dev/null || true
exit "$status"
