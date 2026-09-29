#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
binary="$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
results="$root/tests/splitscreen/gameplay/results/certification/2p/modes"
client="$root/tests/splitscreen/gameplay/certification/2p/mode_client.sh"
run_id="${OPENJK_CERT_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM}"
ledger="$root/tests/splitscreen/gameplay/results/certification/2p/local-suite/$run_id/modes.tsv"

test "$(shasum -a 256 "$binary" | awk '{print $1}')" = "$expected"
[[ "$run_id" =~ ^[A-Za-z0-9._-]+$ ]] || exit 2
mkdir -p "$results" "$(dirname "$ledger")"
touch "$ledger"
modes=("$@")
if (( ${#modes[@]} == 0 )); then
	modes=(ffa holocron jedimaster)
fi
for mode in "${modes[@]}"; do
	output="$("$root/tests/splitscreen/gameplay/run_e2e.sh" \
		--case "${mode}-2p" --players 2 --vm native --timeout 120 \
		--artifact-root "$results" --hash "$binary" \
		--client-command "/bin/bash '$client' '$mode'")"
	manifest="$(printf '%s\n' "$output" | tail -1)"
	test -s "$manifest"
	ledger_tmp="$ledger.tmp.$$"
	awk -F '\t' -v target="${mode}-2p" '$1 != target' "$ledger" >"$ledger_tmp"
	mv "$ledger_tmp" "$ledger"
	printf '%s\t%s\t%s\n' "${mode}-2p" "$manifest" \
		"$(shasum -a 256 "$manifest" | awk '{print $1}')" >>"$ledger"
done
test "$(shasum -a 256 "$binary" | awk '{print $1}')" = "$expected"
