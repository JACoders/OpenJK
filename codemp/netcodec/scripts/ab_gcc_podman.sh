#!/bin/bash
# A/B (owner, Debian VM): bench_codec built with a recent GCC in a podman container, run on the VM outside it.
# Three builds: a reference commit (REF, default HEAD), the working tree without the forced inlining of PredictAngle
# (noinl), the working tree (cur); code aligned to 64 bytes in all of them, as ab_gcc.sh. Prints the warm timings per
# run, then the best of each build against ref, with the checks of ab_summary.awk.
# Usage, from anywhere: [REF=<commit>] bash codemp/netcodec/scripts/ab_gcc_podman.sh [rounds] [image]
# (default image: docker.io/library/gcc:16)
set -e
export LC_ALL=C   # numbers with a decimal point in awk (a French locale reads 1404.9 as 1404)
R="$(cd "$(dirname "$0")/../../.." && pwd)"
ROUNDS=${1:-6}
IMG=${2:-docker.io/library/gcc:16}
REF=${REF:-HEAD}
W=/tmp/netcodec-abp
G() { git -c safe.directory='*' -C "$R" "$@"; }
rm -rf "$W" && mkdir -p "$W/ref" "$W/cur" "$W/noinl"
G archive "$REF" codemp/netcodec | tar -x -C "$W/ref"
echo "ref = $REF ($(G log -1 --format='%h %s' "$REF" | cut -c1-100)); cur = the working tree"
mkdir -p "$W/cur/codemp" && cp -r "$R/codemp/netcodec" "$W/cur/codemp/"
cp -r "$W/cur/codemp" "$W/noinl/"
F="$W/noinl/codemp/netcodec/src/field_codec.h"
sed -i 's/\[\[nodiscard\]\] NETCODEC_ALWAYS_INLINE std::optional<int64_t> PredictAngle(/[[nodiscard]] inline std::optional<int64_t> PredictAngle(/' "$F"
grep -q "nodiscard\]\] inline std::optional<int64_t> PredictAngle" "$F" || { echo "noinl: PredictAngle not found in $F"; exit 1; }
# the OpenJK sources of the legacy benchmark, copied: rootless podman cannot read the VirtualBox shared folder
mkdir -p "$W/oj/lib/gsl-lite" && cp -r "$R/codemp" "$R/shared" "$W/oj/" && cp -r "$R/lib/gsl-lite/include" "$W/oj/lib/gsl-lite/"
echo "building in $IMG ..."
podman run --rm -v "$W:/w" "$IMG" bash -c '
	set -e
	(apt-get update -qq && apt-get install -y -qq cmake) > /dev/null 2>&1
	g++ --version | head -1
	for v in ref noinl cur; do
		cmake -S /w/$v/codemp/netcodec -B /w/build-$v -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++ \
			-DCMAKE_CXX_FLAGS="-falign-functions=64 -falign-loops=64 -falign-jumps=64" \
			-DCMAKE_EXE_LINKER_FLAGS=-static \
			-Dnetcodec_OPENJK_DIR=/w/oj -Dnetcodec_TRACE_DIR=/w/no-traces > /w/build-$v.log 2>&1 &&
		cmake --build /w/build-$v --target netcodec_bench_codec -j 4 >> /w/build-$v.log 2>&1 ||
		{ echo "build of $v failed:"; tail -30 /w/build-$v.log; exit 1; }
	done'
echo "functions of the library not inlined, compared with ref (< only in ref, > only in the other build):"
for v in ref noinl cur; do
	nm -C --defined-only "$W/build-$v/libnetcodec.a" | awk '$2 ~ /^[TtWw]$/ {$1=""; $2=""; print}' | sed 's/^  //' | sort -u > "$W/syms-$v.txt"
done
for v in noinl cur; do
	echo "  $v:"
	diff "$W/syms-ref.txt" "$W/syms-$v.txt" | grep '^[<>]' | cut -c1-160 | sed 's/^/    /' || echo "    (none)"
done
T="$R/docs_rewrite/netcode/data/traces/codec_trace.bin"
PIN=""
command -v taskset > /dev/null && PIN="taskset -c 1"
for i in $(seq 1 "$ROUNDS"); do
	for v in ref noinl cur; do
		printf '%s %d: ' $v $i
		$PIN "$W/build-$v/bench/netcodec_bench_codec" "$T" 10 \
			| awk '/mismatches/{split($0,a,"mismatches "); mm=a[2]+0} /encode netcodec/{e=$6} /decode netcodec/{d=$6} /encode legacy/{el=$6} /decode legacy/{dl=$6}
			       END{printf "warm encode %s (legacy %s) decode %s (legacy %s) mismatches %d\n", e, el, d, dl, mm}'
	done
done | tee "$W/runs.txt"
awk -f "$(dirname "$0")/ab_summary.awk" "$W/runs.txt"
