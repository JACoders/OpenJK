#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD_DIR="${OPENJK_BUILD_DIR:-$ROOT/build-arm64-package}"
DIST_DIR="${OPENJK_DIST_DIR:-$ROOT/dist}"
CACHE_DIR="${OPENJK_RELEASE_CACHE:-$ROOT/build-release-cache}"
JOBS="${OPENJK_BUILD_JOBS:-8}"
MIN_MACOS="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
SDL_VERSION=2.30.12
SDL_ARCHIVE="SDL2-$SDL_VERSION.tar.gz"
SDL_URL="https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/$SDL_ARCHIVE"
SDL_SHA256=ac356ea55e8b9dd0b2d1fa27da40ef7e238267ccf9324704850d5d47375b48ea
SDL_SOURCE="$CACHE_DIR/src/SDL2-$SDL_VERSION"
SDL_BUILD="$CACHE_DIR/build/SDL2-$SDL_VERSION-arm64"
SDL_PREFIX="$CACHE_DIR/prefix/SDL2-$SDL_VERSION-arm64"
PAYLOAD_NAME=OpenJK-SplitScreen-arm64
PAYLOAD="$DIST_DIR/$PAYLOAD_NAME"
ARCHIVE="$DIST_DIR/$PAYLOAD_NAME.zip"

if [[ $(uname -s) != Darwin || $(uname -m) != arm64 ]]; then
	echo "This release pipeline must run natively on Apple Silicon macOS." >&2
	exit 2
fi
for tool in cmake ninja curl shasum zip otool file; do
	command -v "$tool" >/dev/null || { echo "Missing required tool: $tool" >&2; exit 2; }
done
if ! git -C "$ROOT" diff --quiet || ! git -C "$ROOT" diff --cached --quiet; then
	if [[ ${OPENJK_ALLOW_DIRTY:-0} != 1 ]]; then
		echo "Refusing to package a dirty tree. Commit first or set OPENJK_ALLOW_DIRTY=1." >&2
		exit 2
	fi
	DIRTY=true
else
	DIRTY=false
fi

REVISION="$(git -C "$ROOT" rev-parse HEAD)"
SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-$(git -C "$ROOT" show -s --format=%ct HEAD)}"
export SOURCE_DATE_EPOCH MACOSX_DEPLOYMENT_TARGET="$MIN_MACOS"

mkdir -p "$CACHE_DIR/downloads" "$CACHE_DIR/src" "$CACHE_DIR/build" "$CACHE_DIR/prefix" "$DIST_DIR"
if [[ ! -f "$CACHE_DIR/downloads/$SDL_ARCHIVE" ]]; then
	curl -L --fail --retry 3 -o "$CACHE_DIR/downloads/$SDL_ARCHIVE" "$SDL_URL"
fi
echo "$SDL_SHA256  $CACHE_DIR/downloads/$SDL_ARCHIVE" | shasum -a 256 -c -

if [[ ! -f "$SDL_SOURCE/CMakeLists.txt" ]]; then
	cmake -E remove_directory "$SDL_SOURCE"
	tar -xzf "$CACHE_DIR/downloads/$SDL_ARCHIVE" -C "$CACHE_DIR/src"
fi
if [[ ! -f "$SDL_PREFIX/lib/libSDL2.a" ]]; then
	cmake -E remove_directory "$SDL_BUILD"
	cmake -S "$SDL_SOURCE" -B "$SDL_BUILD" -G Ninja \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_OSX_ARCHITECTURES=arm64 \
		-DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
		-DCMAKE_INSTALL_PREFIX="$SDL_PREFIX" \
		-DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST=OFF
	cmake --build "$SDL_BUILD" -j "$JOBS"
	cmake --install "$SDL_BUILD"
fi

if [[ ${OPENJK_CLEAN_BUILD:-1} == 1 ]]; then
	cmake -E remove_directory "$BUILD_DIR"
fi
cmake -S "$ROOT" -B "$BUILD_DIR" -G Ninja \
	-DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_OSX_ARCHITECTURES=arm64 \
	-DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_MACOS" \
	-DSDL2_DIR="$SDL_PREFIX/lib/cmake/SDL2" \
	-DMakeApplicationBundles=ON -DBuildTests=OFF
cmake --build "$BUILD_DIR" -j "$JOBS"

