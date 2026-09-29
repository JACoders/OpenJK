#!/usr/bin/env bash
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
players="${1:?player count required}"
home="${OPENJK_E2E_HOMEPATH:?Phase 0 harness homepath required}"

case "$players" in
	2)
		join='splitnet_connect 2 localhost'
		team='splitnet_cmd 2 team free'
		alive='splitnet_assert_lifecycle 2 ALIVE'
		;;
	3)
		join='splitnet_connect 2 localhost; splitnet_connect 3 localhost'
		team='splitnet_cmd 2 team free; splitnet_cmd 3 team free'
		alive='splitnet_assert_lifecycle 2 ALIVE; splitnet_assert_lifecycle 3 ALIVE'
		;;
	4)
		join='splitnet_connect 2 localhost; splitnet_connect 3 localhost; splitnet_connect 4 localhost'
		team='splitnet_cmd 2 team free; splitnet_cmd 3 team free; splitnet_cmd 4 team free'
		alive='splitnet_assert_lifecycle 2 ALIVE; splitnet_assert_lifecycle 3 ALIVE; splitnet_assert_lifecycle 4 ALIVE'
		;;
	*) echo "players must be 2, 3, or 4" >&2; exit 2 ;;
esac

mkdir -p "$home/base/splitqa" "$home/base/screenshots"
"$ROOT/tests/splitscreen/install_assets.sh" "$home" >/dev/null
cp "$HERE"/modal_*.cfg "$home/base/splitqa/"
cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$home/base/"
cp "$BUILD_DIR"/codemp/cgame/cgame*x86_64.dylib "$home/base/"
cp "$BUILD_DIR/codemp/game/jampgamex86_64.dylib" "$home/base/"

set +e
OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT="${OPENJK_VIRTUAL_GAMEPAD_PORT:?}" \
"$BIN" \
	+set fs_basepath "$BASEPATH" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "$OPENJK_E2E_CLIENT_PORT" +set r_fullscreen 0 +set r_mode 3 \
	+set s_initsound 0 +set in_joystick 1 +set developer 1 +set com_introplayed 1 \
	+set ui_splitScreenPlayerCount "$players" +set modal_players "$players" \
	+set modal_join "$join" +set modal_team "$team" +set modal_assert_alive "$alive" \
	+set modal_assert_count "splitui_assert ui_splitScreenPlayerCount $players" \
	+set modal_sequence "exec splitqa/modal_sequence_${players}.cfg" \
	+exec splitqa/modal_common.cfg
status=$?
set -e

find "$home" -type f -path '*/screenshots/modal_*.png' -exec cp {} "$OPENJK_E2E_SCREENSHOTS/" \;
find "$home" -type f -name 'modal_*_console.txt' -exec cp {} "$OPENJK_E2E_RUN_DIR/" \;
exit "$status"
