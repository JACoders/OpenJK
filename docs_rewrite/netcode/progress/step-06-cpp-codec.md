# Step 6: the codec library in C++

Status: **in progress** (started 5 November 2026). Plan: [PLAN.md](../PLAN.md), step 6.

## Decisions (owner, 5 November 2026)

- Directory `codemp/netcodec/`, its own `CMakeLists.txt`, usable as a subdirectory of OpenJK and standalone (tests and benchmark without the rest of the engine). Not `shared/` (common to the single-player, JK2 and multiplayer trees) and not `lib/` (third-party libraries); the directory holds JKA-specific data (field schema, trained tables).
- C++20 inside the library (first C++17, moved to C++20 for `std::span`, C++ Core Guidelines I.13 and Bounds.1); the engine includes only a public header kept compatible with its own standard. Plain C++ interface (a few functions and plain structures, no STL or exceptions in the public header); no C interface. Code follows the C++ Core Guidelines (owner, 5 November 2026).
- CMake builds with MSVC, GCC and Clang. Development builds here with llvm-mingw (clang), CMake and Ninja (portable, in `C:\Logiciels`); the owner validates with Visual Studio 2026 and GCC on Debian 12 (VirtualBox virtual machines). Timings in a virtual machine are indicative; comparisons inside one run are valid.
- Trained tables compiled into the binary (generated source file), not a data file.
- Test traces generated locally from the demos and never committed (the demos contain player names).

## Python reference codec, before the C++ code

The C++ encoder must produce the same bytes as `codec.py` / `codec2.py`, so the format changes are made in Python first.

| change | result |
|---|---|
| Final rANS state written little-endian at the start of the message (as `ryg_rans`), instead of big-endian | sizes unchanged; capture, fuzz and 80-demo round trips 0 failures |
| Booleans (C type `bool` in `fields.json`, 40 fields): a changed boolean whose old and new values are 0 / 1 is sent as one "flip" symbol, no value; any other value is sent whole on 32 bits (a `qboolean` is an `int`) | on the capture, 3,024 boolean changes, all flips; sizes unchanged (in delta mode a 1-bit field already cost almost nothing) |
| `user*` fields coded by C type: 32-bit `int` or float, instead of their 1-bit legacy width | sizes unchanged (the demos carry only 1-bit values); 18 more prediction candidates (the float `user*` fields) |
| Checks | `run_full.py` 139.1, `run_pred.py pi --fuzz` 107.3 and fuzz 0 failures (the fuzz now produces boolean values other than 0 / 1); `run_step5.py` held-out 69.0 bytes per snapshot-only message, ratio 0.483 on all messages, 0 failures on 80 demos × 3 table sets |

## rANS: design and optimisation notes

Design: `ryg_rans` `rans_byte.h` (public domain / CC0): 32-bit state, byte renormalisation, lower bound `L = 2^23`, 15-bit probabilities, plus the initial-state trick (`X0 = 1`, 1.8 bytes per message on the capture) and a decoder that stops reading at the end of the input.

Techniques to use or measure:

