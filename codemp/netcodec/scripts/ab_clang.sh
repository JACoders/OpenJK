#!/bin/bash
# A/B (this Windows machine, Git Bash, llvm-mingw clang): bench_codec of a reference (REF=<commit>, default HEAD, or
# REF_DIR=<a copy holding codemp/netcodec>) against the working tree, Release, run alternately on logical CPU 20 (the
# second CCD; the first, with the 3D V-Cache, is the owner's) with high priority. Builds in build-netcodec/ab-clang.
# Prints the warm and cold timings per run, then the best of each build against ref with the checks of
# ab_summary.awk. Only meaningful on a quiet machine (no game, the VMs off): ask the owner first.
# Usage, from anywhere: [REF=<commit> | REF_DIR=<dir>] bash codemp/netcodec/scripts/ab_clang.sh [rounds] [--no-build]
export LC_ALL=C   # numbers with a decimal point in awk (a French locale reads 1404.9 as 1404)
HERE="$(cd "$(dirname "$0")" && pwd)"
R="$(cd "$HERE/../../.." && pwd -W 2>/dev/null || pwd)"
S="$R/build-netcodec/ab-clang"
mkdir -p "$S"
export PATH="/c/Logiciels/llvm-mingw-20261104-ucrt-x86_64/bin:/c/Logiciels/cmake-4.4.5/bin:/c/Logiciels/ninja:$PATH"
ROUNDS=${1:-6}
REF=${REF:-HEAD}
build() {   # $1 = name (ref, cur)
	(cd "$S/$1/codemp/netcodec" &&
	 cmake --preset mingw-clang-release -Dnetcodec_OPENJK_DIR="$R" -Dnetcodec_TRACE_DIR="$R/docs_rewrite/netcode/data/traces" > "$S/build-$1.log" 2>&1 &&
	 cmake --build --preset mingw-clang-release --target netcodec_bench_codec >> "$S/build-$1.log" 2>&1) ||
	{ echo "build of $1 failed:"; tail -30 "$S/build-$1.log"; exit 1; }
}
if [[ " $* " != *" --no-build "* ]]; then
	rm -rf "$S/ref" "$S/cur"
	mkdir -p "$S/ref/codemp" "$S/cur/codemp"
	if [ -n "$REF_DIR" ]; then
		cp -r "$REF_DIR/codemp/netcodec" "$S/ref/codemp/"
		echo "ref = the copy in $REF_DIR" > "$S/ref.txt"
	else
		(cd "$R" && git archive "$REF" codemp/netcodec | tar -x -C "$S/ref") || exit 1
		echo "ref = $REF ($(cd "$R" && git log -1 --format='%h %s' "$REF" | cut -c1-100))" > "$S/ref.txt"
	fi
	cp -r "$R/codemp/netcodec" "$S/cur/codemp/"
	rm -rf "$S/cur/build-netcodec"
	build ref
	build cur
fi
echo "$(cat "$S/ref.txt"); cur = the working tree$([[ " $* " == *" --no-build "* ]] && echo ' as last built')"
TR=$(cygpath -w "$R/docs_rewrite/netcode/data/traces/codec_trace.bin")
for i in $(seq 1 "$ROUNDS"); do
	for v in ref cur; do
		exe=$(cygpath -w "$S/$v/build-netcodec/mingw-clang-release/bench/netcodec_bench_codec.exe")
		printf '%s %d: ' $v $i
		cmd //c start "" //b //wait //affinity 0x100000 //high "$exe" "$TR" 10 \
			| awk '/mismatches/{split($0,a,"mismatches "); mm=a[2]+0}
			       /encode netcodec/{en=$3; we=$6} /encode legacy/{wel=$6} /decode netcodec/{dn=$3; wd=$6} /decode legacy/{wdl=$6}
			       END{printf "warm encode %.1f (legacy %.1f) decode %.1f (legacy %.1f) mismatches %d | cold encode %.0f decode %.0f\n", we, wel, wd, wdl, mm, en, dn}'
	done
done | tee "$S/runs.txt"
awk -f "$HERE/ab_summary.awk" "$S/runs.txt"
