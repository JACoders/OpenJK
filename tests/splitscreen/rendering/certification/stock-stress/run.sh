#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
QA_ROOT="${OPENJK_STRESS_ROOT:-/tmp/openjk-r2-03-stock-stress}"
PORT="${OPENJK_STRESS_PORT:-29331}"
CYCLES="${OPENJK_STRESS_CYCLES:-200}"
DWELL_FRAMES="${OPENJK_STRESS_DWELL_FRAMES:-2}"
MIN_SECONDS="${OPENJK_STRESS_MIN_SECONDS:-0}"
CAPTURE_INTERVAL="${OPENJK_STRESS_CAPTURE_INTERVAL:-0}"
FINAL_DWELL_FRAMES="${OPENJK_STRESS_FINAL_DWELL_FRAMES:-10}"
HOME_PATH="$QA_ROOT/home"
STOCK_HOME="$QA_ROOT/stock-home"
LOG_DIR="$QA_ROOT/logs"
CFG_DIR="$HOME_PATH/base/splitqa"
CFG="$CFG_DIR/r2_03_stress.cfg"
RSS="$LOG_DIR/rss-kb.tsv"
LOG="$LOG_DIR/game.log"

if [[ ! -x "$BIN" ]]; then
	echo "missing frozen binary: $BIN" >&2
	exit 2
fi
if ! [[ "$CYCLES" =~ ^[1-9][0-9]*$ && "$DWELL_FRAMES" =~ ^[1-9][0-9]*$ && "$MIN_SECONDS" =~ ^[0-9]+$ && "$CAPTURE_INTERVAL" =~ ^[0-9]+$ && "$FINAL_DWELL_FRAMES" =~ ^[1-9][0-9]*$ ]]; then
	echo "cycle, dwell, duration, and capture settings must be non-negative integers (cycles/dwell positive)" >&2
	exit 2
fi

mkdir -p "$CFG_DIR" "$LOG_DIR" "$HOME_PATH/base/screenshots" "$STOCK_HOME/base/splitqa" "$STOCK_HOME/base/screenshots"
rm -f "$HOME_PATH/base/screenshots/r2_03_stress_final.png" "$STOCK_HOME/base/screenshots/splitqa_one_player_mp.png"
cp "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" "$HOME_PATH/base/"
cp "$BUILD_DIR/codemp/cgame/"cgame*x86_64.dylib "$HOME_PATH/base/"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$HOME_PATH/base/"
cp "$ROOT/tests/splitscreen/cfg/one_player_mp_smoke.cfg" "$CFG_DIR/"
cp "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" "$STOCK_HOME/base/"
cp "$BUILD_DIR/codemp/cgame/"cgame*x86_64.dylib "$STOCK_HOME/base/"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$STOCK_HOME/base/"
cp "$ROOT/tests/splitscreen/cfg/one_player_mp_smoke.cfg" "$STOCK_HOME/base/splitqa/"

: >"$CFG"
printf '%s\n' \
	'set cl_splitScreen 1' \
	'set ui_splitScreenPlayerCount 4' \
	'set cl_splitScreenLocalCmds 0' \
	'set con_notifytime 0' \
	'devmap mp/ffa3' \
	'wait 360' \
	'closemenu' \
	'wait 30' \
	'cmd team free' \
	'wait 60' \
	'splitnet_connect 2 localhost' \
	'splitnet_connect 3 localhost' \
	'splitnet_connect 4 localhost' \
	'wait 360' \
	'splitnet_cmd 2 team free' \
	'splitnet_cmd 3 team free' \
	'splitnet_cmd 4 team free' \
	'wait 120' >>"$CFG"
