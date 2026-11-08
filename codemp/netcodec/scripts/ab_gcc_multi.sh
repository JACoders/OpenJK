#!/bin/bash
# Several builds in one run (owner, Debian VM), to find which of a series of commits changed the timing: bench_codec of
# each copy (a directory holding codemp/netcodec, for example build-netcodec/bisect/* prepared on the host), GCC
# Release with functions, loops and jumps aligned to 64 bytes in every build (as ab_gcc.sh), run in turn on core 1.
# The first copy is the reference; the summary (ab_summary.awk) compares every build with it, and the lines "step"
# compare each build with the one before it.
# Usage, from anywhere: [CXX=clang++ CC=clang] [M32=1] bash codemp/netcodec/scripts/ab_gcc_multi.sh <rounds> <copy> ...
# CXX / CC: another compiler (default g++ / gcc); with clang, functions and loops only (clang has no -falign-jumps).
# M32=1: 32-bit x86 builds (-m32, as the gcc-x86 preset).
set -e
export LC_ALL=C   # numbers with a decimal point in awk (a French locale reads 1404.9 as 1404)
R="$(cd "$(dirname "$0")/../../.." && pwd)"
ROUNDS=$1
shift
W=/tmp/netcodec-ab-multi
rm -rf "$W" && mkdir -p "$W"
CXX=${CXX:-g++}
CC=${CC:-gcc}
ALIGN="-falign-functions=64 -falign-loops=64 -falign-jumps=64"
case "$CXX" in *clang*) ALIGN="-falign-functions=64 -falign-loops=64" ;; esac
M32FLAG=""
[ "${M32:-0}" = 1 ] && M32FLAG="-m32"
NAMES=()
for d in "$@"; do
	n=$(basename "$d")
	[ ${#NAMES[@]} -eq 0 ] && n=ref
	NAMES+=("$n")
	mkdir -p "$W/$n/codemp"
	cp -r "$d/codemp/netcodec" "$W/$n/codemp/"
	rm -rf "$W/$n/codemp/netcodec/build-netcodec"
	echo "$n = $d"
	cmake -S "$W/$n/codemp/netcodec" -B "$W/build-$n" -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER="$CXX" -DCMAKE_C_COMPILER="$CC" \
		-DCMAKE_CXX_FLAGS="$ALIGN $M32FLAG" -DCMAKE_C_FLAGS="$M32FLAG" -Dnetcodec_OPENJK_DIR="$R" -Dnetcodec_TRACE_DIR="$R/docs_rewrite/netcode/data/traces" \
		> "$W/build-$n.log" 2>&1 &&
	cmake --build "$W/build-$n" --target netcodec_bench_codec -j 4 >> "$W/build-$n.log" 2>&1 ||
	{ echo "build of $n failed:"; tail -30 "$W/build-$n.log"; exit 1; }
done
T="$R/docs_rewrite/netcode/data/traces/codec_trace.bin"
PIN=""
command -v taskset > /dev/null && PIN="taskset -c 1"
for i in $(seq 1 "$ROUNDS"); do
	for n in "${NAMES[@]}"; do
		printf '%s %d: ' "$n" "$i"
		$PIN "$W/build-$n/bench/netcodec_bench_codec" "$T" 10 \
			| awk '/mismatches/{split($0,a,"mismatches "); mm=a[2]+0}
			       /encode netcodec/{en=$3; e=$6} /decode netcodec/{dn=$3; d=$6} /encode legacy/{el=$6} /decode legacy/{dl=$6}
			       END{printf "warm encode %.1f (legacy %.1f) decode %.1f (legacy %.1f) mismatches %d | cold encode %.0f decode %.0f\n", e, el, d, dl, mm, en, dn}'
	done
done | tee "$W/runs.txt"
awk -f "$(dirname "$0")/ab_summary.awk" "$W/runs.txt" | tee "$W/summary.txt"
# each build against the previous one (best warm times)
awk '/^best /{n++; b[n]=$2; e[n]=$5; d[n]=$7}
     END{for (k = 2; k <= n; k++) printf "step %-14s -> %-14s warm encode %+.1f %%, decode %+.1f %%\n", b[k-1], b[k], 100 * (e[k] / e[k-1] - 1), 100 * (d[k] / d[k-1] - 1)}' "$W/summary.txt"
