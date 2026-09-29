#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BIN="$ROOT/tests/splitscreen/external/macos_input_sim"
ENTITLEMENTS="$ROOT/tests/splitscreen/external/macos_input_sim.entitlements"
IDENTITY="${OPENJK_INPUT_SIM_SIGN_IDENTITY:--}"

if [[ ! -x "$BIN" ]]; then
	"$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null
fi

codesign --force --sign "$IDENTITY" --entitlements "$ENTITLEMENTS" "$BIN"
codesign --display --entitlements - "$BIN"
