#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
bin="$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
basepath="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
run_id="${OPENJK_CERT_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM}"
results="$root/tests/splitscreen/gameplay/results/certification/3p/local-suite/$run_id/lifecycle"
home="$(mktemp -d /private/tmp/openjk-gp5-02-lifecycle.XXXXXX)"
mkdir -p "$home/base/splitqa" "$home/base/screenshots" "$results/screenshots"
test "$(shasum -a 256 "$bin" | awk '{print $1}')" = "$expected"
"$root/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$root/tests/splitscreen/cfg/join_spectate_flow.cfg" "$home/base/splitqa/cert_lifecycle.cfg"
perl -0pi -e 's/ui_splitScreenPlayerCount 4/ui_splitScreenPlayerCount 3/; s/^.*(?:ui_splitScreenP4|splitnet_cmd 4|splitnet_assert_lifecycle 4).*\n//mg' \
	"$home/base/splitqa/cert_lifecycle.cfg"
perl -0pi -e 's#splitnet_status#map_restart 0\nwait 720\nsplitnet_assert_lifecycle 1 ALIVE\nsplitnet_assert_lifecycle 2 ALIVE\nsplitnet_assert_lifecycle 3 ALIVE\nscreenshot_png gp5_3p_restart\nmap mp/ffa5\nwait 720\nclosemenu\nsplitnet_assert_lifecycle 1 ALIVE\nsplitnet_assert_lifecycle 2 ALIVE\nsplitnet_assert_lifecycle 3 ALIVE\nscreenshot_png gp5_3p_next_map\nsplitnet_status#' \
	"$home/base/splitqa/cert_lifecycle.cfg"
cp "$root/build-x86_64/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$root/build-x86_64"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$root/build-x86_64/codemp/game/jampgamex86_64.dylib" "$home/base/"
"$bin" +set fs_basepath "$basepath" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port 29529 \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 \
	+set developer 1 +set com_introplayed 1 +exec splitqa/cert_lifecycle.cfg \
	>"$results/candidate.log" 2>&1
find "$home/base/screenshots" -type f -name '*.png' -exec cp {} "$results/screenshots/" \;
printf 'expected_sha256=%s\nafter_sha256=%s\nhomepath=%s\n' "$expected" \
	"$(shasum -a 256 "$bin" | awk '{print $1}')" "$home" >"$results/run.env"
printf 'run_id=%s\n' "$run_id" >>"$results/run.env"
! rg -q 'SplitNetLifecycleAssert: FAIL|ERROR:|Sys_Error' "$results/candidate.log"
test "$(rg -c 'SplitNetLifecycleAssert: PASS' "$results/candidate.log")" -ge 14
test -s "$results/screenshots/splitqa_join_spectate_flow.png"
test -s "$results/screenshots/gp5_3p_restart.png"
test -s "$results/screenshots/gp5_3p_next_map.png"

index="$root/tests/splitscreen/gameplay/results/certification/3p/local-suite/$run_id/aux.tsv"
mkdir -p "$(dirname "$index")"
lifecycle_index="$(dirname "$index")/lifecycle.tsv"
{
	printf 'format\topenjk-cert-local-v1\nrun_id\t%s\n' "$run_id"
	for artifact in "$results/candidate.log" "$results/run.env" "$results"/screenshots/*.png; do
		test -s "$artifact"
		printf 'artifact\t%s\t%s\n' "$artifact" \
			"$(shasum -a 256 "$artifact" | awk '{print $1}')"
	done
} >"$lifecycle_index"
printf 'lifecycle\t%s\t%s\n' "$lifecycle_index" \
	"$(shasum -a 256 "$lifecycle_index" | awk '{print $1}')" >>"$index"
