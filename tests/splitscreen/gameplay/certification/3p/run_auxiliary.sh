#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
run_id="${OPENJK_CERT_RUN_ID:?OPENJK_CERT_RUN_ID is required}"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
suite="$root/tests/splitscreen/gameplay/results/certification/3p/local-suite/$run_id"
suite_env="$suite/suite.env"

mkdir -p "$suite"
if [[ -f "$suite_env" ]]; then
	rg -qx "run_id=$run_id" "$suite_env"
	rg -qx "frozen_sha256=$expected" "$suite_env"
else
	{
		printf 'run_id=%s\n' "$run_id"
		printf 'frozen_sha256=%s\n' "$expected"
		printf 'started_epoch=%s\n' "$(date +%s)"
	} >"$suite_env"
fi
run_if_missing() {
	local label="$1"
	shift
	local count=0
	if [[ -f "$suite/aux.tsv" ]]; then
		count="$(awk -F '\t' -v label="$label" '$1 == label {count++} END {print count + 0}' \
			"$suite/aux.tsv")"
	fi
	[[ "$count" -le 1 ]]
	[[ "$count" -eq 1 ]] || "$@"
}
run_if_missing modal_ownership \
	"$root/tests/splitscreen/gameplay/certification/run_modal_probe.sh" \
	3 "$suite" "$expected" "$run_id"
run_if_missing virtual_controller_restart \
	"$root/tests/splitscreen/gameplay/certification/run_virtual_controller_probe.sh" \
	3 "$suite" "$expected" "$run_id"
run_if_missing asymmetric_network_recovery \
	"$root/tests/splitscreen/gameplay/certification/run_asymmetric_recovery_probe.sh" \
	3 "$suite" "$expected" "$run_id"
