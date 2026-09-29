#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$(mktemp -d /private/tmp/openjk-routed-selection-3p.XXXXXX)}"
CFG_DIR="$HOMEPATH/base/splitqa"
LOG="$HOMEPATH/base/routed-selection-3p.stdout.log"

mkdir -p "$CFG_DIR" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$ROOT/tests/splitscreen/cfg/routed_selection_3p_acceptance.cfg" "$CFG_DIR/"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$HOMEPATH/base/"
cp "$BUILD_DIR"/codemp/cgame/cgame*x86_64.dylib "$HOMEPATH/base/"
cp "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" "$HOMEPATH/base/"

if rg -q 'splitscreen_setup|splitnet_applyprofile|set (ui_splitScreenP[123]Model|ui_splitScreenP[123]ForcePowers|model|forcepowers)' \
	"$ROOT/tests/splitscreen/cfg/routed_selection_3p_acceptance.cfg"; then
	echo "Three-player acceptance must select models and Force profiles through routed UI input" >&2
	exit 2
fi

"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$HOMEPATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set in_joystick 1 +set net_port 29272 \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 \
	+set com_introplayed 1 +set developer 1 \
	+exec splitqa/routed_selection_3p_acceptance.cfg >"$LOG" 2>&1

rg "Split(UI|Input|NetLifecycle|NetStat|Profile)Assert: (PASS|FAIL)|SplitProfile P[123]:" "$LOG"
if rg -q "Split(UI|Input|NetLifecycle|NetStat|Profile)Assert: FAIL" "$LOG"; then
	echo "Three-player routed selection acceptance failed; log: $LOG" >&2
	exit 1
fi
rg -q "SplitInputSim: button device=controller1 player=2 button=0 pressed=1" "$LOG"
rg -q "SplitInputSim: button device=controller2 player=3 button=0 pressed=1" "$LOG"
rg -q "SplitInputSim: key device=keyboard player=1" "$LOG"
rg -q "SplitNetLifecycleAssert: PASS player=1 expected=INTERMISSION actual=INTERMISSION" "$LOG"
rg -q "SplitNetLifecycleAssert: PASS player=2 expected=INTERMISSION actual=INTERMISSION" "$LOG"
rg -q "SplitNetLifecycleAssert: PASS player=3 expected=INTERMISSION actual=INTERMISSION" "$LOG"
for frame in \
	routed3_chain_01_main_menu.png \
	routed3_chain_02_multiplayer.png \
	routed3_chain_03_split_screen.png \
	routed3_chain_04_three_players.png \
	routed3_chain_05_player_setup.png \
	routed3_selection_p1_kyle_dark.png \
	routed3_selection_p2_desann.png \
	routed3_selection_p3_reborn.png \
	routed3_selection_profiles_final.png \
	routed3_gameplay_all_alive.png \
	routed3_gameplay_kyle_lightning_p2.png \
	routed3_gameplay_kyle_lightning_p3.png \
	routed3_match_complete.png; do
	test -s "$HOMEPATH/base/screenshots/$frame"
done

echo "Three-player routed selection acceptance passed"
echo "Log: $LOG"
echo "Screenshots: $HOMEPATH/base/screenshots"
