#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/controller_lifecycle"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/controller_lifecycle"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
SIM="$ROOT/tests/splitscreen/external/macos_input_sim"
mkdir -p "$RESULTS"

run_one() {
	local iteration="$1" port="$((29720 + iteration))"
	local home
	home="$(mktemp -d "/private/tmp/openjk-gp1-controller-${iteration}.XXXXXX")"
	local log="$RESULTS/bridge-run-$iteration.log"
	mkdir -p "$home/base/splitqa" "$home/base/screenshots"
	"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
	cp "$HERE/bridge_lifecycle.cfg" "$home/base/splitqa/"
	cp "$BUILD/codemp/ui/uix86_64.dylib" "$home/base/"
	cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$home/base/"
	cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
	OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$BIN" \
		+set fs_basepath "$BASE" +set fs_homepath "$home" \
		+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
		+set net_port "$((29700 + iteration))" +set r_fullscreen 0 \
		+set s_initsound 0 +set in_joystick 1 \
		+exec splitqa/bridge_lifecycle.cfg >"$log" 2>&1 &
	local pid=$!
	trap 'kill "$pid" 2>/dev/null || true' RETURN
	wait_marker() {
		local marker="$1"
		for _ in $(seq 1 600); do
			rg -q "$marker" "$log" 2>/dev/null && return 0
			kill -0 "$pid" 2>/dev/null || return 1
			sleep 0.1
		done
		return 1
	}
	wait_marker "GP1-CONTROLLER:READY-CONCURRENT"
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" \
		gamepad 1 axis 1 -24000 gamepad 1 button 4 down \
		gamepad 2 axis 0 -20000 gamepad 2 button 5 down \
		gamepad 3 button 6 down
	wait_marker "GP1-CONTROLLER:CONCURRENT-CHECKED"
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" \
		gamepad 1 axis 1 0 gamepad 1 button 4 up \
		gamepad 2 axis 0 0 gamepad 2 button 5 up \
		gamepad 3 button 6 up
	wait_marker "GP1-CONTROLLER:READY-EXTRA-REMAP"
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" \
		gamepad 1 button 7 down gamepad 2 button 8 down
	wait_marker "GP1-CONTROLLER:EXTRA-REMAP-CHECKED"
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" \
		gamepad 1 button 7 up gamepad 2 button 8 up
	wait_marker "GP1-CONTROLLER:READY-PAUSE"
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" gamepad 1 button 4 down
	wait_marker "GP1-CONTROLLER:PAUSE-CHECKED" || true
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" gamepad 1 button 4 up
	wait_marker "GP1-CONTROLLER:READY-RESTART"
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" \
		gamepad 1 axis 1 -24000 gamepad 1 button 4 down \
		gamepad 2 axis 0 -20000 gamepad 2 button 5 down \
		gamepad 3 button 6 down
	wait_marker "GP1-CONTROLLER:RESTART-CHECKED" || true
	OPENJK_VIRTUAL_GAMEPAD_PORT="$port" "$SIM" \
		gamepad 1 axis 1 0 gamepad 1 button 4 up \
		gamepad 2 axis 0 0 gamepad 2 button 5 up \
		gamepad 3 button 6 up
	wait "$pid"
	trap - RETURN
}

for iteration in 1 2; do run_one "$iteration"; done
set +e
"$HERE/unsupported_detach_probe.sh" >"$RESULTS/unsupported-detach.log" 2>&1
unsupported_status=$?
set -e
[[ "$unsupported_status" -eq 3 ]]
python3 "$HERE/analyze.py" \
	--run1 "$RESULTS/bridge-run-1.log" \
	--run2 "$RESULTS/bridge-run-2.log" \
	--unsupported "$RESULTS/unsupported-detach.log" \
	--output "$RESULTS/matrix.json"

manifest="$RESULTS/certification-manifest.tsv"
sha256() { shasum -a 256 "$1" | awk '{print $1}'; }
{
	printf 'format\topenjk-e2e-v1\n'
	printf 'status\tpassed\n'
	printf 'case\tGP1-03-discovery-artifact-integrity\n'
	printf 'discovery_result\tacceptance_not_met\n'
	for input in \
		"$BIN" \
		"$BUILD/codemp/ui/uix86_64.dylib" \
		"$BUILD/codemp/game/jampgamex86_64.dylib" \
		"$BUILD/codemp/cgame/cgamex86_64.dylib" \
		"$SIM"; do
		printf 'hash\t%s\t%s\n' "$input" "$(sha256 "$input")"
	done
	for artifact in \
		"$RESULTS/bridge-run-1.log" \
		"$RESULTS/bridge-run-2.log" \
		"$RESULTS/unsupported-detach.log" \
		"$RESULTS/matrix.json"; do
		printf 'artifact\t%s\t%s\n' "$artifact" "$(sha256 "$artifact")"
	done
} >"$manifest"
"$ROOT/tests/splitscreen/gameplay/run_e2e.sh" --validate-only "$manifest"
