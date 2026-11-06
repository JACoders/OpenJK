#!/usr/bin/env bash
# Validation of netcodec on Linux (GCC and Clang): configure, build and test every Linux preset, then the size
# benchmark (Release and ASan + UBSan), the codec benchmark (two runs) and the x86 trainer, which must rewrite the
# committed tables unchanged. Needs the traces of docs_rewrite/netcode/data/traces (export_rans_trace.py,
# export_codec_trace.py, export_demo_traces.py).
# Run from anywhere: codemp/netcodec/scripts/validate.sh
# Output: the console and build-netcodec/validate-linux.log; the summary and the benchmark figures come last.
set -u
cd "$(dirname "$0")/.."
B=../../build-netcodec
T=../../docs_rewrite/netcode/data/traces
mkdir -p "$B"
LOG="$B/validate-linux.log"
exec > >(tee "$LOG") 2>&1

JOBS=$(nproc 2>/dev/null || echo 2)
summary=()
failed=0
step() {
	local name=$1
	shift
	echo
	echo "=== $name"
	if "$@"; then summary+=("ok    $name"); else summary+=("FAIL  $name"); failed=1; fi
}

for p in gcc-release gcc-debug gcc-asan gcc-x86 clang-release; do
	step "$p: configure" cmake --preset "$p"
	step "$p: build" cmake --build --preset "$p" -j "$JOBS"
	step "$p: tests" ctest --preset "$p"
done

OUT=$(mktemp -d)
step "size benchmark, gcc-release" bash -c "'$B/gcc-release/bench/netcodec_bench_size' '$T/holdout.bin' > '$OUT/size'"
step "size benchmark, gcc-asan" bash -c "'$B/gcc-asan/bench/netcodec_bench_size' '$T/holdout.bin' > '$OUT/size-asan'"
step "codec benchmark, gcc-release, run 1" bash -c "'$B/gcc-release/bench/netcodec_bench_codec' '$T/codec_trace.bin' 10 > '$OUT/codec1'"
step "codec benchmark, gcc-release, run 2" bash -c "'$B/gcc-release/bench/netcodec_bench_codec' '$T/codec_trace.bin' 10 > '$OUT/codec2'"
step "trainer gcc-x86 writes the committed tables" bash -c \
	"'$B/gcc-x86/tools/netcodec_train' '$T/train.bin' '$OUT/tables.inc' && cmp '$OUT/tables.inc' src/generated/jka_tables.inc"

echo
echo "=== results"
for f in size size-asan; do
	echo "--- $f"
	sed -n '1,2p;5,6p' "$OUT/$f" 2>/dev/null
done
for f in codec1 codec2; do
	echo "--- $f"
	cat "$OUT/$f" 2>/dev/null
done
echo
echo "=== summary"
printf '%s\n' "${summary[@]}"
if [ "$failed" -eq 0 ]; then echo "ALL OK"; else echo "SOME STEPS FAILED"; fi
echo "log: $LOG"
rm -rf "$OUT"
exit "$failed"
