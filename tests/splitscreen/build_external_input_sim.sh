#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
SRC="$ROOT/tests/splitscreen/external/macos_input_sim.c"
OUT="$ROOT/tests/splitscreen/external/macos_input_sim"

mkdir -p "$(dirname "$OUT")"
MIN_MACOS="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
clang "$SRC" -o "$OUT" -mmacosx-version-min="$MIN_MACOS" \
	-framework ApplicationServices -framework CoreGraphics -framework IOKit
echo "$OUT"