for ((cycle = 1; cycle <= CYCLES; cycle++)); do
	player=$((cycle % 4 + 1))
	layout=$((cycle % 2))
	case $((cycle % 4)) in
		0) stock_menu=ingame_about ;;
		1) stock_menu=ingame_addbot ;;
		2) stock_menu=ingame_setup ;;
		3) stock_menu=ingame_vote ;;
	esac
	printf 'set cl_splitScreenLayout %d\nsplitscreen_topmenu %d\nwait %d\nset ui_splitScreenStockMenu %s\nset ui_splitScreenMenuMode stock\nwait %d\n' "$layout" "$player" "$DWELL_FRAMES" "$stock_menu" "$DWELL_FRAMES" >>"$CFG"
	if (( CAPTURE_INTERVAL > 0 && cycle % CAPTURE_INTERVAL == 0 )); then
		printf 'screenshot_png r3_03_soak_%04d\n' "$cycle" >>"$CFG"
	fi
	printf 'set ui_splitScreenMenuMode ""\nset ui_splitScreenStockMenu ""\nset ui_splitScreenInputTarget 0\nclosemenu\nwait 2\n' >>"$CFG"
done
printf '%s\n' \
	"wait $FINAL_DWELL_FRAMES" \
	'echo R2-03-STRESS-COMPLETE' \
	'splitui_status' \
	'screenshot_png r2_03_stress_final' \
	'wait 10' \
	'quit' >>"$CFG"

shasum -a 256 "$BIN" "$BUILD_DIR/codemp/ui/uix86_64.dylib" >"$LOG_DIR/frozen-sha256.txt"
"$BIN" +set fs_basepath "$BASEPATH" +set fs_homepath "$STOCK_HOME" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port "$((PORT + 1))" \
	+set r_fullscreen 0 +set s_initsound 0 +set com_introplayed 1 \
	+exec splitqa/one_player_mp_smoke.cfg >"$LOG_DIR/stock-one-player.log" 2>&1
rg -q "Wrote screenshots/splitqa_one_player_mp.png" "$LOG_DIR/stock-one-player.log"
test -s "$STOCK_HOME/base/screenshots/splitqa_one_player_mp.png"
if rg -q "SplitNet party:" "$LOG_DIR/stock-one-player.log"; then
	echo "R2-03: FAIL stock run started a split-screen party" >&2
	exit 1
fi
printf 'sample\trss_kb\n' >"$RSS"
"$BIN" +set fs_basepath "$BASEPATH" +set fs_homepath "$HOME_PATH" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port "$PORT" \
	+set r_fullscreen 0 +set s_initsound 0 +set com_introplayed 1 \
	+exec splitqa/r2_03_stress.cfg >"$LOG" 2>&1 &
game_pid=$!
start_epoch="$(date +%s)"
trap 'kill "$game_pid" 2>/dev/null || true' EXIT
sample=0
while kill -0 "$game_pid" 2>/dev/null; do
	rss_kb="$(ps -o rss= -p "$game_pid" | tr -d ' ')"
	if [[ -n "$rss_kb" ]]; then
		printf '%d\t%s\n' "$sample" "$rss_kb" >>"$RSS"
	fi
	sample=$((sample + 1))
	sleep 0.2
done
wait "$game_pid"
end_epoch="$(date +%s)"
trap - EXIT
elapsed_seconds=$((end_epoch - start_epoch))

if rg -i "Segmentation fault|Assertion.*failed|recursive error|ERROR:|Sys_Error|stack (overflow|underflow)" "$LOG"; then
	echo "R2-03: FAIL error marker in game log" >&2
	exit 1
fi
rg -q "R2-03-STRESS-COMPLETE" "$LOG"
rg -q "Wrote screenshots/r2_03_stress_final.png" "$LOG"
test -s "$HOME_PATH/base/screenshots/r2_03_stress_final.png"
if (( elapsed_seconds < MIN_SECONDS )); then
	echo "R2-03: FAIL elapsed_seconds=$elapsed_seconds minimum=$MIN_SECONDS" >&2
	exit 1
fi
python3 "$ROOT/tests/splitscreen/rendering/certification/stock-stress/summarize_rss.py" "$RSS" | tee "$LOG_DIR/rss-summary.txt"
printf 'R2-03: PASS cycles=%s dwell_frames=%s final_dwell_frames=%s elapsed_seconds=%s port=%s root=%s\n' "$CYCLES" "$DWELL_FRAMES" "$FINAL_DWELL_FRAMES" "$elapsed_seconds" "$PORT" "$QA_ROOT"
