#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
HARNESS="$ROOT/tests/splitscreen/gameplay/run_e2e.sh"
RESULTS="${OPENJK_MODAL_RESULTS:-$ROOT/tests/splitscreen/gameplay/results/modal_ownership}"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
requested="${1:-all}"

"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null
mkdir -p "$RESULTS"

if [[ "$requested" == all ]]; then
	counts=(2 3 4)
elif [[ "$requested" =~ ^[234]$ ]]; then
	counts=("$requested")
else
	echo "usage: $0 [2|3|4|all]" >&2
	exit 2
fi

failures=0
for players in "${counts[@]}"; do
	port=$((29480 + players))
	set +e
	output=$(OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$HARNESS" \
		--case "modal-${players}p" --players "$players" --timeout 180 \
		--artifact-root "$RESULTS/runs" \
		--hash "$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64" \
		--hash "$HERE/modal_common.cfg" --hash "$HERE/modal_sequence_${players}.cfg" \
		--hash "$HERE/launch_game.sh" --hash "$HERE/drive_modals.sh" --hash "$HERE/analyze.py" \
		--client-command "OPENJK_VIRTUAL_GAMEPAD_PORT=$port '$HERE/launch_game.sh' $players" \
		--remote-command "OPENJK_VIRTUAL_GAMEPAD_PORT=$port '$HERE/drive_modals.sh' $players")
	status=$?
	set -e
	manifest=$(printf '%s\n' "$output" | tail -1)
	printf '%s\n' "$manifest" >"$RESULTS/${players}p-manifest-path.txt"
	if [[ $status -ne 0 || ! -f "$manifest" ]]; then
		echo "modal ${players}p infrastructure failed" >&2
		failures=$((failures + 1))
		continue
	fi
	screenshots=$(awk -F '\t' '$1 == "screenshots" {print $2}' "$manifest")
	client_log=$(awk -F '\t' '$1 == "process.client.log" {print $2}' "$manifest")
	if ! PYTHONDONTWRITEBYTECODE=1 python3 "$HERE/analyze.py" "$screenshots" "$players" \
		--log "$client_log" \
		--output "$RESULTS/${players}p-report.json"; then
		echo "modal ${players}p discovered one or more ownership defects" >&2
		failures=$((failures + 1))
	fi
done

[[ $failures -eq 0 ]]
