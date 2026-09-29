#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
PAYLOAD="${1:?usage: run_release_gate.sh /path/to/OpenJK-SplitScreen-arm64 /path/to/build-dir}"
BUILD_DIR="${2:?usage: run_release_gate.sh /path/to/OpenJK-SplitScreen-arm64 /path/to/build-dir}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
QA_ROOT="${OPENJK_RELEASE_QA_ROOT:-/tmp/openjk_release_gate}"
REPORT="$QA_ROOT/release-gate.log"
MP_BIN="$PAYLOAD/openjk.arm64.app/Contents/MacOS/openjk.arm64"
SP_BIN="$PAYLOAD/openjk_sp.arm64.app/Contents/MacOS/openjk_sp.arm64"

for path in "$MP_BIN" "$SP_BIN" "$BUILD_DIR/codemp/ui/uiarm64.dylib"; do
	[[ -s $path ]] || { echo "Missing release-gate input: $path" >&2; exit 2; }
done
[[ -f "$BASEPATH/base/assets0.pk3" ]] || { echo "Missing Jedi Academy data: $BASEPATH" >&2; exit 2; }

mkdir -p "$QA_ROOT"
: >"$REPORT"

run_gate() {
	local name=$1
	shift
	echo "== $name ==" | tee -a "$REPORT"
	"$@" 2>&1 | tee -a "$REPORT"
}

same_file() {
	cmp -s "$1" "$2" || {
		echo "Packaged file does not match build product: $1" >&2
		exit 1
	}
}

run_gate "package audit" "$ROOT/scripts/release/audit_macos_arm64.sh" "$PAYLOAD"
same_file "$PAYLOAD/base/uiarm64.dylib" "$BUILD_DIR/codemp/ui/uiarm64.dylib"
same_file "$PAYLOAD/base/jampgamearm64.dylib" "$BUILD_DIR/codemp/game/jampgamearm64.dylib"
for player in 1 2 3 4; do
	suffix=""
	if (( player > 1 )); then
		suffix="$player"
	fi
	same_file "$PAYLOAD/base/cgame${suffix}arm64.dylib" "$BUILD_DIR/codemp/cgame/cgame${suffix}arm64.dylib"
done

export OPENJK_BIN="$MP_BIN"
export OPENJK_SP_BIN="$SP_BIN"
export OPENJK_BUILD_DIR="$BUILD_DIR"
export OPENJK_BASEPATH="$BASEPATH"

run_gate "default split-screen matrix" env OPENJK_HOMEPATH="$QA_ROOT/default" "$ROOT/tests/splitscreen/run_splitscreen_qa.sh"

external_suites=(gamepad menu character combat controls lifecycle team_profile topmenu)
port=29420
for suite in "${external_suites[@]}"; do
	run_gate "external $suite" env \
		OPENJK_HOMEPATH="$QA_ROOT/external-$suite" \
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" \
		"$ROOT/tests/splitscreen/run_external_${suite}_qa.sh"
	port=$((port + 1))
done

phase5_suites=(
	"two-player/run.sh"
	"three-player/run.sh"
	"three-player/run_powerduel.sh"
	"four-player/run.sh"
	"four-player/run_modes.sh"
	"four-player/run_siege.sh"
)
index=0
for suite in "${phase5_suites[@]}"; do
	slug=${suite//\//-}
	run_gate "phase5 $slug" env \
		OPENJK_HOMEPATH="$QA_ROOT/phase5-$slug" \
		OPENJK_VIRTUAL_GAMEPAD_PORT="$((29440 + index))" \
		"$ROOT/tests/splitscreen/phase5/$suite"
	index=$((index + 1))
done

run_gate "stock regression" env OPENJK_STOCK_QA_ROOT="$QA_ROOT/stock" "$ROOT/tests/splitscreen/run_stock_regression_qa.sh"
run_gate "vanilla network compatibility" env OPENJK_VANILLA_QA_ROOT="$QA_ROOT/vanilla" "$ROOT/tests/splitscreen/run_vanilla_network_qa.sh"
run_gate "performance" env OPENJK_PERF_ROOT="$QA_ROOT/performance" "$ROOT/tests/splitscreen/run_performance_qa.sh"

{
	echo "== candidate identity =="
	echo "revision=$(git -C "$ROOT" rev-parse HEAD)"
	shasum -a 256 "$MP_BIN" "$SP_BIN"
	archive="$PAYLOAD/../$(basename "$PAYLOAD").zip"
	[[ ! -f $archive ]] || shasum -a 256 "$archive"
	echo "screenshots=$(find "$QA_ROOT" -type f -path '*/screenshots/*.png' | wc -l | tr -d ' ')"
	echo "release gate passed"
} | tee -a "$REPORT"

echo "release-gate report: $REPORT"
