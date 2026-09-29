#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/certification/4p"
BIN="$ROOT/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
EXPECTED="${GP5_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
python3 "$ROOT/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" \
	--expected-client-hash "$EXPECTED"
actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$EXPECTED" ]] || { echo "hash gate failed: $actual" >&2; exit 2; }
case_name="${1:?siege|lifecycle|slot-churn|fifth|modal|public|all}"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/certification/4p"
RUN="${GP5_RUN_ROOT:-$RESULTS/runs/$(date -u +%Y%m%dT%H%M%SZ)-remaining}"
mkdir -p "$RUN"

run_delegated() {
	local name="$1" runner="$2"
	local home="$RUN/$name-home" log="$RUN/$name-runner.log"
	local compat="$RUN/x86-compat"
	mkdir -p "$compat/codemp/ui" "$compat/codemp/cgame" "$compat/codemp/game" "$compat/codemp/rd-vanilla"
	cp "$ROOT/build-x86_64/codemp/ui/uix86_64.dylib" "$compat/codemp/ui/"
	cp "$ROOT/build-x86_64/codemp/game/jampgamex86_64.dylib" "$compat/codemp/game/"
	cp "$ROOT"/build-x86_64/codemp/cgame/cgame*x86_64.dylib "$compat/codemp/cgame/"
	cp "$ROOT/build-x86_64/codemp/rd-vanilla/rd-vanilla_x86_64.dylib" "$compat/codemp/rd-vanilla/"
	OPENJK_BUILD_DIR="$compat" OPENJK_BIN="$BIN" \
		OPENJK_MODULE_ARCH=x86_64 OPENJK_HOMEPATH="$home" \
		"$runner" >"$log" 2>&1
	"$HERE/strict_scan.sh" "$log" "$home"/base/qa-logs/*.txt
}

run_owned() {
	local name="$1" runner="$2"
	local log="$RUN/$name-runner.log"
	GP5_EXPECTED_SHA256="$EXPECTED" GP5_CASE_ROOT="$RUN/$name-runs" \
		OPENJK_BUILD_DIR="$ROOT/build-x86_64" OPENJK_BIN="$BIN" \
		"$runner" >"$log" 2>&1
	"$HERE/strict_scan.sh" "$log"
}

run_siege() { run_delegated siege "$ROOT/tests/splitscreen/phase5/four-player/run_siege.sh"; }
run_lifecycle() { run_delegated lifecycle "$ROOT/tests/splitscreen/run_external_lifecycle_qa.sh"; }
run_slot_churn() { run_owned slot-churn "$HERE/run_slot_churn.sh"; }
run_modal() { run_owned modal "$HERE/run_modal.sh"; }
run_fifth() { run_owned fifth "$HERE/run_controlled_fifth.sh"; }
run_public() {
	[[ "${GP5_PUBLIC_TARGET:-}" == 135.125.145.49:29070 ]] || { echo "public target gate failed" >&2; exit 2; }
	[[ -x "${GP5_PUBLIC_RUNNER:-}" ]] || { echo "set GP5_PUBLIC_RUNNER to routed public 4p runner" >&2; exit 2; }
	run_delegated public "$GP5_PUBLIC_RUNNER"
}

case "$case_name" in
	siege) run_siege ;;
	lifecycle) run_lifecycle ;;
	slot-churn) run_slot_churn ;;
	fifth) run_fifth ;;
	modal) run_modal ;;
	public) run_public ;;
	all) run_siege; run_lifecycle; run_slot_churn; run_fifth; run_modal ;;
	*) echo "unknown case: $case_name" >&2; exit 2 ;;
esac
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]] || exit 3
