#!/usr/bin/env bash
set -euo pipefail

PAYLOAD="${1:?usage: audit_macos_arm64.sh /path/to/OpenJK-SplitScreen-arm64}"
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

required=(
	"openjk.arm64.app/Contents/MacOS/openjk.arm64"
	"openjk.arm64.app/Contents/MacOS/rd-vanilla_arm64.dylib"
	"openjk.arm64.app/Contents/MacOS/rd-rend2_arm64.dylib"
	"openjk_sp.arm64.app/Contents/MacOS/openjk_sp.arm64"
	"openjk_sp.arm64.app/Contents/MacOS/rdsp-vanilla_arm64.dylib"
	"openjkded.arm64"
	"base/jampgamearm64.dylib"
	"base/cgamearm64.dylib"
	"base/cgame2arm64.dylib"
	"base/cgame3arm64.dylib"
	"base/cgame4arm64.dylib"
	"base/uiarm64.dylib"
	"base/ui/jamp/splitscreen_start.menu"
	"base/ui/jamp/splitscreen_players.menu"
	"tools/macos_input_sim"
	"BUILD-MANIFEST.txt"
	"LICENSE.txt"
	"SDL2-LICENSE.txt"
	"docs/PLAYER-GUIDE.md"
	"docs/NETWORK-GUIDE.md"
	"install.sh"
	"SHA256SUMS"
)
for path in "${required[@]}"; do
	[[ -s "$PAYLOAD/$path" ]] || { echo "Missing release file: $path" >&2; exit 1; }
done

binaries=()
while IFS= read -r -d '' binary; do
	binaries[${#binaries[@]}]="$binary"
done < <(find "$PAYLOAD" -type f \( -name '*.dylib' -o -name 'openjk.arm64' -o -name 'openjk_sp.arm64' -o -name 'openjkded.arm64' -o -name 'macos_input_sim' \) -print0)
for binary in "${binaries[@]}"; do
	description="$(file "$binary")"
	[[ $description == *"Mach-O 64-bit"* && $description == *"arm64"* ]] || {
		echo "Non-ARM64 release binary: $description" >&2
		exit 1
	}
	if otool -L "$binary" | grep -Eq '/opt/homebrew|/usr/local|/MacPorts|build-arm64|build-release-cache'; then
		echo "Developer-machine dependency in $binary" >&2
		otool -L "$binary" >&2
		exit 1
	fi
	while IFS= read -r dependency; do
		case "$dependency" in
			/System/*|/usr/lib/*) ;;
			"@rpath/$(basename "$binary")") ;;
			*) echo "Unapproved dynamic dependency in $binary: $dependency" >&2; exit 1 ;;
		esac
	done < <(otool -L "$binary" | tail -n +2 | awk '{print $1}')
	minos="$(otool -l "$binary" | awk '/LC_BUILD_VERSION/{found=1;next} found&&/minos/{print $2; exit}')"
	[[ -n $minos ]] || { echo "Missing LC_BUILD_VERSION in $binary" >&2; exit 1; }
	min_major=${minos%%.*}
	min_minor=${minos#*.}
	min_minor=${min_minor%%.*}
	if (( min_major > 11 || (min_major == 11 && min_minor > 0) )); then
		echo "$binary requires macOS $minos; the package target is macOS 11.0." >&2
		exit 1
	fi
done

if find "$PAYLOAD" -type f \( -name '*.cfg' -o -name '*.log' -o -name '*.png' \) | grep -q .; then
	echo "Release contains generated configuration, logs, or screenshots." >&2
	exit 1
fi
(
	cd "$PAYLOAD"
	shasum -a 256 -c SHA256SUMS >/dev/null
)
plutil -lint "$PAYLOAD/openjk.arm64.app/Contents/Info.plist" >/dev/null
plutil -lint "$PAYLOAD/openjk_sp.arm64.app/Contents/Info.plist" >/dev/null

echo "ARM64 package audit passed: $PAYLOAD"
echo "audited binaries: ${#binaries[@]}"
