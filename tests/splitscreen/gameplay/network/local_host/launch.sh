#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
role="${1:?role}" players="${2:?players}"
build="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
bin="${OPENJK_BIN:-$build/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
base="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
if [[ "$role" == host ]]; then
	home="$OPENJK_E2E_HOMEPATH"; cfg="host_${players}p.cfg"; port="$OPENJK_E2E_CLIENT_PORT"
else
	home="$OPENJK_E2E_RUN_DIR/remote-home"; cfg=remote.cfg; port="$OPENJK_E2E_REMOTE_PORT"
fi
mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$ROOT/tests/splitscreen/gameplay/network/local_host/cfg/$cfg" "$home/base/splitqa/"
cp "$build/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$build"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$build/codemp/game/jampgamex86_64.dylib" "$home/base/"
"$bin" +set fs_basepath "$base" +set fs_homepath "$home" +set vm_game 0 \
	+set vm_cgame 0 +set vm_ui 0 +set net_port "$port" +set r_fullscreen 0 \
	+set r_mode 3 +set s_initsound 0 +set in_joystick 1 +set com_introplayed 1 \
	+set developer 1 +set gp301_connect "connect 127.0.0.1:$OPENJK_E2E_CLIENT_PORT" \
	+exec "splitqa/$cfg"
if [[ "$role" == host ]]; then
	find "$home/base/screenshots" -type f -name '*.png' -exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \;
fi
