#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
DEST="${1:-${OPENJK_HOMEPATH:-$ROOT/runtime-home}}"
SOURCE="$ROOT/assets/splitscreen/base"

if [[ ! -d "$SOURCE" ]]; then
	echo "Missing split-screen assets: $SOURCE" >&2
	exit 2
fi

mkdir -p "$DEST/base"
cp -R "$SOURCE/." "$DEST/base/"

echo "$DEST/base"
