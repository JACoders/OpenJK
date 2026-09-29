#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
players="${1:?player count required}"
home="${OPENJK_E2E_HOMEPATH:?Phase 0 homepath required}"
server="127.0.0.1:${OPENJK_E2E_SERVER_PORT:?server port required}"

mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
python3 "$HERE/generate_cfg.py" "$players" "$server" >"$home/base/splitqa/wan_churn.cfg"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$BUILD_DIR"/codemp/cgame/cgame*x86_64.dylib "$home/base/"

set +e
"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set net_qport "$OPENJK_E2E_QPORT" \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 \
	+set com_introplayed 1 +exec splitqa/wan_churn.cfg
status=$?
set -e

find "$home" -type f -path '*/screenshots/gp3_wan_churn_*.png' \
	-exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \; 2>/dev/null || true
find "$home" -type f -name gp3_wan_churn_console.txt \
	-exec cp {} "$OPENJK_E2E_RUN_DIR/" \; 2>/dev/null || true
exit "$status"
