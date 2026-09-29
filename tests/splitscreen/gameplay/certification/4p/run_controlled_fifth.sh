#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/certification/4p"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
DED="${OPENJK_DED_BIN:-$BUILD/openjkded.x86_64}"
EXPECTED="${GP5_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
ARTIFACT_ROOT="${GP5_CASE_ROOT:-$ROOT/tests/splitscreen/gameplay/results/certification/4p/owned}"
HARNESS="$ROOT/tests/splitscreen/gameplay/run_e2e.sh"

python3 "$ROOT/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" \
	--expected-client-hash "$EXPECTED"
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]] || {
	echo "hash gate failed" >&2
	exit 2
}
output="$(
	"$HARNESS" --case gp5-03-controlled-fifth --players 4 --timeout 180 \
		--artifact-root "$ARTIFACT_ROOT" \
		--hash "$BIN" --hash "$DED" \
		--hash "$HERE/controlled_fifth_host.cfg" \
		--hash "$HERE/controlled_fifth_upstream.cfg" \
		--server-command "'$HERE/launch_controlled_server.sh'" \
		--remote-command "'$HERE/launch_fifth_client.sh'" \
		--client-command "'$HERE/launch_fifth_host.sh'"
)"
manifest="$(printf '%s\n' "$output" | tail -1)"
[[ -f "$manifest" ]]
"$HARNESS" --validate-only "$manifest"
host_log="$(awk -F '\t' '$1 == "process.client.log" {print $2}' "$manifest")"
fifth_log="$(awk -F '\t' '$1 == "process.remote.log" {print $2}' "$manifest")"
server_log="$(awk -F '\t' '$1 == "process.server.log" {print $2}' "$manifest")"
screenshots="$(awk -F '\t' '$1 == "screenshots" {print $2}' "$manifest")"
"$HERE/strict_scan.sh" "$host_log" "$fifth_log" "$server_log"
rg -q 'GP5-03:FIFTH-READY' "$host_log"
[[ "$(rg -c 'SplitNetStatAssert: PASS' "$host_log")" -eq 4 ]]
[[ "$(rg -c 'SplitNetLifecycleAssert: PASS' "$host_log")" -eq 4 ]]
rg -q 'SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE' "$fifth_log"
rg -q 'GP5-03:FIFTH-CONNECTED' "$fifth_log"
rg -q 'ClientConnect: 4 .*"GP5_Independent_Fifth' "$server_log"
rg -q 'ChangeTeam: 4 .*"GP5_Independent_Fifth.* SPECTATOR -> FREE' "$server_log"
test -s "$screenshots/gp5_4p_with_remote_fifth.png"
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]]
printf '%s\n' "$manifest"
