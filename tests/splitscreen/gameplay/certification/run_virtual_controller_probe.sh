#!/usr/bin/env bash
set -euo pipefail

players="${1:?players required}"
suite="${2:?suite directory required}"
expected="${3:?frozen sha required}"
run_id="${4:?run id required}"
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
build="${OPENJK_BUILD_DIR:-$root/build-x86_64}"
bin="$build/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
results="$suite/controller-recovery"
port="$((29640 + players))"

case "$players" in 2|3) ;; *) exit 2 ;; esac
test "$(shasum -a 256 "$bin" | awk '{print $1}')" = "$expected"
"$root/tests/splitscreen/build_external_input_sim.sh" >/dev/null
mkdir -p "$results"
printf -v client_command '%q' \
	"$root/tests/splitscreen/gameplay/certification/virtual_controller_client.sh"
printf -v driver_command '%q' \
	"$root/tests/splitscreen/gameplay/certification/virtual_controller_driver.sh"
set +e
output="$(
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" \
	"$root/tests/splitscreen/gameplay/run_e2e.sh" \
		--case "cert-controller-${players}p-$run_id" --players "$players" \
		--timeout 150 --artifact-root "$results/runs" \
		--hash "$bin" \
		--hash "$build/codemp/ui/uix86_64.dylib" \
		--hash "$build/codemp/game/jampgamex86_64.dylib" \
		--hash "$build/codemp/cgame/cgamex86_64.dylib" \
		--client-command "OPENJK_VIRTUAL_GAMEPAD_PORT=$port $client_command" \
		--remote-command "OPENJK_VIRTUAL_GAMEPAD_PORT=$port $driver_command"
)"
status=$?
set -e
manifest="$(printf '%s\n' "$output" | tail -n 1)"
test "$status" -eq 0
"$root/tests/splitscreen/gameplay/run_e2e.sh" --validate-only "$manifest"
client_log="$(awk -F '\t' '$1 == "process.client.log" {print $2}' "$manifest")"
screenshots="$(awk -F '\t' '$1 == "screenshots" {print $2}' "$manifest")"
report="$results/report.json"
python3 "$root/tests/splitscreen/gameplay/certification/analyze_virtual_controller.py" \
	--players "$players" --run-id "$run_id" --frozen-sha "$expected" \
	--log "$client_log" --screenshots "$screenshots" --output "$report"
index="$suite/virtual-controller.tsv"
"$root/tests/splitscreen/gameplay/certification/write_aux_index.sh" \
	virtual_controller_restart "$players" "$run_id" "$expected" \
	"$manifest" "$report" "$index"
printf 'virtual_controller_restart\t%s\t%s\n' "$index" \
	"$(shasum -a 256 "$index" | awk '{print $1}')" >>"$suite/aux.tsv"
