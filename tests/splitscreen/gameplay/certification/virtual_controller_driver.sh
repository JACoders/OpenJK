#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
sim="$root/tests/splitscreen/external/macos_input_sim"
log="${OPENJK_E2E_RUN_DIR:?}/logs/client.log"
port="${OPENJK_VIRTUAL_GAMEPAD_PORT:?}"
players="${OPENJK_E2E_PLAYERS:?}"

wait_marker() {
	local marker="$1"
	for _ in $(seq 1 1200); do
		rg -q "$marker" "$log" 2>/dev/null && return 0
		sleep 0.1
	done
	return 1
}
press() {
	if ((players == 2)); then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$sim" \
			gamepad 1 axis 1 -24000 gamepad 1 button 4 down
	else
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$sim" \
			gamepad 1 axis 1 -24000 gamepad 1 button 4 down \
			gamepad 2 axis 0 -20000 gamepad 2 button 5 down
	fi
}
release() {
	if ((players == 2)); then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$sim" \
			gamepad 1 axis 1 0 gamepad 1 button 4 up
	else
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$sim" \
			gamepad 1 axis 1 0 gamepad 1 button 4 up \
			gamepad 2 axis 0 0 gamepad 2 button 5 up
	fi
}

wait_marker "CERT-CONTROLLER:PRE-READY"
press
wait_marker "CERT-CONTROLLER:PRE-CHECKED"
release
wait_marker "CERT-CONTROLLER:POST-READY"
press
wait_marker "CERT-CONTROLLER:POST-CHECKED"
release
wait_marker "CERT-CONTROLLER:COMPLETE"
printf 'virtual controller driver complete players=%s\n' "$players"
