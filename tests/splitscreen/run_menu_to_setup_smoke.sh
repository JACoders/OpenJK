#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$(mktemp -d /private/tmp/openjk-menu-to-setup.XXXXXX)}"
CFG_DIR="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/menu-to-setup.stdout.log"
PERSISTENCE_LOG="$HOMEPATH/base/layout-persistence.stdout.log"

mkdir -p "$CFG_DIR" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/menu_to_setup_smoke.cfg" "$CFG_DIR/"
cp "$ROOT/tests/splitscreen/cfg/layout_persistence_smoke.cfg" "$CFG_DIR/"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$HOMEPATH/base/"

"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_ui 0 +set in_joystick 1 +set net_port 29272 \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 \
	+set com_introplayed 1 +set developer 1 \
	+exec splitqa/menu_to_setup_smoke.cfg >"$LOG" 2>&1

rg "SplitUIAssert: (PASS|FAIL)|SplitUIStatus:" "$LOG"
if rg -q "SplitUIAssert: FAIL" "$LOG"; then
	echo "Menu-to-setup smoke failed; log: $LOG" >&2
	exit 1
fi
test -s "$HOMEPATH/base/screenshots/split_menu_layout_vertical_selected.png"
test -s "$HOMEPATH/base/screenshots/split_menu_to_setup_smoke.png"

"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_ui 0 +set in_joystick 1 +set net_port 29273 \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 \
	+set com_introplayed 1 +set developer 1 \
	+exec splitqa/layout_persistence_smoke.cfg >"$PERSISTENCE_LOG" 2>&1

rg "SplitUIAssert: (PASS|FAIL)" "$PERSISTENCE_LOG"
if rg -q "SplitUIAssert: FAIL" "$PERSISTENCE_LOG"; then
	echo "Layout persistence smoke failed; log: $PERSISTENCE_LOG" >&2
	exit 1
fi

echo "Menu-to-setup smoke passed"
echo "Log: $LOG"
echo "Persistence log: $PERSISTENCE_LOG"
echo "Layout screenshot: $HOMEPATH/base/screenshots/split_menu_layout_vertical_selected.png"
echo "Screenshot: $HOMEPATH/base/screenshots/split_menu_to_setup_smoke.png"
