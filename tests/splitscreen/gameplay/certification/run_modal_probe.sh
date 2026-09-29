#!/usr/bin/env bash
set -euo pipefail

players="${1:?players required}"
suite="${2:?suite directory required}"
expected="${3:?frozen sha required}"
run_id="${4:?run id required}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
results="$suite/modal"

case "$players" in 2|3) ;; *) exit 2 ;; esac
test "$(shasum -a 256 "$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64" |
	awk '{print $1}')" = "$expected"

OPENJK_MODAL_RESULTS="$results" \
	"$root/tests/splitscreen/gameplay/modal_ownership/run.sh" "$players"
manifest="$(<"$results/${players}p-manifest-path.txt")"
report="$results/${players}p-report.json"
"$root/tests/splitscreen/gameplay/run_e2e.sh" --validate-only "$manifest"
index="$suite/modal.tsv"
"$root/tests/splitscreen/gameplay/certification/write_aux_index.sh" \
	modal_ownership "$players" "$run_id" "$expected" "$manifest" "$report" "$index"
printf 'modal_ownership\t%s\t%s\n' "$index" \
	"$(shasum -a 256 "$index" | awk '{print $1}')" >>"$suite/aux.tsv"
