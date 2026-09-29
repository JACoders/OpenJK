#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
bin="$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
build="$root/build-x86_64"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
base="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
results="$root/tests/splitscreen/gameplay/results/certification/3p/modes"
harness="$root/tests/splitscreen/gameplay/run_e2e.sh"
run_id="${OPENJK_CERT_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM}"
ledger="$root/tests/splitscreen/gameplay/results/certification/3p/local-suite/$run_id/modes.tsv"

[[ "$run_id" =~ ^[A-Za-z0-9._-]+$ ]] || exit 2
test "$(shasum -a 256 "$bin" | awk '{print $1}')" = "$expected"
mkdir -p "$results" "$(dirname "$ledger")"
touch "$ledger"

record() {
	local case_name="$1" manifest="$2"
	local ledger_tmp="$ledger.tmp.$$"
	test -s "$manifest"
	awk -F '\t' -v target="$case_name" '$1 != target' "$ledger" >"$ledger_tmp"
	mv "$ledger_tmp" "$ledger"
	printf '%s\t%s\t%s\n' "$case_name" "$manifest" \
		"$(shasum -a 256 "$manifest" | awk '{print $1}')" >>"$ledger"
}

run_command() {
	local case_name="$1" command="$2" output manifest
	output="$("$harness" --case "$case_name" --players 3 --vm native --timeout 210 \
		--artifact-root "$results" --hash "$bin" --client-command "$command")"
	manifest="$(printf '%s\n' "$output" | tail -1)"
	record "$case_name" "$manifest"
}

run_cfg() {
	local case_name="$1" cfg="$2" gametype="${3:-}" command
	command="mkdir -p \"\$OPENJK_E2E_HOMEPATH/base/splitqa\"; '$root/tests/splitscreen/install_assets.sh' \"\$OPENJK_E2E_HOMEPATH\" >/dev/null; ln -sfn \"\$OPENJK_E2E_SCREENSHOTS\" \"\$OPENJK_E2E_HOMEPATH/base/screenshots\"; cp '$cfg' \"\$OPENJK_E2E_HOMEPATH/base/splitqa/case.cfg\"; cp '$build/codemp/ui/uix86_64.dylib' \"\$OPENJK_E2E_HOMEPATH/base/\"; cp '$build/codemp/game/jampgamex86_64.dylib' \"\$OPENJK_E2E_HOMEPATH/base/\"; cp '$build'/codemp/cgame/cgame*x86_64.dylib \"\$OPENJK_E2E_HOMEPATH/base/\"; '$bin' +set fs_basepath '$base' +set fs_homepath \"\$OPENJK_E2E_HOMEPATH\" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port \"\$OPENJK_E2E_CLIENT_PORT\" +set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 +set developer 1 ${gametype:+ +set g_gametype $gametype} +exec splitqa/case.cfg"
	run_command "$case_name" "$command"
}

canonical_modes=(ffa holocron jedimaster duel powerduel ctf cty team-ffa siege)
only="${GP502_MODES_ONLY:-all}"
case "$only" in
	all) selected_modes=("${canonical_modes[@]}") ;;
	ffa|holocron|jedimaster|duel|powerduel|ctf|cty|team-ffa|siege)
		selected_modes=("$only")
		;;
	*)
		echo "unknown GP502_MODES_ONLY cell: $only" >&2
		exit 2
		;;
esac

for mode in "${selected_modes[@]}"; do
	case "$mode" in
		ffa|holocron|jedimaster)
			run_command "$mode-3p" \
				"'/bin/bash' '$root/tests/splitscreen/gameplay/certification/3p/mode_client.sh' '$mode'"
			;;
		duel)
			run_cfg duel-3p "$root/tests/splitscreen/gameplay/modes/duel/duel_3p.cfg" 3
			;;
		powerduel)
			run_cfg powerduel-3p \
				"$root/tests/splitscreen/gameplay/modes/duel/powerduel_3p.cfg" 4
			;;
		ctf|cty)
			run_command "$mode-3p" \
				"'$root/tests/splitscreen/gameplay/modes/ctf_cty/launch_game.sh' 3 '$mode'"
			;;
		team-ffa)
			team_cfg="$(dirname "$ledger")/team_ffa_3p.cfg"
			python3 "$root/tests/splitscreen/gameplay/modes/team_ffa/generate_cfg.py" 3 \
				>"$team_cfg"
			run_cfg team-ffa-3p "$team_cfg" 6
			;;
		siege)
			siege_cfg="$(dirname "$ledger")/siege_3p.cfg"
			python3 "$root/tests/splitscreen/gameplay/modes/siege/generate_cfg.py" 3 \
				>"$siege_cfg"
			run_cfg siege-3p "$siege_cfg" 7
			;;
	esac
done

test "$(shasum -a 256 "$bin" | awk '{print $1}')" = "$expected"
