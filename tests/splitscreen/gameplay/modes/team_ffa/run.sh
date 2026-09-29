#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/gameplay/modes/team_ffa"
RESULTS="$ROOT/tests/splitscreen/gameplay/results/modes/team_ffa"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
EXPECTED="737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$EXPECTED" ]] || { echo "frozen binary mismatch: $actual" >&2; exit 2; }
mkdir -p "$RESULTS"

for players in 2 3 4; do
	home="$(mktemp -d "/private/tmp/openjk-gp2-team-${players}.XXXXXX")"
	cfg="$home/base/splitqa/team_ffa.cfg"
	log="$RESULTS/team-${players}p.log"
	mkdir -p "$home/base/splitqa" "$home/base/screenshots"
	"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
	python3 "$HERE/generate_cfg.py" "$players" >"$cfg"
	cp "$BUILD/codemp/ui/uix86_64.dylib" "$home/base/"
	cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$home/base/"
	cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
	"$BIN" +set fs_basepath "$BASE" +set fs_homepath "$home" \
		+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
		+set net_port "$((29800 + players))" +set r_fullscreen 0 \
		+set s_initsound 0 +set in_joystick 1 \
		+exec splitqa/team_ffa.cfg >"$log" 2>&1
	mkdir -p "$RESULTS/${players}p"
	cp "$home"/base/screenshots/gp2_*.png "$RESULTS/${players}p/" 2>/dev/null || true
	cp "$home/base/gp2_team_ffa_console.txt" "$RESULTS/${players}p/" 2>/dev/null || true
done

python3 "$HERE/analyze.py" --logs \
	"$RESULTS/team-2p.log" "$RESULTS/team-3p.log" "$RESULTS/team-4p.log" \
	--screenshot-oracle "$ROOT/tests/splitscreen/assert_screenshot.py" \
	--output "$RESULTS/matrix.json"

manifest="$RESULTS/artifact-integrity-manifest.tsv"
sha256() { shasum -a 256 "$1" | awk '{print $1}'; }
{
	printf 'format\topenjk-e2e-v1\nstatus\tpassed\ncase\tGP2-03-artifact-integrity-only\n'
	printf 'discovery_result\tacceptance_not_met\n'
	for input in "$BIN" "$BUILD/codemp/ui/uix86_64.dylib" \
		"$BUILD/codemp/game/jampgamex86_64.dylib" "$BUILD/codemp/cgame/cgamex86_64.dylib"; do
		printf 'hash\t%s\t%s\n' "$input" "$(sha256 "$input")"
	done
	for artifact in "$RESULTS"/team-*p.log "$RESULTS/matrix.json" "$RESULTS"/*p/*.png; do
		[[ -f "$artifact" ]] && printf 'artifact\t%s\t%s\n' "$artifact" "$(sha256 "$artifact")"
	done
} >"$manifest"
"$ROOT/tests/splitscreen/gameplay/run_e2e.sh" --validate-only "$manifest"
python3 "$HERE/validate.py"
