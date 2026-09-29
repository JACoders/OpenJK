#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-/tmp/openjk_phase5_modes}"
LOG="$HOMEPATH/base/qa-logs/modes.stdout.txt"

mkdir -p "$HOMEPATH/base/splitqa" "$(dirname "$LOG")" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/phase5/four-player/modes.cfg" "$HOMEPATH/base/splitqa/phase5_modes.cfg"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$HOMEPATH/base/"
for module in "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib; do cp "$module" "$HOMEPATH/base/"; done
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$HOMEPATH/base/"
cp "$BUILD_DIR/codemp/rd-vanilla/rd-vanilla_arm64.dylib" "$(dirname "$BIN")/"

"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29195 \
	+set r_fullscreen 0 +set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/phase5_modes.cfg >"$LOG" 2>&1

if rg -q "Split(NetLifecycle|NetStat)Assert: FAIL|SplitUIAssert: FAIL|ERROR:|Sys_Error" "$LOG"; then
	rg "Split(NetLifecycle|NetStat)Assert|SplitUIAssert|SplitNet party:|ERROR:|Sys_Error" "$LOG"
	exit 1
fi
[[ $(rg -c "SplitNetLifecycleAssert: PASS" "$LOG") -eq 24 ]]
[[ $(rg -c "SplitNetStatAssert: PASS" "$LOG") -eq 12 ]]
[[ $(rg -c "SplitUIAssert: PASS" "$LOG") -eq 12 ]]
for shot in ffa holocron jedimaster team ctf cty; do
	test -s "$HOMEPATH/base/screenshots/phase5_mode_${shot}.png"
done
test -s "$HOMEPATH/base/screenshots/phase5_mode_team_setup.png"
echo "four-player mode matrix QA passed"
echo "screenshots: $HOMEPATH/base/screenshots"
echo "log: $LOG"
