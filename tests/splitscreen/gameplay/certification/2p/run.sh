#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
binary="$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
basepath="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
results="$root/tests/splitscreen/gameplay/results/certification/2p"
home="$(mktemp -d /private/tmp/openjk-gp5-01-2p.XXXXXX)"
cfgdir="$home/base/splitqa"
run_id="${OPENJK_CERT_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM}"
suite_env="$results/local-suite/$run_id/suite.env"
started_epoch="$(date +%s)"
evidence="$results/local-suite/$run_id/base.tsv"
journey="$results/local-suite/$run_id/journey"

python3 "$root/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" \
	--expected-client-hash "$expected"
[[ "$run_id" =~ ^[A-Za-z0-9._-]+$ ]] || {
	printf 'Invalid OPENJK_CERT_RUN_ID: %s\n' "$run_id" >&2
	exit 2
}
mkdir -p "$cfgdir" "$home/base/screenshots" "$journey/screenshots" "$(dirname "$evidence")"
if [[ -f "$suite_env" ]]; then
	rg -qx "run_id=$run_id" "$suite_env"
	rg -qx "frozen_sha256=$expected" "$suite_env"
	started_epoch="$(awk -F = '$1 == "started_epoch" {print $2}' "$suite_env")"
	[[ "$started_epoch" =~ ^[1-9][0-9]*$ ]]
else
	{
		printf 'run_id=%s\n' "$run_id"
		printf 'frozen_sha256=%s\n' "$expected"
		printf 'started_epoch=%s\n' "$started_epoch"
	} >"$suite_env"
fi
actual_before="$(shasum -a 256 "$binary" | awk '{print $1}')"
test "$actual_before" = "$expected"

"$root/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$root/tests/splitscreen/cfg/routed_selection_acceptance.cfg" "$cfgdir/certification_2p.cfg"
cp "$root/build-x86_64/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$root/build-x86_64"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$root/build-x86_64/codemp/game/jampgamex86_64.dylib" "$home/base/"

set +e
"$binary" \
	+set fs_basepath "$basepath" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set in_joystick 1 +set net_port 29501 \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 \
	+set com_introplayed 1 +set developer 1 \
	+exec splitqa/certification_2p.cfg >"$journey/candidate.log" 2>&1
exit_code=$?
set -e

find "$home/base/screenshots" -type f -name '*.png' -exec cp {} "$journey/screenshots/" \;
test ! -f "$home/base/routed_selection_continuous_console.txt" ||
	cp "$home/base/routed_selection_continuous_console.txt" "$journey/console.txt"
actual_after="$(shasum -a 256 "$binary" | awk '{print $1}')"

{
	printf 'run_id=%s\n' "$run_id"
	printf 'started_epoch=%s\n' "$started_epoch"
	printf 'expected_sha256=%s\n' "$expected"
	printf 'before_sha256=%s\n' "$actual_before"
	printf 'after_sha256=%s\n' "$actual_after"
	printf 'process_exit=%s\n' "$exit_code"
	printf 'homepath=%s\n' "$home"
	printf 'evidence_index=%s\n' "$evidence"
} >"$journey/run.env"

{
	printf 'format\topenjk-cert-local-v1\n'
	printf 'run_id\t%s\n' "$run_id"
	for artifact in "$journey/candidate.log" "$journey/run.env" "$journey"/screenshots/*.png; do
		[[ -s "$artifact" ]] || continue
		printf 'artifact\t%s\t%s\n' "$artifact" \
			"$(shasum -a 256 "$artifact" | awk '{print $1}')"
	done
} >"$evidence"

if [[ "${GP501_DEFER_VALIDATE:-0}" != 1 ]]; then
	"$root/tests/splitscreen/gameplay/certification/2p/validate.py" \
		"$results" "$expected" "$run_id"
fi
