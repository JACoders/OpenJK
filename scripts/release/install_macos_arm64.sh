#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEFAULT_GAME="/Users/$USER/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents"
GAME_CONTENTS="${1:-${OPENJK_GAME_CONTENTS:-$DEFAULT_GAME}}"
OPENJK_HOME="${OPENJK_HOME:-$HOME/Library/Application Support/OpenJK}"

if [[ ! -f "$GAME_CONTENTS/base/assets0.pk3" ]]; then
	echo "Jedi Academy data was not found at: $GAME_CONTENTS" >&2
	echo "Pass the SWJKJA.app/Contents path as the first argument." >&2
	exit 2
fi

mkdir -p "$OPENJK_HOME/base"
cp -R "$ROOT/base/." "$OPENJK_HOME/base/"
cp -R "$ROOT/openjk.arm64.app" "$GAME_CONTENTS/"
cp -R "$ROOT/openjk_sp.arm64.app" "$GAME_CONTENTS/"
cp "$ROOT/openjkded.arm64" "$GAME_CONTENTS/"

echo "Installed ARM64 apps in: $GAME_CONTENTS"
echo "Installed split-screen modules and menus in: $OPENJK_HOME/base"
echo "Original Jedi Academy PK3 files were not modified."
