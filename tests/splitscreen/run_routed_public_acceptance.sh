#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-x86_64}"
BIN="${OPENJK_BIN:-$BUILD_DIR/openjk.x86_64.app/Contents/MacOS/openjk.x86_64}"
EXPECTED_SHA256="${OPENJK_EXPECTED_SHA256:-737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13}"
BASEPATH="${OPENJK_BASEPATH:-/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents}"
PLAYERS="${1:-}"

if [[ ! "$PLAYERS" =~ ^[234]$ ]]; then
	echo "usage: $0 <2|3|4>" >&2
	exit 2
fi

QA_ROOT="${OPENJK_PUBLIC_QA_ROOT:-$(mktemp -d "/private/tmp/openjk-public-${PLAYERS}p.XXXXXX")}"
NET_PORT="${OPENJK_PUBLIC_NET_PORT:-$((29300 + PLAYERS))}"
CFG="routed_public_${PLAYERS}p_acceptance.cfg"
CFG_SOURCE="$ROOT/tests/splitscreen/cfg/$CFG"
CFG_DIR="$QA_ROOT/base/splitqa"
LOG="$QA_ROOT/public-${PLAYERS}p.stdout.log"
CONFIGURED_PUBLIC_SERVER="$(awk '$1 == "addFavorite" { print $2; exit }' "$CFG_SOURCE")"
PUBLIC_SERVER="${OPENJK_PUBLIC_TARGET:-$CONFIGURED_PUBLIC_SERVER}"

for required in "$BIN" "$CFG_SOURCE" "$BUILD_DIR/codemp/ui/uix86_64.dylib"; do
	if [[ ! -f "$required" ]]; then
		echo "Missing public split-screen QA artifact: $required" >&2
		exit 2
	fi
done
actual_sha256="$(shasum -a 256 "$BIN" | awk '{print $1}')"
if [[ "$actual_sha256" != "$EXPECTED_SHA256" ]]; then
	echo "Frozen public candidate mismatch: $actual_sha256" >&2
	exit 2
fi
if [[ -z "$PUBLIC_SERVER" ]]; then
	echo "Public acceptance config does not seed a Favorite server" >&2
	exit 2
fi
if [[ ! "$PUBLIC_SERVER" =~ ^[0-9]{1,3}(\.[0-9]{1,3}){3}:[0-9]{1,5}$ ]]; then
	echo "Public acceptance target must be a numeric IPv4 endpoint: $PUBLIC_SERVER" >&2
	exit 2
fi

# Character, Force-profile, party setup, browser selection, and Join must all
# come from routed visible UI input. The server address is only seeded as a
# browser Favorite so that the stock Internet browser can display it.
if rg -q \
	'splitscreen_setup|splitnet_applyprofile|set (ui_splitScreenP[1234](Model|ForcePowers)|model|forcepowers)|^(connect|splitnet_party_connect|devmap)[[:space:]]' \
	"$CFG_SOURCE"; then
	echo "Public acceptance must select profiles and join through the visible UI" >&2
	exit 2
fi

if [[ "${OPENJK_PUBLIC_VALIDATE_ONLY:-0}" != 1 ]]; then
	mkdir -p "$CFG_DIR" "$QA_ROOT/base/screenshots"
	"$ROOT/tests/splitscreen/install_assets.sh" "$QA_ROOT" >/dev/null
	# The fixture also records `serverstatus` for the seeded endpoint. Replace
	# every occurrence so an explicitly gated alternate target cannot connect
	# successfully and then fail validation against stale default-server data.
	sed "s|$CONFIGURED_PUBLIC_SERVER|$PUBLIC_SERVER|g" \
		"$CFG_SOURCE" >"$CFG_DIR/$CFG"
	cp "$BUILD_DIR/codemp/ui/uix86_64.dylib" "$QA_ROOT/base/"
	cp "$BUILD_DIR"/codemp/cgame/cgame*x86_64.dylib "$QA_ROOT/base/"

	"$BIN" \
		+set fs_basepath "$BASEPATH" \
		+set fs_homepath "$QA_ROOT" \
		+set vm_game 0 \
		+set vm_cgame 0 \
		+set vm_ui 0 \
		+set net_port "$NET_PORT" \
		+set password "" \
		+set r_fullscreen 0 \
		+set r_mode 3 \
		+set s_initsound 0 \
		+set in_joystick 1 \
		+set logfile 2 \
		+set developer 1 \
		+set com_introplayed 1 \
		+exec "splitqa/$CFG" >"$LOG" 2>&1
elif [[ ! -s "$LOG" ]]; then
	echo "Missing public ${PLAYERS}-player log for validation: $LOG" >&2
	exit 2
fi

if rg -n \
	'Split(UI|Input|NetLifecycle|NetStat|Profile)Assert: FAIL|Too Many players with the same IP|@@@TOO_MANY_INFO|recursive error|segmentation|assertion failed' \
	"$LOG" "$QA_ROOT/base/qconsole.log"; then
	echo "Public ${PLAYERS}-player acceptance emitted a failure marker" >&2
	exit 1
fi

required_patterns=(
	"SplitUIAssert: PASS cvar=ui_splitScreenPlayerCount expected=$PLAYERS actual=$PLAYERS"
	"SplitUIAssert: PASS cvar=ui_splitScreenSessionType expected=server actual=server"
	"SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=join_pending actual=join_pending"
	"SplitUIAssert: PASS cvar=ui_splitScreenPartyState expected=active actual=active"
	"SplitNet party: all $PLAYERS local players active"
	"SplitNet party: state=active players=$PLAYERS target=$PUBLIC_SERVER"
	"Public${PLAYERS}_P1"
	"Server ($PUBLIC_SERVER)"
)
for player in $(seq 1 "$PLAYERS"); do
	required_patterns+=(
		"SplitNetLifecycleAssert: PASS player=$player expected=ALIVE actual=ALIVE"
		"Public${PLAYERS}_P${player}"
	)
done
for pattern in "${required_patterns[@]}"; do
	if ! rg -Fq "$pattern" "$LOG" "$QA_ROOT/base/qconsole.log"; then
		echo "Missing public ${PLAYERS}-player evidence: $pattern" >&2
		exit 1
	fi
done

# Every routed screenshot instruction must have produced a non-empty frame.
while read -r frame; do
	if ! find "$QA_ROOT" -type f -path "*/screenshots/$frame.png" -size +0c -print -quit | rg -q .; then
		echo "Missing public ${PLAYERS}-player screenshot: $frame.png" >&2
		exit 1
	fi
done < <(awk '$1 == "screenshot_png" { print $2 }' "$CFG_SOURCE")

gameplay_frame="$(find "$QA_ROOT" -type f -path "*/screenshots/public${PLAYERS}_gameplay_*alive.png" -size +0c -print -quit)"
python3 "$ROOT/tests/splitscreen/assert_screenshot.py" "$gameplay_frame" "$PLAYERS"
final_sha256="$(shasum -a 256 "$BIN" | awk '{print $1}')"
if [[ "$final_sha256" != "$EXPECTED_SHA256" ]]; then
	echo "Frozen public candidate changed during run: $final_sha256" >&2
	exit 2
fi

echo "passed: $PLAYERS routed split-screen clients joined and played on live public server $PUBLIC_SERVER"
echo "candidate: $final_sha256"
echo "root: $QA_ROOT"
echo "log: $LOG"
echo "screenshots:"
find "$QA_ROOT" -type d -name screenshots -print
