#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
BIN="$ROOT/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
RUN_HOME="${MENU_CAPTURE_HOME:-/private/tmp/openjk-r3-04-menu}"
ARTIFACTS="$ROOT/tests/splitscreen/rendering/certification/menu/artifacts"

mkdir -p "$RUN_HOME/base/splitqa" "$RUN_HOME/base/screenshots" "$ARTIFACTS"
"$ROOT/tests/splitscreen/install_assets.sh" "$RUN_HOME" >/dev/null
cp "$ROOT/build-x86_64/codemp/ui/uix86_64.dylib" "$RUN_HOME/base/"
cp "$ROOT/build-x86_64/codemp/game/jampgamex86_64.dylib" "$RUN_HOME/base/"
cp "$ROOT"/build-x86_64/codemp/cgame/cgame*x86_64.dylib "$RUN_HOME/base/"
cp "$ROOT/tests/splitscreen/rendering/certification/menu"/top_menu_?p.cfg "$RUN_HOME/base/splitqa/"

for players in 2 4; do
	log="$ARTIFACTS/top_menu_${players}p.stdout.txt"
	"$BIN" +set fs_basepath "$BASE" +set fs_homepath "$RUN_HOME" \
		+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
		+set net_port "$((29400 + players))" +set r_fullscreen 0 +set r_mode 3 \
		+set s_initsound 0 +set in_joystick 1 +set com_introplayed 1 \
		+exec "splitqa/top_menu_${players}p.cfg" >"$log" 2>&1
	grep -q "SplitUIAssert: PASS cvar=ui_splitScreenMenuMode expected=top actual=top" "$log"
done
cp "$RUN_HOME"/base/screenshots/cert_text_budget_top_menu_*.png "$ARTIFACTS/"
