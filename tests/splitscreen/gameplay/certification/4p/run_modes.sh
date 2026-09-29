#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
here="$root/tests/splitscreen/gameplay/certification/4p"
binary="$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
build="$root/build-x86_64"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
base="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
results="$root/tests/splitscreen/gameplay/results/certification/4p/modes"
harness="$root/tests/splitscreen/gameplay/run_e2e.sh"
run_id="${OPENJK_CERT_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM}"
suite="$root/tests/splitscreen/gameplay/results/certification/4p/local-suite/$run_id"
ledger="$suite/modes.tsv"
mode_env="$suite/modes.env"
all_modes=(ffa holocron jedimaster duel powerduel ctf cty team-ffa siege)
resume=0
selected=()

usage() {
	echo "usage: run_modes.sh [--resume] [ffa|holocron|jedimaster|duel|powerduel|ctf|cty|team-ffa|siege ...]" >&2
	exit 2
}

while (( $# )); do
	case "$1" in
		--resume) resume=1 ;;
		--help|-h) usage ;;
		-*) usage ;;
		*) selected+=("$1") ;;
	esac
	shift
done
if (( ${#selected[@]} == 0 )); then
	selected=("${all_modes[@]}")
	validate_complete_suite=1
else
	validate_complete_suite=0
fi
for mode in "${selected[@]}"; do
	case "$mode" in
		ffa|holocron|jedimaster|duel|powerduel|ctf|cty|team-ffa|siege) ;;
		*) usage ;;
	esac
done

[[ "$run_id" =~ ^[A-Za-z0-9._-]+$ ]] || exit 2
test "$(shasum -a 256 "$binary" | awk '{print $1}')" = "$expected"
mkdir -p "$results" "$suite"
touch "$ledger"
if [[ ! -f "$mode_env" ]]; then
	{
		printf 'format=openjk-cert-modes-v1\n'
		printf 'run_id=%s\n' "$run_id"
		printf 'started_epoch=%s\n' "$(date +%s)"
		printf 'before_sha256=%s\n' "$expected"
		printf 'after_sha256=\n'
	} >"$mode_env"
else
	awk -F = -v run_id="$run_id" -v hash="$expected" '
		$1 == "format" && $2 == "openjk-cert-modes-v1" { format = 1 }
		$1 == "run_id" && $2 == run_id { suite = 1 }
		$1 == "started_epoch" && $2 ~ /^[1-9][0-9]*$/ { started = 1 }
		$1 == "before_sha256" && $2 == hash { binary = 1 }
		END { exit !(format && suite && started && binary) }
	' "$mode_env"
fi

record() {
	local case_name="$1" manifest="$2"
	local ledger_tmp="$ledger.tmp.$$"
	test -s "$manifest"
	awk -F '\t' -v target="$case_name" '$1 != target' "$ledger" >"$ledger_tmp"
	mv "$ledger_tmp" "$ledger"
	printf '%s\t%s\t%s\n' "$case_name" "$manifest" \
		"$(shasum -a 256 "$manifest" | awk '{print $1}')" >>"$ledger"
}

already_recorded() {
	local case_name="$1" row manifest manifest_sha
	row="$(awk -F '\t' -v target="$case_name" '$1 == target { print; count++ } END { if (count != 1) exit 1 }' "$ledger")" ||
		return 1
	IFS=$'\t' read -r _ manifest manifest_sha <<<"$row"
	[[ -s "$manifest" ]] || return 1
	[[ "$(shasum -a 256 "$manifest" | awk '{print $1}')" == "$manifest_sha" ]] || return 1
	"$harness" --validate-only "$manifest" >/dev/null || return 1
	awk -F '\t' -v target="$case_name" -v hash="$expected" '
		$1 == "status" && $2 == "passed" { status = 1 }
		$1 == "case" && $2 == target { case_name = 1 }
		$1 == "players" && $2 == "4" { players = 1 }
		$1 == "hash" && $3 == hash { binary = 1 }
		END { exit !(status && case_name && players && binary) }
	' "$manifest"
}

run_command() {
	local case_name="$1" command="$2" output manifest
	if (( resume )) && already_recorded "$case_name"; then
		printf 'resume: keeping verified %s ledger cell\n' "$case_name"
		return
	fi
	output="$("$harness" --case "$case_name" --players 4 --vm native --timeout 300 \
		--artifact-root "$results" --hash "$binary" --client-command "$command")"
	manifest="$(printf '%s\n' "$output" | tail -1)"
	record "$case_name" "$manifest"
}

run_cfg() {
	local case_name="$1" cfg="$2" gametype="$3" command
	command="mkdir -p \"\$OPENJK_E2E_HOMEPATH/base/splitqa\"; '$root/tests/splitscreen/install_assets.sh' \"\$OPENJK_E2E_HOMEPATH\" >/dev/null; ln -sfn \"\$OPENJK_E2E_SCREENSHOTS\" \"\$OPENJK_E2E_HOMEPATH/base/screenshots\"; cp '$cfg' \"\$OPENJK_E2E_HOMEPATH/base/splitqa/case.cfg\"; cp '$build/codemp/ui/uix86_64.dylib' \"\$OPENJK_E2E_HOMEPATH/base/\"; cp '$build/codemp/game/jampgamex86_64.dylib' \"\$OPENJK_E2E_HOMEPATH/base/\"; cp '$build'/codemp/cgame/cgame*x86_64.dylib \"\$OPENJK_E2E_HOMEPATH/base/\"; '$binary' +set fs_basepath '$base' +set fs_homepath \"\$OPENJK_E2E_HOMEPATH\" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port \"\$OPENJK_E2E_CLIENT_PORT\" +set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 +set developer 1 +set g_gametype '$gametype' +exec splitqa/case.cfg"
	run_command "$case_name" "$command"
}

for mode in "${selected[@]}"; do
	case "$mode" in
		ffa|holocron|jedimaster)
			run_command "$mode-4p" \
				"'/bin/bash' '$root/tests/splitscreen/gameplay/modes/individual/client_case.sh' '$mode' 4"
			;;
		duel)
			run_cfg duel-4p "$root/tests/splitscreen/gameplay/modes/duel/duel_4p.cfg" 3
			;;
		powerduel)
			run_cfg powerduel-4p "$root/tests/splitscreen/gameplay/modes/duel/powerduel_4p.cfg" 4
			;;
		ctf|cty)
			run_command "$mode-4p" \
				"mkdir -p \"\$OPENJK_E2E_HOMEPATH/base\"; ln -sfn \"\$OPENJK_E2E_SCREENSHOTS\" \"\$OPENJK_E2E_HOMEPATH/base/screenshots\"; '$root/tests/splitscreen/gameplay/modes/ctf_cty/launch_game.sh' 4 '$mode'"
			;;
		team-ffa)
			team_cfg="$suite/team_ffa_4p.cfg"
			python3 "$root/tests/splitscreen/gameplay/modes/team_ffa/generate_cfg.py" 4 >"$team_cfg"
			run_cfg team-ffa-4p "$team_cfg" 6
			;;
		siege)
			siege_cfg="$suite/siege_4p.cfg"
			python3 "$root/tests/splitscreen/gameplay/modes/siege/generate_cfg.py" 4 >"$siege_cfg"
			run_cfg siege-4p "$siege_cfg" 7
			;;
	esac
done

test "$(shasum -a 256 "$binary" | awk '{print $1}')" = "$expected"
mode_env_tmp="$mode_env.tmp.$$"
awk -F = -v hash="$expected" '
	$1 != "after_sha256" { print }
	END { print "after_sha256=" hash }
' "$mode_env" >"$mode_env_tmp"
mv "$mode_env_tmp" "$mode_env"
if (( validate_complete_suite )); then
	python3 "$here/validate_modes.py" \
		"$root/tests/splitscreen/gameplay/results/certification/4p" \
		"$expected" "$run_id"
fi
