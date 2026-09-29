#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/modes/duel"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/modes/duel"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
EXPECTED=737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13
actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$EXPECTED" ]] || { echo "frozen binary mismatch: $actual" >&2; exit 2; }
mkdir -p "$RESULTS"
python3 "$HERE/generate.py"
cases=("${@:-duel-2p duel-3p duel-4p powerduel-2p powerduel-3p powerduel-4p}")
for case_name in ${cases[*]}; do
	mode="${case_name%-*p}"; players="${case_name##*-}"; players="${players%p}"
	cfg="${mode}_${players}p.cfg"
	command="mkdir -p \"\$OPENJK_E2E_HOMEPATH/base/splitqa\"; \"$ROOT/tests/splitscreen/install_assets.sh\" \"\$OPENJK_E2E_HOMEPATH\" >/dev/null; cp \"$HERE/$cfg\" \"\$OPENJK_E2E_HOMEPATH/base/splitqa/\"; cp \"$BUILD/codemp/ui/uix86_64.dylib\" \"\$OPENJK_E2E_HOMEPATH/base/\"; cp \"$BUILD/codemp/game/jampgamex86_64.dylib\" \"\$OPENJK_E2E_HOMEPATH/base/\"; cp \"$BUILD\"/codemp/cgame/cgame*x86_64.dylib \"\$OPENJK_E2E_HOMEPATH/base/\"; ln -sfn \"\$OPENJK_E2E_SCREENSHOTS\" \"\$OPENJK_E2E_HOMEPATH/base/screenshots\"; \"$BIN\" +set fs_basepath \"$BASE\" +set fs_homepath \"\$OPENJK_E2E_HOMEPATH\" +set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port \"\$OPENJK_E2E_CLIENT_PORT\" +set net_qport \"\$OPENJK_E2E_QPORT\" +set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 +set in_joystick 1 +set com_introplayed 1 +set developer 1 +exec splitqa/$cfg"
	manifest="$("$ROOT/tests/splitscreen/gameplay/run_e2e.sh" --case "$case_name" \
		--players "$players" --vm native --timeout 120 --artifact-root "$RESULTS" \
		--hash "$BIN" --hash "$BUILD/codemp/ui/uix86_64.dylib" \
		--client-command "$command")"
	printf '%s\n' "$manifest" >"$RESULTS/$case_name.latest"
	"$ROOT/tests/splitscreen/gameplay/run_e2e.sh" --validate-only "$manifest"
	echo "$case_name PASS $manifest"
done
python3 "$HERE/analyze.py" --results "$RESULTS"
