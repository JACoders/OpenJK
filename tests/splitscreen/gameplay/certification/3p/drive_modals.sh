#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
sim="$root/tests/splitscreen/external/macos_input_sim"
log="${OPENJK_E2E_RUN_DIR:?}/logs/client.log"
port="${OPENJK_VIRTUAL_GAMEPAD_PORT:?}"
declare -A sent=()
send_once() {
	local marker=$1
	shift
	if rg -q "ModalOwnership: $marker" "$log" 2>/dev/null && [[ -z ${sent[$marker]+x} ]]; then
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$sim" "$@"
		sent[$marker]=1
	fi
}
for _ in $(seq 1 2400); do
	send_once READY_P2_TOP gamepad 1 button 6 down wait 500 gamepad 1 button 6 up
	send_once READY_P2_TOP_CLOSE gamepad 1 button 1 down wait 500 gamepad 1 button 1 up
	send_once READY_P2_CONSOLE gamepad 1 button 4 down gamepad 1 button 6 down wait 500 gamepad 1 button 6 up gamepad 1 button 4 up
	send_once READY_P2_CONSOLE_CLOSE gamepad 1 button 4 down gamepad 1 button 6 down wait 500 gamepad 1 button 6 up gamepad 1 button 4 up
	send_once READY_P2_SCORE gamepad 1 button 4 down
	send_once READY_P2_SCORE_RELEASE gamepad 1 button 4 up
	send_once READY_P2_CHAT gamepad 1 button 15 down wait 500 gamepad 1 button 15 up
	send_once READY_P3_TOP gamepad 2 button 6 down wait 500 gamepad 2 button 6 up
	send_once READY_P3_TOP_CLOSE gamepad 2 button 1 down wait 500 gamepad 2 button 1 up
	send_once READY_P3_SCORE gamepad 2 button 4 down
	send_once READY_P3_SCORE_RELEASE gamepad 2 button 4 up
	if rg -q 'Dumped console text to modal_' "$log" 2>/dev/null; then
		exit 0
	fi
	sleep 0.1
done
exit 1
