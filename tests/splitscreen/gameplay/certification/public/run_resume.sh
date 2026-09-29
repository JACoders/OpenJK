#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../../../../.." && pwd)"
BUILD="$ROOT/build-x86_64"
BIN="$BUILD/openjk.x86_64.app/Contents/MacOS/openjk.x86_64"
BASE="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
EXPECTED="737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13"
players="${1:-}"
home="${2:-}"

[[ "$players" == 2 || "$players" == 4 ]] || {
	echo "usage: $0 <2|4> <persisted-public-home>" >&2
	exit 2
}
[[ -d "$home/base" ]] || { echo "missing persisted public home: $home" >&2; exit 2; }
actual="$(shasum -a 256 "$BIN" | awk '{print $1}')"
[[ "$actual" == "$EXPECTED" ]] || { echo "frozen hash mismatch: $actual" >&2; exit 2; }

cfg="resume_${players}p.cfg"
mkdir -p "$home/base/splitqa" "$home/base/screenshots"
cp "$ROOT/tests/splitscreen/gameplay/certification/public/$cfg" "$home/base/splitqa/"
log="$home/public-${players}p-resume.log"
"$BIN" \
	+set fs_basepath "$BASE" +set fs_homepath "$home" \
	+set vm_game 0 +set vm_cgame 0 +set vm_ui 0 \
	+set net_port "$((29400 + players))" +set password "" \
	+set r_fullscreen 0 +set r_mode 3 +set s_initsound 0 \
	+set in_joystick 1 +set developer 1 +set com_introplayed 1 \
	+exec "splitqa/$cfg" >"$log" 2>&1

if rg -n 'Assert: FAIL|Too Many players with the same IP|recursive error|segmentation|assertion failed' "$log"; then
	echo "public ${players}p continuation failed" >&2
	exit 1
fi
for player in $(seq 1 "$players"); do
	rg -q "SplitNetLifecycleAssert: PASS player=$player expected=ALIVE actual=ALIVE" "$log"
done
test -s "$home/base/screenshots/public${players}_resumed_active.png"
python3 "$ROOT/tests/splitscreen/assert_screenshot.py" \
	"$home/base/screenshots/public${players}_resumed_active.png" "$players"
[[ "$(shasum -a 256 "$BIN" | awk '{print $1}')" == "$EXPECTED" ]]
echo "passed: public ${players}p continuation active after server rollover"