- Encoder without division: multiply-high by a precomputed reciprocal per symbol (Alverson's method, `RansEncSymbol` fields `rcp_freq`, `rcp_shift`, `bias`, `cmpl_freq`; `freq = 1` handled apart). Exact for the 31-bit state, also with `X0 = 1`.
- Raw bits (chunks of at most 15 bits with a power-of-two frequency): shift and mask instead of division; same bytes.
- Decoder symbol lookup: a full slot-to-symbol table is 32 KB per context at 15 bits (about 36 MB for 1,130 contexts, far beyond the caches). Candidates: linear or SIMD search in the cumulative frequencies (most alphabets have fewer than 40 symbols), Vose's alias method (O(1), a few hundred bytes per context), full tables for the few hottest contexts only.
- Tables built once at load time: frequencies, reciprocals, decode tables; no allocation per message.
- Rejected: interleaved states (about 1.4 to 1.6 times faster, but about 4 bytes more per message for the second state, or "knot tying" to avoid it; CPU is not a constraint, 04 section 4.4); 64-bit state or 16-bit renormalisation (larger final state).
- Possible size gain, to measure in Python: a variable-length final state (the state carries about log2(x) useful bits; up to 1 byte per message).

Sources: [ryg_rans](https://github.com/rygorous/ryg_rans), [rANS with static probability distributions](https://fgiesen.wordpress.com/2014/02/18/rans-with-static-probability-distributions/), [rANS in practice](https://fgiesen.wordpress.com/2015/12/21/rans-in-practice/), [Interleaved entropy coders](https://arxiv.org/pdf/1402.3392).

## Unit `rans` (done)

Files: `codemp/netcodec/CMakeLists.txt` (standalone or inside OpenJK; options `NETCODEC_TESTS`, `NETCODEC_SANITIZE`), `rans.h` (header only), `tests/trace.h` and `tests/test_rans.cpp`, `bench/bench_rans.cpp`, `bench/ryg/rans_byte.h` (the original, public domain, for comparison only; kept in the repository for now, owner 5 November 2026). Trace: `tools/python/export_rans_trace.py` writes `data/traces/rans_trace.bin` (ignored by git): the rANS units and expected bytes of the 8,842 capture messages, tables trained as `run_full.py`, `X0 = 1`.

- Encoder: records the symbols, then codes them last to first with the reciprocal of each frequency (no division); raw chunks with shifts; output buffer and record buffer owned by the caller; refuses (returns 0) when either is too small.
- Decoder: stops refilling at the end of the input; symbol lookup by a coarse table of 2^6 buckets per context (128 bytes per table) and a forward search.

| check | result |
|---|---|
| Golden trace, clang 23 (llvm-mingw) x64 `-O3` (CMake `Release`), x64 `-O0`, x86 `-O3`, x64 with ASan + UBSan | 8,842 messages, 3.2 million units: 0 encode and 0 decode mismatches against the Python bytes, in every build |
| Truncated, mutated and random inputs (5,056) | no fault under ASan + UBSan |
| Output buffer one byte too small | refused in 92 of 92 cases |
| Warnings (`-Wall -Wextra -Wpedantic`; MSVC `/W4`) | none, after three fixes for MSVC (uninitialised trace variables, `_CRT_SECURE_NO_WARNINGS` for the test programs, C4244 masked around the unchanged `rans_byte.h`) |
| Owner's validation after the move to C++20 and spans (5 November 2026) | MSVC x64 Release and Debug (debug index checks), Win32; GCC 12.2 x64 Release, Debug (`_GLIBCXX_ASSERTIONS`), x86, ASan + UBSan; Clang (Debian 12) x64: 0 mismatches, exit code 0, no warning. Benchmark unchanged within noise: MSVC encode 1,402 / decode 1,731 ns per message (before 1,388 / 1,689), GCC 1,268 / 1,670 (before 1,272 / 1,656), Clang 1,386 / 1,749 (before 1,348 / 1,696) |
| Owner's validation (5 November 2026) | MSVC 19.51 (VS 2026) x64 and Win32, GCC 12.2 x64, x86 and x64 with ASan + UBSan, Clang (Debian 12) x64: 0 mismatches, all checks pass, no warning |

C++ Core Guidelines (owner's request, 5 November 2026): named casts instead of C casts (ES.48, ES.49), `noexcept` (F.6), `[[nodiscard]]` on results, `constexpr` symbol setup (Con.5), every member initialised (ES.20), preconditions as `assert` (I.6), RAII for the trace file (R.1), unsigned arithmetic kept on purpose for bit manipulation (ES.101). Buffers and tables are `std::span` (I.13, Bounds.1; C++20), with index checks in debug builds (`_GLIBCXX_ASSERTIONS`, libc++ hardening, MSVC debug checks). No performance change: with spans, encode 1,359 to 1,383 ns and decode 1,683 to 1,692 ns per message; three runs after the changes give encode 1,368 to 1,429 ns and decode 1,645 to 1,762 ns per message (before: 1,403 to 1,486 and 1,807), the ratio to ryg unchanged.

Benchmark (`netcodec_bench_rans`, AMD Ryzen 9 9950X3D, Windows 11, clang 23 `-O3` x64, best of 20 runs over the trace; capture messages average 362 units and 139 bytes; run-to-run variation 5 to 10 %):

| | ns per message | ns per unit |
|---|---|---|
| encode, netcodec | 1,486 | 4.10 |
| encode, ryg `rans_byte.h` (reciprocal for symbols, division for raw bits) | 1,665 | 4.60 |
| decode, netcodec, coarse table 2^6 (kept) | 1,807 | 4.99 |
| decode, netcodec, binary search | 2,139 | 5.91 |
| decode, netcodec, linear search | 2,322 | 6.41 |
| decode, netcodec, coarse 2^8 / 2^10 | 1,921 / 2,169 | 5.31 / 5.99 |
| decode, netcodec, full slot-to-symbol table (517 × 64 KB) | 6,868 | 18.97 |
| decode, ryg, binary search / coarse 2^8 / full table | 2,352 / 2,023 / 7,098 | 6.50 / 5.59 / 19.61 |

MSVC (Visual Studio 2026, MSBuild 18.11, x64 Release, owner's VirtualBox machine; indicative timings): golden test 0 encode and 0 decode mismatches, robustness and small-buffer checks pass, exit code 0. Benchmark in ns per message: encode netcodec 1,388, ryg 1,593; decode netcodec coarse 2^6 1,689, binary search 3,514, linear 2,044, coarse 2^8 1,749, coarse 2^10 1,974, full table 3,991; ryg full table 4,141, binary search 3,544, coarse 2^8 1,921. GCC 12.2 (Debian 12, owner's VirtualBox machine, x64 Release, no warning with `-Wall -Wextra -Wpedantic`): golden test 0 mismatches, also with ASan + UBSan; benchmark in ns per message: encode netcodec 1,272, ryg 1,388; decode netcodec coarse 2^6 1,656, binary search 3,225, linear 2,017, coarse 2^8 1,762, full table 3,831; ryg coarse 2^8 1,760, binary search 3,220, full table 3,832. Clang (Debian 12 package, x64 Release, no warning): 0 mismatches; encode netcodec 1,348, ryg 1,383; decode netcodec coarse 2^6 1,696, binary search 1,938, coarse 2^8 1,815, full table 3,695; ryg binary search 1,887, coarse 2^8 1,891. GCC 12.2 x86 (`-m32`, no warning): 0 mismatches, robustness and small-buffer checks pass. The coarse table 2^6 is the fastest on the three compilers; binary search is much slower with MSVC (branches instead of conditional moves).

Size: netcodec 1.43 bytes per message smaller than ryg (initial-state trick). With the same symbol lookup, the netcodec decoder is faster than ryg's (raw bits with shifts); the full table is the slowest (out of the caches), as expected.

## Next

1. Export from Python: schema (from `fields.json`), tables and per-field modes (generated source), golden traces (local).
2. `fieldcodec`, `msgcodec`, `escape`; byte identity with Python on all captures and demos; fuzz; sanitizers; determinism on MSVC, GCC, Clang, x86 and x64.
3. Profile the whole codec and optimise where the time goes.
