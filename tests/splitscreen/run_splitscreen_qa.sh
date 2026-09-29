#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-native}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.arm64.app/Contents/MacOS/openjk.arm64}"
ARCH_SUFFIX="${OPENJK_ARCH_SUFFIX:-arm64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOMEPATH="${OPENJK_HOMEPATH:-$ROOT/runtime-home}"
CFG_SRC="$ROOT/tests/splitscreen/cfg"
CFG_DST="$HOMEPATH/base/splitqa"
LOG_DIR="$HOMEPATH/base/qa-logs"

DEFAULT_TESTS=(
	multivm_2p_ffa
	multivm_3p_ffa
	multivm_4p_ffa
	local_2p_ffa
	local_3p_ffa
	local_4p_ffa
	local_4p_team
	local_4p_ctf
	local_duel
	respawn_flow
	profile_cvars
	profile_customization_flow
	join_spectate_flow
	ui_console_keyboard
	controller_bind
	input_routing_sim
	input_isolation_2p
	input_isolation_3p
	input_isolation_4p
	gameplay_input_sim
	controls_force_combat_sim
	splitnet_localhost
	party_event_attach
	party_rejoin
	stock_host_handoff
	stock_browser_handoff
)

if [[ $# -gt 0 ]]; then
	TESTS=("$@")
else
	TESTS=("${DEFAULT_TESTS[@]}")
fi

if [[ ! -x "$BIN" ]]; then
	echo "Missing OpenJK binary: $BIN" >&2
	exit 2
fi

mkdir -p "$CFG_DST" "$LOG_DIR" "$HOMEPATH/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOMEPATH" >/dev/null
cp "$CFG_SRC"/*.cfg "$CFG_DST"/
cp "$BUILD_DIR/codemp/ui/ui${ARCH_SUFFIX}.dylib" "$HOMEPATH/base/"
for cgame_module in "$BUILD_DIR"/codemp/cgame/cgame*"${ARCH_SUFFIX}".dylib; do
	cp "$cgame_module" "$HOMEPATH/base/"
done
cp "$BUILD_DIR/codemp/game/jampgame${ARCH_SUFFIX}.dylib" "$HOMEPATH/base/"

FAILURES=0
for test_name in "${TESTS[@]}"; do
	cfg="$CFG_DST/$test_name.cfg"
	log="$LOG_DIR/$test_name.stdout.txt"

	if [[ ! -f "$cfg" ]]; then
		echo "missing cfg: $test_name" >&2
		FAILURES=$((FAILURES + 1))
		continue
	fi

	echo "==> $test_name"
	set +e
	"$BIN" \
		+set fs_basepath "$BASEPATH" \
		+set fs_homepath "$HOMEPATH" \
		+set vm_game 0 \
		+set vm_cgame 0 \
		+set vm_ui 0 \
		+set net_port 29170 \
		+set r_fullscreen 0 \
		+set s_initsound "${OPENJK_SOUND:-0}" \
		+set in_joystick 1 \
		+set logfile 2 \
		+exec "splitqa/$test_name.cfg" \
		> "$log" 2>&1
	status=$?
	set -e

	if [[ $status -ne 0 ]]; then
		echo "failed: $test_name exited $status"
		FAILURES=$((FAILURES + 1))
		continue
	fi

	if grep -Eiq "segmentation|assertion|fatal|recursive error|z_malloc failed|hunk_alloc failed|ERROR:" "$log"; then
		echo "failed: $test_name emitted an error marker"
		FAILURES=$((FAILURES + 1))
		continue
	fi
	if grep -Eq "SplitInputAssertModel: FAIL" "$log"; then
		echo "failed: $test_name emitted a split-input assertion failure"
		FAILURES=$((FAILURES + 1))
		continue
	fi
	if grep -Eq "SplitInputAssertCmd: FAIL" "$log"; then
		echo "failed: $test_name emitted a split-input command assertion failure"
		FAILURES=$((FAILURES + 1))
		continue
	fi
	if grep -Eq "SplitProfileAssert: FAIL" "$log"; then
		echo "failed: $test_name emitted a split-screen profile assertion failure"
		FAILURES=$((FAILURES + 1))
		continue
	fi
	if grep -Eq "SplitStateAssert: FAIL" "$log"; then
		echo "failed: $test_name emitted a split-screen state assertion failure"
		FAILURES=$((FAILURES + 1))
		continue
	fi
	if grep -Eq "Split(UI|NetLifecycle|NetStat)Assert: FAIL|SplitNetStagePair: FAIL" "$log"; then
		echo "failed: $test_name emitted a split-screen network/UI assertion failure"
		FAILURES=$((FAILURES + 1))
		continue
	fi

	required_patterns=()
	case "$test_name" in
		multivm_2p_ffa)
			required_patterns=("VM_Create: cgame2arm64.dylib succeeded" "SplitNet P2: enabled=1 state=8 .*clientNum=1 snap=1")
			;;
		multivm_3p_ffa)
			required_patterns=("VM_Create: cgame2arm64.dylib succeeded" "VM_Create: cgame3arm64.dylib succeeded" "SplitNet P2: enabled=1 state=8 .*clientNum=1 snap=1" "SplitNet P3: enabled=1 state=8 .*clientNum=2 snap=1")
			;;
		multivm_4p_ffa)
			required_patterns=("VM_Create: cgame2arm64.dylib succeeded" "VM_Create: cgame3arm64.dylib succeeded" "VM_Create: cgame4arm64.dylib succeeded" "SplitNet P2: enabled=1 state=8 .*clientNum=1 snap=1" "SplitNet P3: enabled=1 state=8 .*clientNum=2 snap=1" "SplitNet P4: enabled=1 state=8 .*clientNum=3 snap=1")
			;;
		local_2p_ffa)
			required_patterns=("SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE")
			;;
		local_3p_ffa)
			required_patterns=("SplitNetLifecycleAssert: PASS player=1 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=3 expected=ALIVE actual=ALIVE")
			;;
		local_4p_ffa|respawn_flow|profile_cvars|ui_console_keyboard)
			required_patterns=("SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=3 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE")
			;;
		local_4p_team)
			required_patterns=("SplitNetStatAssert: PASS player=2 field=team op=eq expected=2 actual=2" "SplitNetStatAssert: PASS player=3 field=team op=eq expected=1 actual=1" "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE")
			;;
		local_4p_ctf)
			required_patterns=("SplitNetStatAssert: PASS player=2 field=team op=eq expected=2 actual=2" "SplitNetStatAssert: PASS player=3 field=team op=eq expected=1 actual=1" "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE")
			;;
		local_duel)
			required_patterns=("SplitNetStatAssert: PASS player=2 field=team op=eq expected=0 actual=0" "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE")
			;;
		splitnet_localhost)
			required_patterns=("SplitNet P2: enabled=1 state=8" "SplitNet P3: enabled=1 state=8" "SplitNet P4: enabled=1 state=8")
			;;
		party_event_attach)
			required_patterns=("SplitNet party: waiting for primary client" "SplitNet party: all 4 local players active" "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active" "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE")
			;;
		party_rejoin)
			required_patterns=("SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=partial actual=partial" "SplitNet P2: reconnecting before" "SplitNet P2: sent deferred" "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active" "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE")
			;;
		stock_host_handoff)
			required_patterns=("SplitUIAssert: PASS cvar=ui_splitScreenHostPending expected=1 actual=1" "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=host_pending actual=host_pending")
			;;
		stock_browser_handoff)
			required_patterns=("SplitUIAssert: PASS cvar=ui_splitScreenHostPending expected=0 actual=0" "SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=join_pending actual=join_pending")
			;;
		reported_startup_regressions_2p)
			required_patterns=("SplitUIAssert: PASS cvar=ui_splitScreenSetupComplete expected=1 actual=1" "SplitUIAssert: PASS cvar=cl_splitScreenRenderReady expected=0 actual=0" "SplitNet party: using character profiles selected before connect" "SplitNet party: joined 2 preconfigured local players" "SplitUIAssert: PASS cvar=cl_splitScreenRenderReady expected=1 actual=1" "SplitUIAssert: PASS cvar=ui_splitScreenSetupComplete expected=0 actual=0" "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE")
			;;
		controller_bind)
			required_patterns=("Player 2 controller bind: \\+forward = JOY3" "cl_splitScreenP2Bind00 = .*3" "cl_splitScreenP2Bind01 = .*-1")
			;;
		input_routing_sim)
			required_patterns=("SplitInputRoute: keyboardOwner=1 controller1Owner=2" "SplitInputAssert after_controller" "SplitInputAssert after_keyboard" "SplitInputAssert after_mouse" "SplitInputSim: key device=controller1 player=2" "SplitInputSim: key device=keyboard player=1" "SplitInputSim: mouse device=mouse" "SplitInputAssertModel: PASS player=1" "SplitInputAssertModel: PASS player=2")
			;;
		input_isolation_2p)
			required_patterns=("SplitInputRoute: keyboardOwner=1 controller1Owner=2" "InputIsolationAssert ui_controller_p2_only" "InputIsolationAssert ui_keyboard_p1_only" "InputIsolationAssert mouse_clamped_to_p1" "SplitUIAssert: PASS cvar=ui_splitScreenMouseOwner expected=1 actual=1" "SplitUIAssert: PASS cvar=ui_splitScreenMouseCursorX expected=600 actual=600" "SplitUIAssert: PASS cvar=ui_splitScreenMouseCursorY expected=200 actual=200" "InputIsolationAssert gameplay_keyboard_p1_only" "InputIsolationAssert gameplay_mouse_not_p2" "InputIsolationAssert gameplay_controller1_p2_only" "SplitInputAssertCmd: PASS player=2 expectedForward=127" "SplitInputAssertCmd: PASS player=2 .*expectedButtons=1 actualButtons=1")
			;;
		input_isolation_3p)
			required_patterns=("SplitInputRoute: keyboardOwner=1 controller1Owner=2 controller2Owner=3" "InputIsolation3Assert ui_controller1_p2_only" "InputIsolation3Assert ui_controller2_p3_only" "InputIsolation3Assert ui_keyboard_p1_only" "InputIsolation3Assert mouse_clamped_to_p1" "SplitUIAssert: PASS cvar=ui_splitScreenMouseOwner expected=1 actual=1" "SplitUIAssert: PASS cvar=ui_splitScreenMouseCursorX expected=600 actual=600" "SplitUIAssert: PASS cvar=ui_splitScreenMouseCursorY expected=200 actual=200" "InputIsolation3Assert gameplay_keyboard_p1_only" "InputIsolation3Assert gameplay_mouse_not_p2_or_p3" "InputIsolation3Assert gameplay_controller1_p2_only" "InputIsolation3Assert gameplay_controller2_p3_only" "SplitInputAssertCmd: PASS player=2 expectedForward=127" "SplitInputAssertCmd: PASS player=3 expectedForward=0 actualForward=0 expectedRight=-80 actualRight=-80" "SplitInputAssertCmd: PASS player=3 .*expectedButtons=1 actualButtons=1")
			;;
		input_isolation_4p)
			required_patterns=("SplitInputRoute: keyboardOwner=1 controller1Owner=2 controller2Owner=3 controller3Owner=4" "InputIsolation4Assert ui_controller1_p2_only" "InputIsolation4Assert ui_controller2_p3_only" "InputIsolation4Assert ui_controller3_p4_only" "InputIsolation4Assert ui_keyboard_p1_only" "InputIsolation4Assert mouse_clamped_to_p1" "SplitUIAssert: PASS cvar=ui_splitScreenMouseOwner expected=1 actual=1" "SplitUIAssert: PASS cvar=ui_splitScreenMouseCursorX expected=280 actual=280" "SplitUIAssert: PASS cvar=ui_splitScreenMouseCursorY expected=200 actual=200" "InputIsolation4Assert gameplay_keyboard_p1_only" "InputIsolation4Assert gameplay_mouse_not_p2_p3_or_p4" "InputIsolation4Assert gameplay_controller1_p2_only" "InputIsolation4Assert gameplay_controller2_p3_only" "InputIsolation4Assert gameplay_controller3_p4_only" "SplitInputAssertCmd: PASS player=2 expectedForward=127" "SplitInputAssertCmd: PASS player=3 expectedForward=0 actualForward=0 expectedRight=-80 actualRight=-80" "SplitInputAssertCmd: PASS player=4 expectedForward=96" "SplitInputAssertCmd: PASS player=4 .*expectedButtons=1 actualButtons=1")
			;;
		gameplay_input_sim)
			required_patterns=("SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=3 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE" "SplitGameplayAssert keyboard_forward" "SplitGameplayAssert controller1_forward" "SplitGameplayAssert controller2_strafe" "SplitGameplayAssert controller3_altattack" "SplitGameplayAssert mouse_no_controller_bleed" "SplitGameplayAssert swapped_device_ownership" "SplitInputSim: axis device=controller1 player=2 axis=1 value=-127" "SplitInputRoute: keyboardOwner=2 controller1Owner=1 controller2Owner=3 controller3Owner=4" "SplitInputSim: axis device=controller2 player=3 axis=0 value=-80" "SplitInputSim: button device=controller3 player=4 button=1 pressed=1" "SplitInputAssertCmd: PASS player=4")
			;;
		profile_customization_flow)
			required_patterns=("SplitProfileAssert: PASS player=2 key=name expected=QA_Profile2B" "SplitProfileAssert: PASS player=3 key=model expected=reborn/default" "SplitProfileAssert: PASS player=4 key=saber1 expected=desann" "SplitProfileAssert: PASS player=4 key=forcepowers expected=7-1-333003000313003120")
			;;
		join_spectate_flow)
			required_patterns=("SplitNetLifecycleAssert: PASS player=2 expected=SPECTATOR actual=SPECTATOR" "SplitNetLifecycleAssert: PASS player=3 expected=SPECTATOR actual=SPECTATOR" "SplitNetLifecycleAssert: PASS player=4 expected=SPECTATOR actual=SPECTATOR" "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE")
			;;
		controls_force_combat_sim)
			required_patterns=("SplitControlsAssert default_controls" "SplitControlsAssert custom_force_bindings" "SplitInputAssertCmd: PASS player=2 .*expectedButtons=0 .*expectedGeneric=5 actualGeneric=5" "SplitInputAssertCmd: PASS player=3 .*expectedButtons=1024" "SplitInputAssertCmd: PASS player=4 .*expectedButtons=64" "SplitCombatAssert command_stream" "SplitNetLifecycleAssert: PASS player=2 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=3 expected=ALIVE actual=ALIVE" "SplitNetLifecycleAssert: PASS player=4 expected=ALIVE actual=ALIVE")
			;;
	esac

	for pattern in "${required_patterns[@]}"; do
		if ! grep -Eq "$pattern" "$log"; then
			echo "failed: $test_name missing required log pattern: $pattern"
			FAILURES=$((FAILURES + 1))
			continue 2
		fi
	done

	oracle_players=0
	oracle_image=""
	case "$test_name" in
		multivm_2p_ffa)
			oracle_players=2
			oracle_image="$HOMEPATH/base/screenshots/splitqa_multivm_2p_after.png"
			;;
		multivm_3p_ffa)
			oracle_players=3
			oracle_image="$HOMEPATH/base/screenshots/splitqa_multivm_3p_after.png"
			;;
		multivm_4p_ffa)
			oracle_players=4
			oracle_image="$HOMEPATH/base/screenshots/splitqa_multivm_4p_after.png"
			;;
		local_2p_ffa)
			oracle_players=2
			oracle_image="$HOMEPATH/base/screenshots/splitqa_local_2p_ffa_start.png"
			;;
		local_3p_ffa)
			oracle_players=3
			oracle_image="$HOMEPATH/base/screenshots/splitqa_local_3p_ffa_start.png"
			;;
		local_4p_ffa)
			oracle_players=4
			oracle_image="$HOMEPATH/base/screenshots/splitqa_local_4p_ffa_start.png"
			;;
		local_4p_team)
			oracle_players=4
			oracle_image="$HOMEPATH/base/screenshots/splitqa_local_4p_team_start.png"
			;;
		local_4p_ctf)
			oracle_players=4
			oracle_image="$HOMEPATH/base/screenshots/splitqa_local_4p_ctf_start.png"
			;;
		local_duel)
			oracle_players=2
			oracle_image="$HOMEPATH/base/screenshots/splitqa_local_duel_start.png"
			;;
	esac
	if [[ $oracle_players -gt 0 ]]; then
		if ! python3 "$ROOT/tests/splitscreen/assert_screenshot.py" "$oracle_image" "$oracle_players" >>"$log" 2>&1; then
			echo "failed: $test_name screenshot oracle rejected $oracle_image"
			FAILURES=$((FAILURES + 1))
			continue
		fi
	fi

	echo "passed: $test_name"
done

echo "logs: $LOG_DIR"
if [[ $FAILURES -ne 0 ]]; then
	echo "$FAILURES split-screen QA test(s) failed" >&2
	exit 1
fi

echo "all split-screen QA tests passed"
