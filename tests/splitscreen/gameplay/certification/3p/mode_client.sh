#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
mode="${1:?mode required}"
case "$mode" in
	ffa) gametype=0 ;;
	holocron) gametype=1 ;;
	jedimaster) gametype=2 ;;
	*) exit 2 ;;
esac
build="$root/build-x86_64"
binary="$build/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
basepath="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
home="${OPENJK_E2E_HOMEPATH:?}"
cfg="$home/base/splitqa/lifecycle_3p.cfg"

mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$root/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$root/tests/splitscreen/gameplay/modes/individual/cfg/lifecycle_3p.cfg" "$cfg"
# The shared discovery fixture predates enforcement of the stock five-second
# team-switch cooldown. Keep this owned certification copy policy-compliant.
perl -0pi -e \
	's/wait 180\nsplitnet_assert_lifecycle 3 SPECTATOR/wait 1000\nsplitnet_assert_lifecycle 3 SPECTATOR/' \
	"$cfg"
cp "$build/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$build"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$build/codemp/game/jampgamex86_64.dylib" "$home/base/"
"$binary" +set fs_basepath "$basepath" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set in_joystick 1 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set com_introplayed 1 +set developer 1 \
	+set g_gametype "$gametype" +exec splitqa/lifecycle_3p.cfg
find "$home/base/screenshots" -type f -name '*.png' -exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \;
