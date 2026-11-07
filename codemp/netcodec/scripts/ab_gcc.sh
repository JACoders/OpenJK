#!/bin/bash
# A/B (owner, Debian VM): bench_codec of a reference commit (REF, default HEAD) against the working tree, GCC Release,
# run alternately on core 1; prints the warm timings per run, then the best of each build and the difference.
# Usage, from anywhere: [REF=<commit>] [NOINL=1] [CXXFLAGS=...] bash codemp/netcodec/scripts/ab_gcc.sh [rounds]
# Functions, loops and jumps are aligned to 64 bytes in both builds: without it, where the code falls in memory moves
# the timings by 10 to 20 % under GCC (the unchanged legacy coder included). NOINL=1: the working-tree copy without
# the forced inlining of PredictAngle. CXXFLAGS: extra compiler flags for both builds.
# Checks: a netcodec mismatch fails the run; the unchanged legacy coder moving by more than 4 % between runs means the
# machine was busy and the numbers should not be used.
set -e
export LC_ALL=C   # numbers with a decimal point in awk (a French locale reads 1404.9 as 1404)
R="$(cd "$(dirname "$0")/../../.." && pwd)"
ROUNDS=${1:-6}
REF=${REF:-HEAD}
W=/tmp/netcodec-ab
G() { git -c safe.directory='*' -C "$R" "$@"; }
rm -rf "$W" && mkdir -p "$W/ref" "$W/cur/codemp"
G archive "$REF" codemp/netcodec | tar -x -C "$W/ref"
cp -r "$R/codemp/netcodec" "$W/cur/codemp/"
echo "ref = $REF ($(G log -1 --format='%h %s' "$REF" | cut -c1-100)); cur = the working tree"
if [ "${NOINL:-0}" = 1 ]; then
	F="$W/cur/codemp/netcodec/src/field_codec.h"
	sed -i 's/\[\[nodiscard\]\] NETCODEC_ALWAYS_INLINE std::optional<int64_t> PredictAngle(/[[nodiscard]] inline std::optional<int64_t> PredictAngle(/' "$F"
	grep -q "nodiscard\]\] inline std::optional<int64_t> PredictAngle" "$F" || { echo "NOINL: PredictAngle not found in $F"; exit 1; }
	echo "cur: PredictAngle not forced inline"
fi
build() {   # $1 = name (ref, cur)
	cmake -S "$W/$1/codemp/netcodec" -B "$W/build-$1" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ \
		-DCMAKE_CXX_FLAGS="-falign-functions=64 -falign-loops=64 -falign-jumps=64 ${CXXFLAGS:-}" \
		-Dnetcodec_OPENJK_DIR="$R" -Dnetcodec_TRACE_DIR="$R/docs_rewrite/netcode/data/traces" > "$W/build-$1.log" 2>&1 &&
	cmake --build "$W/build-$1" --target netcodec_bench_codec -j 4 >> "$W/build-$1.log" 2>&1 ||
	{ echo "build of $1 failed:"; tail -30 "$W/build-$1.log"; exit 1; }
}
echo "building the reference and the working tree ..."
build ref
build cur
echo "functions of the library not inlined (code symbols), only in ref (<) or only in cur (>):"
for v in ref cur; do
	nm -C --defined-only "$W/build-$v/libnetcodec.a" | awk '$2 ~ /^[TtWw]$/ {$1=""; $2=""; print}' | sed 's/^  //' | sort -u > "$W/syms-$v.txt"
done
diff "$W/syms-ref.txt" "$W/syms-cur.txt" | grep '^[<>]' | cut -c1-200 || echo "  (none)"
echo "sizes of the code sections: ref $(size -t "$W/build-ref/libnetcodec.a" | tail -1 | awk '{print $1}'), cur $(size -t "$W/build-cur/libnetcodec.a" | tail -1 | awk '{print $1}')"
T="$R/docs_rewrite/netcode/data/traces/codec_trace.bin"
PIN=""
command -v taskset > /dev/null && PIN="taskset -c 1"
for i in $(seq 1 "$ROUNDS"); do
	for v in ref cur; do
		printf '%s %d: ' $v $i
		$PIN "$W/build-$v/bench/netcodec_bench_codec" "$T" 10 \
			| awk '/mismatches/{split($0,a,"mismatches "); mm=a[2]+0} /encode netcodec/{e=$6} /decode netcodec/{d=$6} /encode legacy/{el=$6} /decode legacy/{dl=$6}
			       END{printf "warm encode %s (legacy %s) decode %s (legacy %s) mismatches %d\n", e, el, d, dl, mm}'
	done
done | tee "$W/runs.txt"
awk -f "$(dirname "$0")/ab_summary.awk" "$W/runs.txt"
