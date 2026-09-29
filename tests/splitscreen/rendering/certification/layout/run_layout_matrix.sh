#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
HERE="$ROOT/tests/splitscreen/rendering/certification/layout"
BUILD="$ROOT/build-x86_64"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
RUN_ROOT="${OPENJK_LAYOUT_RUN_ROOT:-/private/tmp/openjk-layout-certification}"
ARTIFACTS="$HERE/artifacts"

mkdir -p "$ARTIFACTS"

for aspect in 16x9 4x3; do
	if [[ "$aspect" == 16x9 ]]; then width=1280; height=720; else width=1024; height=768; fi
	for players in 2 3 4; do
		for layout in horizontal vertical; do
			case_name="layout_${aspect}_${players}p_${layout}"
			layout_value=0
			[[ "$layout" == vertical ]] && layout_value=1
			homepath="$RUN_ROOT/$case_name"
			aspect_offset=0
			[[ "$aspect" == 4x3 ]] && aspect_offset=1
			port=$((31000 + players * 100 + layout_value * 10 + aspect_offset))
			mkdir -p "$homepath/base/splitqa" "$homepath/base/screenshots"
			"$ROOT/tests/splitscreen/install_assets.sh" "$homepath" >/dev/null
			cp "$BUILD/codemp/ui/uix86_64.dylib" "$homepath/base/"
			cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$homepath/base/"
			cp "$BUILD"/codemp/cgame/cgame*x86_64.dylib "$homepath/base/"
			sed -e "s/@PLAYERS@/$players/g" -e "s/@LAYOUT@/$layout_value/g" \
				-e "s/@CASE@/$case_name/g" "$HERE/layout_case.cfg.in" \
				> "$homepath/base/splitqa/$case_name.cfg"

			echo "==> $case_name port=$port home=$homepath"
			"$BIN" +set fs_basepath "$BASEPATH" +set fs_homepath "$homepath" \
				+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 +set net_port "$port" \
				+set r_fullscreen 0 +set r_mode -1 +set r_customwidth "$width" \
				+set r_customheight "$height" +set s_initsound 0 +set in_joystick 0 \
				+exec "splitqa/$case_name.cfg" > "$ARTIFACTS/$case_name.log" 2>&1

			for state in gameplay menu; do
				png="$homepath/base/screenshots/${case_name}_${state}.png"
				cp "$png" "$ARTIFACTS/"
			done
			python3 "$HERE/certify_layout.py" "$ARTIFACTS/${case_name}_gameplay.png" \
				"$players" "$layout" "$width" "$height" \
				> "$ARTIFACTS/$case_name.oracle.txt"
		done
	done
done

echo "matrix complete: $ARTIFACTS"
