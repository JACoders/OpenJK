#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
bin="$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
basepath="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
results="$root/tests/splitscreen/gameplay/results/certification/3p/powerduel"
home="$(mktemp -d /private/tmp/openjk-gp5-02-powerduel.XXXXXX)"
port=29526
mkdir -p "$home/base/splitqa" "$home/base/screenshots" "$results/screenshots"
test "$(shasum -a 256 "$bin" | awk '{print $1}')" = "$expected"
"$root/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$root/tests/splitscreen/phase5/three-player/powerduel.cfg" "$home/base/splitqa/cert_powerduel.cfg"
# Drive the Power Duel command proof through the deterministic device-scoped
# QA route. The external SDL bridge remains active as an additional diagnostic,
# but is not allowed to make the 1v2 round timing nondeterministic.
perl -0pi -e \
	's/echo Phase5PowerDuel: READY_ALL_ATTACK/echo Phase5PowerDuel: READY_ALL_ATTACK\nsplitinput_device_axis controller1 1 -24000\nsplitinput_device_button controller1 0 1\nsplitinput_device_axis controller2 1 -22000\nsplitinput_device_button controller2 0 1/' \
	"$home/base/splitqa/cert_powerduel.cfg"
perl -0pi -e \
	's/echo Phase5PowerDuel: READY_ALL_ATTACK/echo Phase5PowerDuel: READY_ALL_ATTACK\nsplitinput_device_key keyboard MOUSE1 1/' \
	"$home/base/splitqa/cert_powerduel.cfg"
perl -0pi -e \
	's/screenshot_png phase5_powerduel_input\nwait 1100/screenshot_png phase5_powerduel_input\nsplitinput_device_key keyboard MOUSE1 0\nsplitinput_device_button controller1 0 0\nsplitinput_device_axis controller1 1 0\nsplitinput_device_button controller2 0 0\nsplitinput_device_axis controller2 1 0\nwait 1100/' \
	"$home/base/splitqa/cert_powerduel.cfg"
cp "$root/build-x86_64/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$root/build-x86_64"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$root/build-x86_64/codemp/game/jampgamex86_64.dylib" "$home/base/"
"$bin" \
	+set fs_basepath "$basepath" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29525 \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 \
	+set developer 1 +set com_introplayed 1 \
	+exec splitqa/cert_powerduel.cfg >"$results/candidate.log" 2>&1
find "$home/base/screenshots" -type f -name '*.png' -exec cp {} "$results/screenshots/" \;
{
	printf 'expected_sha256=%s\n' "$expected"
	printf 'after_sha256=%s\n' "$(shasum -a 256 "$bin" | awk '{print $1}')"
	printf 'input_sequence_sent=device-scoped-in-engine\n'
	printf 'homepath=%s\n' "$home"
} >"$results/run.env"
! rg -q "Split(NetLifecycle|NetStat)Assert: FAIL|SplitInputAssertCmd: FAIL|SplitUIAssert: FAIL|ERROR:|Sys_Error" "$results/candidate.log"
test "$(rg -c "SplitNetLifecycleAssert: PASS" "$results/candidate.log")" -eq 3
test "$(rg -c "SplitNetStatAssert: PASS" "$results/candidate.log")" -eq 3
test "$(rg -c "SplitInputAssertCmd: PASS" "$results/candidate.log")" -eq 6
test -s "$results/screenshots/phase5_powerduel_active.png"
test -s "$results/screenshots/phase5_powerduel_input.png"
