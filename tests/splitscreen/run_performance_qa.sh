#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
QA_ROOT="${OPENJK_PERF_ROOT:-/tmp/openjk_splitscreen_performance}"
REPORT="$QA_ROOT/performance.json"

mkdir -p "$QA_ROOT"
logs=()
for players in 1 2 3 4; do
	home="$QA_ROOT/${players}p"
	mkdir -p "$home/base/splitqa" "$home/base/screenshots" "$home/base/qa-logs"
	"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
	cp "$ROOT/tests/splitscreen/cfg/performance_${players}p.cfg" "$home/base/splitqa/"
	cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$home/base/"
	cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$home/base/"
	cp "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib "$home/base/"
	log="$home/base/qa-logs/performance_${players}p.stdout.txt"
	/usr/bin/time -lp "$BIN" \
		+set fs_basepath "$BASEPATH" +set fs_homepath "$home" \
		+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
		+set net_port "$((29300 + players))" +set r_fullscreen 0 +set r_mode 4 \
		+set s_initsound 0 +exec "splitqa/performance_${players}p.cfg" >"$log" 2>&1
	logs+=("$players=$log")
	if rg -q "SplitNetLifecycleAssert: FAIL|ERROR:|Sys_Error|Segmentation fault" "$log"; then
		echo "performance ${players}p run emitted an error" >&2
		exit 1
	fi
	grep -q "Wrote screenshots/splitqa_performance_${players}p.png" "$log"
	if (( players > 1 )); then
		python3 "$ROOT/tests/splitscreen/assert_screenshot.py" \
			"$home/base/screenshots/splitqa_performance_${players}p.png" "$players"
	fi
done

python3 "$ROOT/tests/splitscreen/analyze_performance.py" --output "$REPORT" "${logs[@]}"
echo "split-screen performance QA passed"
echo "report: $REPORT"
