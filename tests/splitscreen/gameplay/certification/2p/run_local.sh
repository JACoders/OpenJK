#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
here="$root/tests/splitscreen/gameplay/certification/2p"
results="$root/tests/splitscreen/gameplay/results/certification/2p"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
run_id="${OPENJK_CERT_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM}"
suite_dir="$results/local-suite/$run_id"

[[ "$run_id" =~ ^[A-Za-z0-9._-]+$ ]] || exit 2
python3 "$root/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" \
	--expected-client-hash "$expected"
mkdir -p "$suite_dir"
touch "$suite_dir/modes.tsv" "$suite_dir/aux.tsv"

OPENJK_CERT_RUN_ID="$run_id" GP501_DEFER_VALIDATE=1 "$here/run.sh"
OPENJK_CERT_RUN_ID="$run_id" "$here/run_auxiliary.sh"
OPENJK_CERT_RUN_ID="$run_id" "$here/run_modes.sh"
OPENJK_CERT_RUN_ID="$run_id" "$here/run_extended_modes.sh"
"$here/validate.py" "$results" "$expected" "$run_id"
