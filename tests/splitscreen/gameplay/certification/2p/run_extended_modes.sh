#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
bin="$root/build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
expected="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
out="$root/tests/splitscreen/gameplay/results/certification/2p/extended-modes"
harness="$root/tests/splitscreen/gameplay/run_e2e.sh"
build="$root/build-x86_64"
base="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
run_id="${OPENJK_CERT_RUN_ID:-$(date -u +%Y%m%dT%H%M%SZ)-$$-$RANDOM}"
ledger="$root/tests/splitscreen/gameplay/results/certification/2p/local-suite/$run_id/modes.tsv"

test "$(shasum -a 256 "$bin" | awk '{print $1}')" = "$expected"
[[ "$run_id" =~ ^[A-Za-z0-9._-]+$ ]] || exit 2
mkdir -p "$out" "$(dirname "$ledger")"
touch "$ledger"

record_manifest() {
	local case_name="$1" manifest="$2"
	local ledger_tmp="$ledger.tmp.$$"
	test -s "$manifest"
	awk -F '\t' -v target="$case_name" '$1 != target' "$ledger" >"$ledger_tmp"
	mv "$ledger_tmp" "$ledger"
	printf '%s\t%s\t%s\n' "$case_name" "$manifest" \
		"$(shasum -a 256 "$manifest" | awk '{print $1}')" >>"$ledger"
}

run_cfg() {
	local case_name="$1" cfg="$2" gametype="${3:-}"
	local command
	command="mkdir -p \"\$OPENJK_E2E_HOMEPATH/base/splitqa\"; '$root/tests/splitscreen/install_assets.sh' \"\$OPENJK_E2E_HOMEPATH\" >/dev/null; ln -sfn \"\$OPENJK_E2E_SCREENSHOTS\" \"\$OPENJK_E2E_HOMEPATH/base/screenshots\"; cp '$cfg' \"\$OPENJK_E2E_HOMEPATH/base/splitqa/case.cfg\"; cp '$build/codemp/ui/uix86_64.dylib' \"\$OPENJK_E2E_HOMEPATH/base/\"; cp '$build/codemp/game/jampgamex86_64.dylib' \"\$OPENJK_E2E_HOMEPATH/base/\"; cp '$build'/codemp/cgame/cgame*x86_64.dylib \"\$OPENJK_E2E_HOMEPATH/base/\"; '$bin' +set fs_basepath '$base' +set fs_homepath \"\$OPENJK_E2E_HOMEPATH\" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port \"\$OPENJK_E2E_CLIENT_PORT\" +set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 +set developer 1 ${gametype:+ +set g_gametype $gametype} +exec splitqa/case.cfg"
	local output manifest
	output="$("$harness" --case "$case_name" --players 2 --vm native --timeout 180 \
		--artifact-root "$out" --hash "$bin" --client-command "$command"
	)"
	manifest="$(printf '%s\n' "$output" | tail -1)"
	record_manifest "$case_name" "$manifest"
}

only="${GP501_EXTENDED_ONLY:-all}"
case "$only" in
	all|combat|objectives|duel|powerduel|ctf|cty|team-ffa|siege) ;;
	*) echo "unknown GP501_EXTENDED_ONLY cell: $only" >&2; exit 2 ;;
esac
if [[ "$only" == all || "$only" == combat || "$only" == duel ]]; then
	run_cfg duel-2p "$root/tests/splitscreen/gameplay/modes/duel/duel_2p.cfg"
fi
if [[ "$only" == all || "$only" == combat || "$only" == powerduel ]]; then
	run_cfg powerduel-2p "$root/tests/splitscreen/gameplay/modes/duel/powerduel_2p.cfg"
fi

if [[ "$only" == all || "$only" == objectives ]]; then
	for mode in ctf cty; do
		gametype=8; test "$mode" = ctf || gametype=9
		output="$("$harness" --case "$mode-2p" --players 2 --vm native --timeout 180 \
			--artifact-root "$out" --hash "$bin" \
			--client-command "'$root/tests/splitscreen/gameplay/modes/ctf_cty/launch_game.sh' 2 $mode")"
		manifest="$(printf '%s\n' "$output" | tail -1)"
		record_manifest "$mode-2p" "$manifest"
	done
elif [[ "$only" == ctf || "$only" == cty ]]; then
	gametype=8; test "$only" = ctf || gametype=9
	output="$("$harness" --case "$only-2p" --players 2 --vm native --timeout 180 \
		--artifact-root "$out" --hash "$bin" \
		--client-command "'$root/tests/splitscreen/gameplay/modes/ctf_cty/launch_game.sh' 2 $only")"
	manifest="$(printf '%s\n' "$output" | tail -1)"
	record_manifest "$only-2p" "$manifest"
fi

# Run the owned, contract-tested 2p Team FFA fixture through the same harness.
if [[ "$only" == all || "$only" == combat || "$only" == team-ffa ]]; then
	team_cfg="$root/tests/splitscreen/gameplay/certification/2p/team_ffa_2p.cfg"
	run_cfg team-ffa-2p "$team_cfg" 6
fi

if [[ "$only" == all || "$only" == objectives || "$only" == siege ]]; then
	siege_cfg="$(dirname "$ledger")/siege_2p.cfg"
	python3 "$root/tests/splitscreen/gameplay/modes/siege/generate_cfg.py" 2 >"$siege_cfg"
	run_cfg siege-2p "$siege_cfg" 7
fi

test "$(shasum -a 256 "$bin" | awk '{print $1}')" = "$expected"
