#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/certification/4p"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/certification/4p"
BUILD="$ROOT/build-x86_64"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
DED="$BUILD/openjkded.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
EXPECTED="${GP5_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
sha256() { shasum -a 256 "$1" | awk '{print $1}'; }
python3 "$ROOT/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" \
	--expected-client-hash "$EXPECTED"
[[ "$(sha256 "$BIN")" == "$EXPECTED" ]] || exit 2
run_root="$RESULTS/runs/$(date -u +%Y%m%dT%H%M%SZ)"
mkdir -p "$run_root"
printf '%s\n' "$run_root" >"$RESULTS/latest-run.txt"

install_home() {
	local home="$1"
	mkdir -p "$home/base/splitqa" "$home/base/screenshots"
	"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
	cp "$BUILD/codemp/ui/uix86_64.dylib" "$home/base/"
	cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$home/base/"
	cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
}

write_manifest() {
	local case_name="$1" status="$2" home="$3" log="$4" manifest="$5"
	{
		printf 'format\topenjk-e2e-v1\nstatus\t%s\ncase\t%s\nplayers\t4\nvm\tnative\n' "$status" "$case_name"
		for input in "$BIN" "$DED" "$BUILD/codemp/ui/uix86_64.dylib" \
			"$BUILD/codemp/game/jampgamex86_64.dylib" "$BUILD"/codemp/cgame/cgame*x86_64.dylib; do
			printf 'hash\t%s\t%s\n' "$input" "$(sha256 "$input")"
		done
		printf 'artifact\t%s\t%s\n' "$log" "$(sha256 "$log")"
		while IFS= read -r shot; do
			printf 'artifact\t%s\t%s\n' "$shot" "$(sha256 "$shot")"
		done < <(find "$home/base/screenshots" -type f -print | sort)
	} >"$manifest"
}

local_home="$run_root/local-home"
local_log="$run_root/local.log"
delivery_log="$run_root/bridge-delivery.log"
install_home "$local_home"
cp "$HERE/local_max.cfg" "$local_home/base/splitqa/"
port=29954
OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$BIN" \
	+set fs_basepath "$BASE" +set fs_homepath "$local_home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29952 \
	+set r_fullscreen 0 +set s_initsound 0 +set in_joystick 1 +set developer 1 \
	+exec splitqa/local_max.cfg >"$local_log" 2>&1 &
game_pid=$!
declare -A sent=()
send_once() {
	local marker="$1"; shift
	if rg -q "$marker" "$local_log" 2>/dev/null && [[ -z ${sent[$marker]+x} ]]; then
		sent[$marker]=1
		OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" "$@" >>"$delivery_log" 2>&1 &
	fi
}
for _ in $(seq 1 3600); do
	send_once GP5-03:BRIDGE-P2 gamepad 1 axis 1 -24000 wait 4000 gamepad 1 axis 1 0
	send_once GP5-03:BRIDGE-P3 gamepad 2 axis 0 -20000 wait 4000 gamepad 2 axis 0 0
	send_once GP5-03:BRIDGE-P4 gamepad 3 button 0 down wait 4000 gamepad 3 button 0 up
	send_once GP5-03:BRIDGE-ALL gamepad 1 axis 1 -24000 \
		gamepad 2 axis 0 -20000 gamepad 3 button 0 down \
		wait 4000 gamepad 1 axis 1 0 \
		gamepad 2 axis 0 0 gamepad 3 button 0 up
	# Individual probes must release before FORCE-ALL starts. A ten-second
	# hold overlapped P4's individual release with the simultaneous probe and
	# cleared only P4's input before the engine sampled it.
	send_once GP5-03:FORCE-P2 gamepad 1 button 10 down wait 4000 gamepad 1 button 10 up
	send_once GP5-03:FORCE-P3 gamepad 2 button 10 down wait 4000 gamepad 2 button 10 up
	send_once GP5-03:FORCE-P4 gamepad 3 button 10 down wait 4000 gamepad 3 button 10 up
	send_once GP5-03:FORCE-ALL gamepad 1 button 10 down gamepad 2 button 10 down gamepad 3 button 10 down \
		wait 10000 gamepad 1 button 10 up gamepad 2 button 10 up gamepad 3 button 10 up
	kill -0 "$game_pid" 2>/dev/null || break
	sleep 0.1
done
if wait "$game_pid"; then
	local_exit=0
else
	local_exit=$?
fi
local_status=passed
if (( local_exit != 0 )) || ! "$HERE/strict_scan.sh" "$local_log" ||
	[[ "$(rg -c 'SplitProfileAssert: PASS' "$local_log")" -lt 12 ]] ||
	[[ "$(rg -c 'SplitInputAssertCmd: PASS' "$local_log")" -lt 11 ]] ||
	[[ "$(rg -c 'SplitNetLifecycleAssert: PASS' "$local_log")" -lt 8 ]]; then
	local_status=failed
fi
write_manifest gp5-03-local-max "$local_status" "$local_home" "$local_log" "$run_root/local-manifest.tsv"

