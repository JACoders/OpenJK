#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/network/recovery"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
home="$OPENJK_E2E_HOMEPATH/client"
mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$BUILD/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$home/base/"
cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
python3 "$HERE/generate_cfg.py" "$OPENJK_E2E_PLAYERS" "127.0.0.1:$OPENJK_E2E_SERVER_PORT" \
	>"$home/base/splitqa/recovery.cfg"
"$BIN" +set fs_basepath "$BASE" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set r_fullscreen 0 \
	+set s_initsound 0 +set in_joystick 1 +exec splitqa/recovery.cfg
cp "$home"/base/screenshots/gp3_recovery_*.png "$OPENJK_E2E_SCREENSHOTS/" 2>/dev/null || true
cp "$home/base/gp3_recovery_console.txt" "$OPENJK_E2E_SCREENSHOTS/" 2>/dev/null || true
