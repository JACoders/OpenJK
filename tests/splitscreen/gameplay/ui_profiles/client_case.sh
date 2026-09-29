#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
players="${1:?player count required}"
case "$players" in 2|3|4) ;; *) exit 2 ;; esac
build="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
binary="${OPENJK_BIN:-$build/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
basepath="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
home="${OPENJK_E2E_HOMEPATH:?Phase 0 runner must provide OPENJK_E2E_HOMEPATH}"
cfgdir="$home/base/splitqa"
mkdir -p "$cfgdir" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$ROOT/tests/splitscreen/gameplay/ui_profiles/cfg/profile_${players}p.cfg" "$cfgdir/"
cp "$ROOT/tests/splitscreen/gameplay/ui_profiles/verify_${players}p.cfg" "$cfgdir/"
cp "$build/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$build"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$build/codemp/game/jampgamex86_64.dylib" "$home/base/"

common=(+set fs_basepath "$basepath" +set fs_homepath "$home"
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set in_joystick 1
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0
	+set com_introplayed 1 +set developer 1)

# First process starts from the runner-created empty home and performs all
# customization through routed UI events.
"$binary" "${common[@]}" +set net_port "$OPENJK_E2E_CLIENT_PORT" \
	+exec "splitqa/profile_${players}p.cfg"

# The second process intentionally reuses the exact same home and verifies
# persistence before exercising renderer/input restart and a map change.
"$binary" "${common[@]}" +set net_port "$OPENJK_E2E_CLIENT_PORT" \
	+exec "splitqa/verify_${players}p.cfg"

# Promote game-owned captures into the Phase 0 artifact directory so the
# runner hashes them into its immutable manifest.
find "$home/base/screenshots" -type f -name '*.png' -exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \;
