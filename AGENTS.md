# OpenJK — Agent guidance

## What this is

Community-maintained engine for Jedi Academy (SP + MP) and Jedi Outcast (SP only).  
C++11 / C, CMake 4.3+, GPLv2. Backwards-compatible with vanilla game + mods.

## Directory layout

| Path | What |
|---|---|
| `code/` | JA SP engine + game (`openjk_sp.*`, `jagame*`) |
| `codemp/` | JA MP engine + game + renderers (`openjk.*`, `openjkded.*`, `jampgame*`, `cgame*`, `ui*`) |
| `codeJK2/` | JO SP support — off by default |
| `shared/` | Cross-target code (`qcommon/`, `sys/`, `sdl/`) |
| `tests/` | Boost.Test unit tests (opt-in, requires Boost) |
| `lib/` | Bundled libs (SDL2, OpenAL, zlib, libpng, jpeg-9a, minizip, gsl-lite) |
| `cmake/` | Custom modules + cross-compile toolchains |
| `docs/` | Architecture docs (renderer, libraries, save games) |

## Build

CMake 4.3+ required. Presets are provided for common configurations.

### Configure

```bash
cmake --preset windows-msvc-debug    # or windows-msvc-release
# cmake --preset linux-gcc-debug     # Linux
# cmake --preset macos-clang-debug   # macOS
```

Available presets: `windows-msvc-debug/release`, `linux-gcc-debug/release`, `linux-i686-debug`, `macos-clang-debug/release`.

Manual configure (if presets don't fit):
```bash
cmake -B build -G "Visual Studio 18 2026" -A x64 \
  -DBuildJK2SPEngine=ON -DBuildJK2SPGame=ON -DBuildJK2SPRdVanilla=ON
```

Notable flags:
- `-DBuildTests=ON` — unit tests (requires Boost, MSVC wants static lib)
- `-DBuildPortableVersion=ON` — don't read/write user home dir
- `-DUseInternalLibs=ON` — use bundled deps (auto-on for Windows)
- Linux x86: add `-DCMAKE_TOOLCHAIN_FILE=cmake/Toolchains/linux-i686.cmake`
- Individual components can be toggled OFF (see `CMakeLists.txt:38-53`)

### Build & test

```bash
cmake --build --preset windows-msvc-debug -j 8
ctest --test-dir build              # if -DBuildTests=ON
```

### Install

```bash
cmake --install build --config Debug/Release
```

Release output goes to `build/bin/JediAcademy` and `build/bin/JediOutcast`.

## Key binary names (architecture-suffixed)

| Target | Name |
|---|---|
| JA SP engine | `openjk_sp.<arch>` |
| JA SP game DLL | `jagame<arch>` |
| JA SP renderer | `rdsp-vanilla_<arch>` |
| JA MP client | `openjk.<arch>` |
| JA MP ded server | `openjkded.<arch>` |
| JA MP game DLL | `jampgame<arch>` |
| JA MP cgame DLL | `cgame<arch>` |
| JA MP UI DLL | `ui<arch>` |
| JO SP engine | `openjo_sp.<arch>` |
| JO SP game DLL | `jospgame<arch>` |
| JO SP renderer | `rdjosp-vanilla_<arch>` |

## Versioning

Version string is derived from `git describe --tag` at CMake configure time.  
CI checks out with `fetch-depth: 0` and `fetch-tags: true`. Reproducible builds via `SOURCE_DATE_EPOCH`.

## Testing

- Boost.Test framework, opt-in (`-DBuildTests=ON`)
- Test files in `tests/` (safe string, limited_vector)
- `BOOST_AUTO_TEST_SUITE` / `BOOST_AUTO_TEST_CASE` style
- Only built when `find_package(Boost COMPONENTS unit_test_framework)` succeeds

## Architecture notes

- **Renderer split**: frontend (scene build, no GL) + backend (OpenGL draw). See `docs/renderer-architecture.md`.
- **Mod ABI**: legacy `vmMain`/`dllEntry` interface (Q3 VM style) or newer typed `GetModuleAPI`. See `docs/libraries.md`.
- **JK2_MODE** preprocessor define for Jedi Outcast builds (disables gore, sets product to `openjo_sp`).
- **Engine vs Game separation**: `.exe` loads game `.dll` — enables modding without touching engine.
- **Bundled minizip** always used (modified for `Z_Malloc`).
- **Portable version** (`-DBuildPortableVersion=ON`) writes all files next to the binary instead of user home dir.

## Compiler quirks

- MSVC: `/arch:SSE2`, `/MP` (multi-proc build), `NOMINMAX` + CRT safety disables
- GCC x86: `-msse2 -mstackrealign -mfpmath=sse` (VM crash workaround)
- GCC/Clang: `-fvisibility=hidden` (not project-wide — per-target due to bundled zlib)
- GCC on Windows: static `-static-libgcc -static-libstdc++`
- `NOEXCEPT` macro for VS2013/GCC compat; `OVERRIDE` maps to `override`

## Rules of thumb

- No linter, formatter, or typecheck runner exists.
- No JavaScript, package.json, or Node tooling.
- `q_shared.h` is included first by everything — it's ~2800 lines and defines the project-wide types (`qboolean`, `byte`, etc.), endian helpers, and MSVC warning suppressions.
- GPLv2 — license header on every source file.
