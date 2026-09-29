#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
build="${OPENJK_BUILD_DIR:-$root/build-x86_64}"
bin="$build/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
base="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
home="${OPENJK_E2E_HOMEPATH:?}/client"
players="${OPENJK_E2E_PLAYERS:?}"

mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$root/tests/splitscreen/install_assets.sh" "$home" >/dev/null
python3 "$root/tests/splitscreen/gameplay/certification/generate_virtual_controller_cfg.py" \
	"$players" >"$home/base/splitqa/controller_restart.cfg"
cp "$build/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$build/codemp/game/jampgamex86_64.dylib" "$home/base/"
cp "$build"/codemp/cgame/cgame*x86_64.dylib "$home/base/"

OPENJK_VIRTUAL_GAMEPADS="$((players - 1))" \
OPENJK_VIRTUAL_GAMEPAD_PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:?}" \
"$bin" +set fs_basepath "$base" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+set com_introplayed 1 +exec splitqa/controller_restart.cfg
find "$home/base/screenshots" -type f -name 'cert_virtual_controller_restart.png' \
	-exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \;