controlled_home="$run_root/controlled-home"
controlled_log="$run_root/controlled.log"
server_log="$run_root/server.log"
install_home "$controlled_home"
cp "$HERE/controlled.cfg" "$controlled_home/base/splitqa/"
server_home="$run_root/server-home"
mkdir -p "$server_home/base"
"$ROOT/tests/splitscreen/install_assets.sh" "$server_home" >/dev/null
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$server_home/base/"
"$DED" +set fs_basepath "$BASE" +set fs_homepath "$server_home" +set vm_game 0 \
	+set net_port 29953 +set g_password "" +set g_antiFakePlayer 0 +set sv_maxclients 8 +map mp/ffa3 >"$server_log" 2>&1 &
server_pid=$!
sleep 3
if "$BIN" +set fs_basepath "$BASE" +set fs_homepath "$controlled_home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29955 \
	+set r_fullscreen 0 +set s_initsound 0 +exec splitqa/controlled.cfg >"$controlled_log" 2>&1; then
	controlled_exit=0
else
	controlled_exit=$?
fi
if kill -0 "$server_pid" 2>/dev/null; then
	kill -TERM "$server_pid"
fi
if wait "$server_pid" 2>/dev/null; then
	server_exit=0
else
	server_exit=$?
fi
controlled_status=passed
if (( controlled_exit != 0 )) || ! "$HERE/strict_scan.sh" "$controlled_log" ||
	[[ "$(rg -c 'SplitNetLifecycleAssert: PASS' "$controlled_log")" -lt 8 ]] ||
	[[ "$(rg -c 'SplitNetStatAssert: PASS' "$controlled_log")" -lt 5 ]]; then
	controlled_status=failed
fi
write_manifest gp5-03-controlled "$controlled_status" "$controlled_home" "$controlled_log" "$run_root/controlled-manifest.tsv"

ui_home="$run_root/visible-ui-home"
ui_runner_log="$run_root/visible-ui-runner.log"
if OPENJK_BUILD_DIR="$BUILD" OPENJK_BIN="$BIN" OPENJK_HOMEPATH="$ui_home" \
	"$ROOT/tests/splitscreen/run_routed_selection_4p_acceptance.sh" >"$ui_runner_log" 2>&1; then
	ui_exit=0
else
	ui_exit=$?
fi
ui_game_log="$ui_home/base/routed-selection-4p.stdout.log"
ui_status=passed
if (( ui_exit != 0 )) || [[ ! -s "$ui_game_log" ]] ||
	! "$HERE/strict_scan.sh" "$ui_runner_log" "$ui_game_log" ||
	! rg -q 'Four-player routed selection acceptance passed' "$ui_runner_log"; then
	ui_status=failed
fi
write_manifest gp5-03-visible-ui "$ui_status" "$ui_home" "$ui_game_log" "$run_root/visible-ui-manifest.tsv"

for owned_case in slot-churn fifth modal; do
	owned_log="$run_root/$owned_case-runner.log"
	case "$owned_case" in
		slot-churn) owned_runner="$HERE/run_slot_churn.sh" ;;
		fifth) owned_runner="$HERE/run_controlled_fifth.sh" ;;
		modal) owned_runner="$HERE/run_modal.sh" ;;
	esac
	if GP5_EXPECTED_SHA256="$EXPECTED" GP5_CASE_ROOT="$run_root/owned" \
		OPENJK_BUILD_DIR="$BUILD" OPENJK_BIN="$BIN" \
		"$owned_runner" >"$owned_log" 2>&1; then
		owned_status=passed
	else
		owned_status=failed
	fi
	printf '%s\n' "$owned_status" >"$run_root/$owned_case-status.txt"
done
modal_report="$ROOT/tests/splitscreen/gameplay/results/modal_ownership/4p-report.json"
if [[ -f "$modal_report" ]]; then
	cp "$modal_report" "$run_root/modal-report.json"
fi

public_home="$run_root/public-home"
public_log="$run_root/public.log"
install_home "$public_home"
cp "$HERE/public_attempt.cfg" "$public_home/base/splitqa/"
if [[ "${GP5_ALLOW_PUBLIC:-0}" == 1 ]]; then
	if "$BIN" +set fs_basepath "$BASE" +set fs_homepath "$public_home" \
		+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29956 \
		+set r_fullscreen 0 +set s_initsound 0 +exec splitqa/public_attempt.cfg >"$public_log" 2>&1; then
		public_exit=0
	else
		public_exit=$?
	fi
	public_status=passed
	if (( public_exit != 0 )) || ! "$HERE/strict_scan.sh" "$public_log" ||
		! rg -q 'SplitNet party: all 4 local players active' "$public_log"; then
		public_status=failed
	fi
else
	printf 'GP5-03 public attempt skipped: explicit third-party network authorization required.\n' >"$public_log"
	public_status=skipped
fi
write_manifest gp5-03-public-attempt "$public_status" "$public_home" "$public_log" "$run_root/public-manifest.tsv"

[[ "$(sha256 "$BIN")" == "$EXPECTED" ]] || exit 3
python3 "$HERE/analyze.py" "$run_root" "$RESULTS/matrix.json" "$EXPECTED"
python3 "$HERE/validate.py" "$RESULTS/matrix.json" "$EXPECTED"
