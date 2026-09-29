#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
BUILD="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
DED="${OPENJK_DED_BIN:-$BUILD/openjkded.x86_64}"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
HOME_PATH="${OPENJK_E2E_RUN_DIR:?Phase 0 harness run directory required}/server-home"

mkdir -p "$HOME_PATH/base"
"$ROOT/tests/splitscreen/install_assets.sh" "$HOME_PATH" >/dev/null
cp "$BUILD/codemp/game/jampgamex86_64.dylib" "$HOME_PATH/base/"

exec "$DED" \
	+set fs_basepath "$BASE" +set fs_homepath "$HOME_PATH" +set vm_game 0 \
	+set dedicated 1 +set net_port "${OPENJK_E2E_SERVER_PORT:?}" +set sv_pure 0 \
	+set sv_maxclients 8 +set g_maxConnPerIP 8 +set g_password "" \
	+set g_antiFakePlayer 0 +map mp/ffa3
