#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
mode="${1:?mode}" players="${2:?players}"
case "$mode" in
	ffa) gametype=0; score_assert="splitnet_assert_stat 1 score ge 1" ;;
	holocron) gametype=1; score_assert="splitnet_assert_stat 1 score ge 1" ;;
	jedimaster) gametype=2; score_assert="splitnet_assert_stat 2 deaths ge 1" ;;
	*) exit 2 ;;
esac
case "$players" in 2|3|4) ;; *) exit 2 ;; esac
build="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
binary="${OPENJK_BIN:-$build/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
basepath="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
home="${OPENJK_E2E_HOMEPATH:?}"
mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$ROOT/tests/splitscreen/gameplay/modes/individual/cfg/lifecycle_${players}p.cfg" "$home/base/splitqa/"
cp "$build/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$build"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$build/codemp/game/jampgamex86_64.dylib" "$home/base/"
"$binary" +set fs_basepath "$basepath" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set in_joystick 1 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set com_introplayed 1 +set developer 1 \
	+set g_gametype "$gametype" +set gp201_score_assert "$score_assert" \
	+exec "splitqa/lifecycle_${players}p.cfg"
find "$home/base/screenshots" -type f -name '*.png' -exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \;
