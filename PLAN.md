# Modernization Plan

**Goal:** Incrementally modernize OpenJK's toolchain and codebase — CMake, C++20, Catch2, Clang-first, cleaner architecture.
**Strategy:** Many small, self-contained PRs, no big-bang rewrites.

---

## PR 1 — CMake minimum version bump

Bump `cmake_minimum_required` to 4.3 across the project.

| File | Change |
|---|---|
| `CMakeLists.txt:21` | `3.1...3.31` → `4.3` |
| `tools/WinSymbol/CMakeLists.txt:18` | `3.1` → `4.3` |

**Completion check:** `cmake -B build_vs -G "Visual Studio 18 2026" -A x64 && cmake --build build_vs` passes.

---

## PR 2 — Switch to CMAKE_CXX_STANDARD 20

Replace the hardcoded `-std=c++11` flag with proper `CMAKE_CXX_STANDARD 20` target property.

| File | Change |
|---|---|
| `CMakeLists.txt:274` | Remove `set(CMAKE_CXX_FLAGS ... -std=c++11)` |
| `CMakeLists.txt` (new) | `set(CMAKE_CXX_STANDARD 20)` + `set(CMAKE_CXX_STANDARD_REQUIRED ON)` |

**Completion check:** Build passes, compiler invocations show `-std=c++20` instead of `-std=c++11`.

---

## PR 3 — Simplify NOEXCEPT / OVERRIDE macros

C++20 baseline means `noexcept` and `override` are always available — collapse the legacy workarounds.

| File | Change |
|---|---|
| `shared/qcommon/q_platform.h:187-197` | Replace conditional `NOEXCEPT` with direct `noexcept` |

---

## PR 4 — Swap Boost.Test for Catch2

Replace the only Boost dependency with header-only Catch2.

| File | Change |
|---|---|
| `tests/CMakeLists.txt` | Remove `find_package(Boost)`, add Catch2 |
| `tests/main.cpp` | Remove `BOOST_TEST_MODULE`, add Catch2 `#define CATCH_CONFIG_MAIN` |
| `tests/safe/string.cpp` | Port `BOOST_*` macros to `REQUIRE`/`CHECK` |
| `tests/safe/limited_vector.cpp` | Port `BOOST_*` macros to `REQUIRE`/`CHECK` |
| `CMakeLists.txt:54` | Remove `(requires Boost)` from option text |

---

## PR 5 — Add Catch2 library, verify tests pass

Land a copy of Catch2 (single-header or submodule) and run `ctest`.

---

## PR 6+ — Phase 2 (tooling) and Phase 3 (code modernization)

Per PLAN.md as discussed — `.clang-format`, `.clang-tidy`, CI updates, incremental code modernizations.

---

## Reversibility

Every PR is a self-contained commit that can be backed out if it breaks.

---

## Non-goals (for now)

- Full ECS / data-oriented engine rewrite
- Dropping MSVC or GCC support entirely
- Rewriting the renderer backend
- Changing the mod-loading ABI