cmake -E remove_directory "$PAYLOAD"
cmake -E make_directory "$PAYLOAD/base" "$PAYLOAD/docs" "$PAYLOAD/tools"
cp -R "$BUILD_DIR/openjk.arm64.app" "$PAYLOAD/"
cp -R "$BUILD_DIR/openjk_sp.arm64.app" "$PAYLOAD/"
cp "$BUILD_DIR/openjkded.arm64" "$PAYLOAD/"
cp "$BUILD_DIR/codemp/rd-vanilla/rd-vanilla_arm64.dylib" \
	"$BUILD_DIR/codemp/rd-rend2/rd-rend2_arm64.dylib" \
	"$PAYLOAD/openjk.arm64.app/Contents/MacOS/"
cp "$BUILD_DIR/code/rd-vanilla/rdsp-vanilla_arm64.dylib" \
	"$PAYLOAD/openjk_sp.arm64.app/Contents/MacOS/"
cp "$BUILD_DIR/code/game/jagamearm64.dylib" "$PAYLOAD/base/"
cp "$BUILD_DIR/codemp/game/jampgamearm64.dylib" "$PAYLOAD/base/"
cp "$BUILD_DIR/codemp/ui/uiarm64.dylib" "$PAYLOAD/base/"
cp "$BUILD_DIR"/codemp/cgame/cgame*arm64.dylib "$PAYLOAD/base/"
cp -R "$ROOT/assets/splitscreen/base/." "$PAYLOAD/base/"

MACOSX_DEPLOYMENT_TARGET="$MIN_MACOS" "$ROOT/tests/splitscreen/build_external_input_sim.sh" >/dev/null
cp "$ROOT/tests/splitscreen/external/macos_input_sim" "$PAYLOAD/tools/"
cp "$ROOT/tests/splitscreen/README.md" "$PAYLOAD/tools/QA-README.md"
cp "$ROOT/LICENSE.txt" "$PAYLOAD/"
cp "$SDL_SOURCE/LICENSE.txt" "$PAYLOAD/SDL2-LICENSE.txt"
cp "$ROOT/scripts/release/install_macos_arm64.sh" "$PAYLOAD/install.sh"
cp "$ROOT/docs/splitscreen"/*.md "$PAYLOAD/docs/"

cat >"$PAYLOAD/BUILD-MANIFEST.txt" <<EOF
product=$PAYLOAD_NAME
revision=$REVISION
dirty=$DIRTY
source_date_epoch=$SOURCE_DATE_EPOCH
architecture=arm64
minimum_macos=$MIN_MACOS
sdl_version=$SDL_VERSION
sdl_source=$SDL_URL
sdl_sha256=$SDL_SHA256
cmake=$(cmake --version | head -1)
xcode=$(xcodebuild -version | tr '\n' ' ' | sed 's/ $//')
EOF

if [[ -n ${OPENJK_SIGN_IDENTITY:-} ]]; then
	find "$PAYLOAD" -type f \( -name '*.dylib' -o -name 'openjkded.arm64' -o -name 'macos_input_sim' \) -print0 |
		while IFS= read -r -d '' binary; do
			codesign --force --sign "$OPENJK_SIGN_IDENTITY" --timestamp=none "$binary"
		done
	codesign --force --deep --sign "$OPENJK_SIGN_IDENTITY" --timestamp=none "$PAYLOAD/openjk.arm64.app"
	codesign --force --deep --sign "$OPENJK_SIGN_IDENTITY" --timestamp=none "$PAYLOAD/openjk_sp.arm64.app"
fi

(
	cd "$PAYLOAD"
	find . -type f ! -name SHA256SUMS -print | LC_ALL=C sort |
		while IFS= read -r file_name; do
			file_name=${file_name#./}
			printf '%s  %s\n' "$(shasum -a 256 "$file_name" | awk '{print $1}')" "$file_name"
		done >SHA256SUMS
)

STAMP="$(date -u -r "$SOURCE_DATE_EPOCH" +%Y%m%d%H%M.%S)"
find "$PAYLOAD" -exec touch -h -t "$STAMP" {} +
cmake -E rm -f "$ARCHIVE"
(
	cd "$DIST_DIR"
	find "$PAYLOAD_NAME" -print | LC_ALL=C sort | zip -X -q "$ARCHIVE" -@
)
shasum -a 256 "$ARCHIVE" >"$ARCHIVE.sha256"

"$ROOT/scripts/release/audit_macos_arm64.sh" "$PAYLOAD"
echo "release archive: $ARCHIVE"
echo "archive checksum: $ARCHIVE.sha256"
