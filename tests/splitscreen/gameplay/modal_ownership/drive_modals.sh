#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
players="${1:?player count required}"
log="${OPENJK_E2E_RUN_DIR:?}/logs/client.log"
port="${OPENJK_VIRTUAL_GAMEPAD_PORT:?}"

declare -A sent=()
send_once() {
	local marker="$1"
	shift
	if rg -q "ModalOwnership: $marker" "$log" 2>/dev/null && [[ -z ${sent[$marker]+x} ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" "$@"
		sent[$marker]=1
	fi
}

for _ in $(seq 1 2400); do
	send_once READY_P2_TOP gamepad 1 button 6 tap
	send_once READY_P2_TOP_CLOSE gamepad 1 button 1 tap
	send_once READY_P2_CONSOLE gamepad 1 button 4 down gamepad 1 button 6 down wait 180 gamepad 1 button 6 up gamepad 1 button 4 up
	send_once READY_P2_CONSOLE_CLOSE gamepad 1 button 4 down gamepad 1 button 6 down wait 180 gamepad 1 button 6 up gamepad 1 button 4 up
	send_once READY_P2_SCORE gamepad 1 button 4 down
	send_once READY_P2_SCORE_RELEASE gamepad 1 button 4 up
	send_once READY_P2_CHAT gamepad 1 button 15 tap
	if (( players >= 3 )); then
		send_once READY_P3_TOP gamepad 2 button 6 tap
		send_once READY_P3_TOP_CLOSE gamepad 2 button 1 tap
		send_once READY_P3_SCORE gamepad 2 button 4 down
		send_once READY_P3_SCORE_RELEASE gamepad 2 button 4 up
	fi
	if (( players >= 4 )); then
		send_once READY_P4_TOP gamepad 3 button 6 tap
		send_once READY_P4_TOP_CLOSE gamepad 3 button 1 tap
		send_once READY_P4_SCORE gamepad 3 button 4 down
		send_once READY_P4_SCORE_RELEASE gamepad 3 button 4 up
	fi
	if rg -q 'Dumped console text to modal_' "$log" 2>/dev/null; then
		printf 'modal driver complete players=%s markers=%s\n' "$players" "${!sent[*]}"
		exit 0
	fi
	sleep 0.1
done

echo "modal driver timed out; sent markers: ${!sent[*]}" >&2
exit 1
