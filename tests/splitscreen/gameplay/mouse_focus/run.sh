#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/mouse_focus"
RESULTS="${OPENJK_MOUSE_FOCUS_RESULTS:-$ROOT/tests/splitscreen/gameplay/results/mouse_focus}"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
RUNNER="$ROOT/tests/splitscreen/gameplay/run_e2e.sh"
EXPECTED="${OPENJK_CERT_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"

mkdir -p "$RESULTS"
python3 "$ROOT/tests/splitscreen/gameplay/certification/verify_frozen_artifacts.py" \
	--expected-client-hash "$EXPECTED"
python3 "$HERE/generate.py"
player_counts=("${@:-2 3 4}")
failures=0
for players in ${player_counts[*]}; do
	command="mkdir -p \"\$OPENJK_E2E_HOMEPATH/base/splitqa\"; \"$ROOT/tests/splitscreen/install_assets.sh\" \"\$OPENJK_E2E_HOMEPATH\" >/dev/null; cp \"$HERE/mouse_focus_${players}p.cfg\" \"\$OPENJK_E2E_HOMEPATH/base/splitqa/\"; cp \"$BUILD/codemp/ui/uix86_64.dylib\" \"\$OPENJK_E2E_HOMEPATH/base/\"; cp \"$BUILD/codemp/game/jampgamex86_64.dylib\" \"\$OPENJK_E2E_HOMEPATH/base/\"; cp \"$BUILD\"/codemp/cgame/cgame*x86_64.dylib \"\$OPENJK_E2E_HOMEPATH/base/\"; ln -sfn \"\$OPENJK_E2E_SCREENSHOTS\" \"\$OPENJK_E2E_HOMEPATH/base/screenshots\"; \"$BIN\" +set fs_basepath \"$BASE\" +set fs_homepath \"\$OPENJK_E2E_HOMEPATH\" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port \"\$OPENJK_E2E_CLIENT_PORT\" +set net_qport \"\$OPENJK_E2E_QPORT\" +set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 +set com_introplayed 1 +set developer 1 +exec splitqa/mouse_focus_${players}p.cfg"
	set +e
	manifest="$("$RUNNER" --case "${players}p" --players "$players" --vm native \
		--timeout 120 --artifact-root "$RESULTS" --hash "$BIN" \
		--hash "$BUILD/codemp/ui/uix86_64.dylib" --client-command "$command")"
	status=$?
	set -e
	printf '%s\n' "$manifest" >"$RESULTS/${players}p.latest"
	if [[ -n "$manifest" && -f "$manifest" ]]; then
		if ! "$RUNNER" --validate-only "$manifest"; then
			status=1
		fi
	fi
	printf 'GP1-02 %sp runner exit=%s manifest=%s\n' "$players" "$status" "$manifest"
	if [[ "$status" -ne 0 ]]; then
		failures=$((failures + 1))
	fi
done
python3 "$HERE/analyze.py" --results "$RESULTS"
[[ "$failures" -eq 0 ]]
