# Step 6: the codec library in C++

Status: **in progress** (started 5 November 2026; last update 6 November 2026: first stage, part 1 of the second stage, part 2 A (data, trainer and size benchmark in C++) and in part 2 B the `-0.0` fix (round-trip test) and the coding of server commands against the commands the client holds; field modes chosen in C++ tried and not kept; next: part C, then position and time predictions). Plan: [PLAN.md](../PLAN.md), step 6.

## Decisions (owner, 5 November 2026)

- Directory `codemp/netcodec/`, its own `CMakeLists.txt`, usable as a subdirectory of OpenJK and standalone (tests and benchmark without the rest of the engine). Not `shared/` (common to the single-player, JK2 and multiplayer trees) and not `lib/` (third-party libraries); the directory holds JKA-specific data (field schema, trained tables).
- C++20 inside the library (first C++17, moved to C++20 for `std::span`, C++ Core Guidelines I.13 and Bounds.1); the engine includes only a public header kept compatible with its own standard. Plain C++ interface (a few functions and plain structures, no STL or exceptions in the public header); no C interface. Code follows the C++ Core Guidelines (owner, 5 November 2026).
- CMake builds with MSVC, GCC and Clang. Development builds here with llvm-mingw (clang), CMake and Ninja (portable, in `C:\Logiciels`); the owner validates with Visual Studio 2026 and GCC on Debian 12 (VirtualBox virtual machines). Timings in a virtual machine are indicative; comparisons inside one run are valid.
- Trained tables compiled into the binary (generated source file), not a data file.
- Test traces generated locally from the demos and never committed (the demos contain player names).

## Two stages (owner, 6 November 2026)

1. A first C++ codec reproduces the Python reference byte for byte (golden trace of captures and demos): it proves the port.
2. It is then restructured into the units of the plan and optimised from profiles, under the golden test and the benchmark; the final code keeps the format, not the shape of the Python.

Once the C++ is identical, it becomes the only reference of the format. Format fixes found on the way (prototype oddities: `-0.0` read as `0`, prediction enabled by "the base has a non-zero value", per-field modes keyed by the bare field name, uniform tables stored instead of computed) are then made and measured in C++ only (size benchmark on the training and held-out demos); the Python stays for the published measurements.

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

Files (layout of 6 November 2026, see "CMake project" below): `include/netcodec/rans.h` (header only), `test/rans_trace.h` and `test/test_rans.cpp`, `bench/bench_rans.cpp`, `bench/ryg/rans_byte.h` (the original, public domain, for comparison only; kept in the repository for now, owner 5 November 2026). Trace: `tools/python/export_rans_trace.py` writes `data/traces/rans_trace.bin` (ignored by git): the rANS units and expected bytes of the 8,842 capture messages, tables trained as `run_full.py`, `X0 = 1`.

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

## Message codec, first stage: identical to Python (6 November 2026)

- `tools/python/train_tables.py`: product tables (decision of step 5: one global set) trained on the 60 training demos, with the per-field and prediction modes chosen on them; saved locally (`data/traces/tables.pkl`).
- `tools/python/gen_netcodec.py`: writes `codemp/netcodec/src/generated/jka_codec.inc` (committed): the schema in codec order (offset, codec width, C type, mode, prediction mode and the offsets it reads), the context table of every field and of every structural context, and the frequencies (1,083 distinct tables, 119,724 frequencies, about 240 KB in the binary). Every context the C++ can ask for has a table (untrained ones get the uniform table Python gives), so the C++ has no fallback path.
- `tools/python/export_codec_trace.py`: golden trace (local, `data/traces/codec_trace.bin`, 144 MB): the 3 captures and 5 demos (one held-out demo per game type and a training siege demo with vehicles and pilots), 35,842 messages decoded by `jkadec`, and the Python codec's bytes.
- `include/netcodec/codec.h`, `src/codec.cpp`: message encoder and decoder working on byte images of `entityState_t` and `playerState_t` (offsets of the schema), so the engine can pass its structures unchanged.
- Result: 0 encode and 0 decode mismatches on the 35,842 messages, with clang 23 x64 Release, Debug (index checks), x86, and ASan + UBSan.

## Benchmark against the legacy coding (baseline before the second stage, 6 November 2026)

`bench/bench_codec.cpp` with `bench/legacy/`: OpenJK's own `codemp/qcommon/msg.cpp` and `huffman.cpp`, compiled as the engine compiles them (C++11, `FINAL_BUILD`, its headers) behind a small interface with no engine type (`legacy_codec.h`), driven as the engine drives them (server `SV_SendClientSnapshot`, `SV_WriteSnapshotToClient`, `SV_EmitPacketEntities`; client `CL_ParseServerMessage`, `CL_ParseSnapshot`, `CL_ParsePacketEntities`, vehicle player state included). Stubs (`legacy_stubs.cpp`): no console, no file system (default field widths, as in the recordings). Same messages, same base chain and baselines for both codecs; messages with only commands and snapshots (35,831 of 35,842); best of 10 passes.

Check: the legacy round trip (OpenJK encodes, OpenJK decodes, compared with the recording field by field) gives 0 failures, so the glue drives the engine code correctly.

| clang 23 x64 `-O3`, Ryzen 9 9950X3D | netcodec v1 (first stage) | legacy (OpenJK) |
|---|---|---|
| bytes per message | 96.6 | 218.5 (ratio 0.442) |
| encode, ns per message | 3,772 | 9,419 |
| decode, ns per message | 3,685 | 3,602 |

MSVC 19.51 x64 Release (owner's virtual machine, indicative): netcodec encode 4,365, decode 4,271 ns per message; legacy encode 9,756, decode 4,363; same bytes. MSVC x64 Release and Debug and Win32 Release build with no warning under the strict set (`/W4`, `/WX`) and pass both golden tests.

GCC 12.2 x64 Release (owner's virtual machine, indicative): netcodec encode 3,027, decode 3,857 ns per message; legacy encode 8,012, decode 4,815. GCC Release, Debug (`_GLIBCXX_ASSERTIONS`) and ASan + UBSan build with no warning under the strict set and pass both golden tests.

Clang 14 (Debian 12) x64 Release: netcodec encode 2,890, decode 3,499 ns per message; legacy encode 8,977, decode 4,015. GCC x86 (`-m32`) and Clang build with no warning and pass both golden tests. GCC's `-Wuseless-cast` needed `ToU32` (`rans.h`): a size converted to `uint32_t` needs a cast on 64-bit targets and none on 32-bit ones.

Validation matrix of the first stage (6 November 2026): MSVC x64 Release and Debug, Win32; GCC x64 Release, Debug, ASan + UBSan, x86; Clang 14 x64; clang 23 (llvm-mingw) x64 Release, Debug, ASan + UBSan, x86. Every build: strict warnings as errors, both golden tests pass.

The first run gave 12,332 ns for the netcodec encoder: `MessageEncoder::Encode` resized its output to the 1 MB maximum on every message (zero fill). Fixed (an internal buffer grown once); the table above is after the fix. These are the reference numbers of the second stage.

## Second stage, part 1: structure and hot paths, same bytes (6 November 2026)

Profile of the first stage (phase timers, clang `-O3`): encoding spent 57 % in entities (21 % in a first scan of the 132 fields to know whether an entity changed, then a second scan to code it), 20 % in the player state, 18 % in rANS; decoding 73 % in entities (31 % copying the unchanged entities of the base, which the engine does too, 30 % decoding about 45 fields per message).

Changes, bytes unchanged (golden tests 0 mismatches):

- Units: `src/schema.h` (field descriptors, state image access, field values), `src/numeric.h` (integer and float operations of the format), `src/model.h` / `model.cpp` (tables built once), `src/coder.h` (symbols, raw bits, classes, strings over rANS), `src/field_codec.h` (one changed field and its predictions; a header so that the message loops inline it), `src/message_codec.cpp` (player states, entities, messages). The generated source is split: `jka_schema.inc` (schema and context indices) and `jka_tables.inc` (frequencies, compiled once in `model.cpp`). Typed enumerations (`CType`, `PredMode`) in the generated schema.
- One pass over an entity's fields: the list of changed fields is built once (raw words compared first, the codec value only when they differ) and used to code.
- The decoder reuses the operations of the previous message (no allocation per message once warm).
- clang-tidy at zero on the library (`.clang-tidy`: Core Guidelines, bugprone, performance, modernize, readability; the disabled checks are listed with their reason). No comment of the C++ refers to the Python prototype any more.

| clang 23 x64 `-O3`, best of 10 | encode, ns per message | decode, ns per message |
|---|---|---|
| first stage | 3,400 to 3,800 | 3,600 to 3,700 |
| after this part | 3,080 (−15 %) | 2,850 (−22 %), below the legacy decoder (3,490) |

MSVC x64 Release (owner's virtual machine): encode 3,393 (first stage 4,365), decode 3,170 (4,271) ns per message; legacy 9,592 and 4,064. MSVC x64 Release and Debug and Win32 build with no warning and pass both golden tests.

GCC 12.2 x64 Release: encode 2,847 (first stage 3,027), decode 3,162 (3,857); legacy 7,975 and 4,754. Clang 14 x64: encode 2,660 (2,890), decode 3,026 (3,499); legacy 8,943 and 3,993. GCC Release, Debug, ASan + UBSan, x86 and Clang build with no warning and pass both golden tests.

Run-to-run variation on this machine reaches 8 % (the legacy timings move as much); against the legacy timings of the same run, encoding takes 0.33 to 0.35 of the legacy time and decoding 0.82, before and after the clang-tidy cleanup.

## CMake project (owner's request, 6 November 2026)

Structure after [cpp-best-practices/cmake_template](https://github.com/cpp-best-practices/cmake_template), keeping what fits a library that OpenJK also builds:

- Layout: `include/netcodec/` (headers), `src/` (`codec.cpp`, `generated/jka_codec.inc`), `test/`, `bench/`, `cmake/` (`Warnings.cmake`, `Sanitizers.cmake`, `StaticAnalyzers.cmake`), `CMakePresets.json`, `.clang-tidy`.
- Top-level project (`PROJECT_IS_TOP_LEVEL`): tests and benchmarks built, warnings as errors, in-source builds refused, `compile_commands.json`. Inside OpenJK: only the static library `netcodec::netcodec`; the settings targets `netcodec_options` and `netcodec_warnings` are linked privately and reach no engine target.
- Options: `netcodec_BUILD_TESTS`, `netcodec_BUILD_BENCHMARKS`, `netcodec_BUILD_TOOLS` (added with part 2 A), `netcodec_WARNINGS_AS_ERRORS`, `netcodec_ENABLE_SANITIZER_ADDRESS`, `netcodec_ENABLE_SANITIZER_UNDEFINED`, `netcodec_ENABLE_CLANG_TIDY`, `netcodec_ENABLE_CPPCHECK`.
- Warnings: the template's lists (`-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wold-style-cast`, `-Wdouble-promotion`, ..., GCC extras; MSVC `/W4` and the template's `/w1...`).
- Always on, also inside OpenJK: C++20, floating-point determinism (`-ffp-contract=off`, SSE2 on 32-bit x86, MSVC `/fp:precise`), index checks of spans and containers in debug builds.
- Tests through CTest (`netcodec.rans_golden`; `netcodec.codec_golden` until the server-command coding changed the bytes in part 2 B, replaced by `netcodec.roundtrip`; traces from `netcodec_TRACE_DIR`, default `docs_rewrite/netcode/data/traces`).
- Presets: `msvc-x64`, `msvc-x86`, `msvc-asan`, `mingw-clang-release`, `-debug`, `-asan`, `-x86`, `-tidy`, `gcc-release`, `gcc-debug`, `gcc-asan`, `gcc-x86`, `clang-release`; build directories `build-netcodec/<preset>` at the repository root.
- Not taken: CPM and Catch2 (no external dependency), Emscripten, packaging and CPack, ccache, precompiled headers, unity builds, hardening flags.

clang-tidy (`mingw-clang-tidy` preset, Core Guidelines, bugprone, performance, modernize, readability): the generated source now uses `std::array` and designated initialisers; the remaining findings in `codec.cpp` and `rans.h` are cleared in the second stage.

## Part 2: the format in C++ only (plan for the next session)

The C++ is now the reference of the format (decision of 6 November 2026). The golden tests against the Python bytes stay valid only while the format is unchanged; the first format change replaces `netcodec.codec_golden` by a C++ round trip test (encode, decode, compare every networked field) on the captures, the demos and synthetic messages. `netcodec.rans_golden` stays (the rANS layer does not change).

### A. Data and tools in C++ (do this first)

1. Export the decoded demos once from Python (the Python stays only as the `.dm_26` reader): the same trace format as `export_codec_trace.py`, without the Python bytes, for the 60 training demos, the 20 held-out demos and the captures (`train.bin`, `holdout.bin`, `captures.bin` in `data/traces/`). 427,000 messages would take about 1.7 GB in memory at once: readers and tools must process one stream (demo) at a time; the Debian machine has 8 GB.
2. A C++ trainer (`netcodec_train`, tool target of the project): runs the encoder in a counting mode (an `Out` that counts symbols per context instead of coding), chooses the per-field modes (absolute or delta) and the prediction modes by their measured cost on the training demos (as `train_tables.py` and `run_pred.py` did: a prediction is kept when it saves more than 8 bits on the training set), builds the tables (additive smoothing 0.01, 15-bit quantisation, at least 1 per symbol, the remainder on the most probable symbol) and writes `src/generated/jka_tables.inc` and the modes. The schema part (offsets, widths, C types from `data/fields.json`) can stay generated by `gen_netcodec.py`. Contexts should be defined by the C++ code structurally (field and kind, class and previous field, ...), so that the trainer and the coder share one definition.
3. A C++ size benchmark (`netcodec_bench_size`): bytes on the held-out and training demos by game type, snapshot interval, map, base age, against the legacy bytes of the same messages (OpenJK's `msg.cpp`, as `bench_codec.cpp`), plus the time. First check: with the current format, the C++ trainer must give the step 5 figures (held-out snapshot-only messages 69.0 bytes, ratio 0.483 on all messages); ideally the same tables as Python.

### A. Result (done 6 November 2026)

- `tools/python/export_demo_traces.py` writes `data/traces/train.bin` (60 streams, 319,468 messages, 1.4 GB), `holdout.bin` (20 streams, 108,000 messages, 418 MB) and `captures.bin` (3 streams, 8,842 messages): the decoded messages, the size the server sent for each (without `svc_EOF`) and the demo's row of `selection.tsv`; no coded bytes. Readers process one stream at a time (`test/demo_trace.h`, `DemoTraceFile`); the server loop (`ServerLoop`) skips a message whose base is missing and keeps only the snapshots of the last 255 sequence numbers.
- Contexts defined by the C++ (`src/contexts.h`): 11 contexts per entity field and per player-state field name (one per kind, unused kinds have an empty alphabet), the field-index contexts per table and per entity class, entity operations, entity type per class, message operations and 19 scalar contexts; 6,965 contexts in all, the alphabet of each computed from the schema. Before, the generated source listed table indices, with identical tables merged, so distinct contexts shared one index and could not be counted apart.
- Model (`src/model.cpp`): one table per context; the trained frequencies of `generated/jka_tables.inc`, and for every other context the table of zero counts, computed when the model is built and shared by alphabet size. `Quantize` (additive smoothing 0.01, 15 bits, at least 1 per symbol, remainder on the first most probable symbol, double arithmetic) is shared by the model and the trainer. The generated table source now holds the 1,130 trained contexts only (124,752 frequencies; before, 1,083 distinct tables and 119,724 frequencies including the uniform ones).
- Trainer `netcodec_train` (`tools/train.cpp`): the encoder writes into `CountOut`, which counts the symbols per context (the classes and strings of the format are shared with the coding writer, `SymbolWriter`); 10 seconds for the 60 training demos. `gen_netcodec.py` writes only the schema (offsets, widths, C types, modes) now.
- Size benchmark `netcodec_bench_size` (`bench/bench_size.cpp`): coded bytes against the bytes the server sent, by set, game type, snapshot interval, base age and held-out map, with the encoding time; 13 seconds for the whole corpus.

| check | result |
|---|---|
| Golden tests with the tables trained in C++ and the new contexts | 0 encode and 0 decode mismatches on the 35,842 messages: the C++ trainer gives the same tables as the Python one; clang 23 x64 Release, Debug, ASan + UBSan, x86 |
| Trainer on x86 and x64 | identical `jka_tables.inc` (clang x64 and x86; GCC 12.2 x86 on Debian) |
| Owner's validation on Windows 11 (MSVC 19.51) | x64 Release and Debug, Win32 Release: no warning, both golden tests pass; `netcodec_bench_size` on the held-out set gives the same figures; the Win32 trainer writes an identical `jka_tables.inc`; `netcodec_bench_codec`, three runs: encode 3,661, 3,449 and 3,617 ns against legacy 9,759, 9,980 and 10,429 (0.38, 0.35, 0.35 of the legacy time; part 1: 0.35), decode 3,248, 3,469 and 3,384 against 4,181, 4,369 and 4,448 (0.78, 0.79, 0.76; part 1: 0.78): no regression. The `msvc-asan` preset, not validated before, now builds and passes both golden tests, and `netcodec_bench_size` gives the same figures with no report; two fixes were needed: the legacy code is built with AddressSanitizer and the same container-annotation settings as the project (MSVC's library refuses to link objects that differ), and the AddressSanitizer runtime DLL is copied next to the programs (it is not in the `PATH`) |
| Owner's validation on Debian 12 | GCC Release, Debug, ASan + UBSan, x86 and Clang 14: no warning (after replacing four casts that GCC's `-Wuseless-cast` finds useless on 32 or 64-bit targets), both golden tests pass; `netcodec_bench_size` on the held-out set gives the same figures in Release and under ASan + UBSan (no report); `netcodec_bench_codec` GCC encode 2,821 ns (part 1: 2,847), decode 2,981 ns (3,162), legacy 8,056 and 4,953 |
| `netcodec_bench_size` on the held-out set | snapshot-only messages 69.0 bytes against 155.1 for the engine (ratio 0.445), all messages but the gamestate ratio 0.483, 0 refused: the step 5 figures; training set 73.4 bytes (0.451), all messages 0.494 |
| clang-tidy (`mingw-clang-tidy`) | nothing reported |
| `netcodec_bench_codec` (two runs) | encode 3,444 and 3,572 ns against legacy 10,893 and 10,961 (0.32 and 0.33 of the legacy time; before 0.33 to 0.35); decode 3,373 and 3,339 against 4,132 and 4,060 (0.82, unchanged) |

Lead for later, not done: a `.dm_26` reader in C++ on top of OpenJK's own `msg.cpp` (already built by the legacy benchmark) would remove the Python and the 1.8 GB of traces from the pipeline, and the demos would be decoded by the engine's own code.

### Where the bytes go (6 November 2026, before B.2)

`netcodec_train train.bin jka_tables.inc holdout.bin` reports the cost of the held-out messages with the trained tables (symbol entropy with the quantised tables, plus raw bits attributed to the context of the symbol before them). Held-out set, bytes per message (all messages, 107.4 real bytes per message on average):

| part | symbols | raw bits | total | share | trained frequencies |
|---|---|---|---|---|---|
| entity field values | 14.13 | 37.78 | 51.91 | 48.9 % | 9,170 |
| player-state field values | 3.71 | 10.32 | 14.03 | 13.2 % | 6,614 |
| entity field indices | 4.95 | | 4.95 | 4.7 % | 67,431 |
| player-state field indices | 1.88 | | 1.88 | 1.8 % | 38,899 |
| entity numbers and operations | 2.98 | 0.68 | 3.65 | 3.4 % | 58 |
| server commands | 26.57 | | 26.57 | 25.0 % | 256 |
| config strings, setgame | 0.72 | | 0.72 | 0.7 % | 256 |
| entity type, message operations, header and other scalars | 1.59 | 0.87 | 2.46 | 2.3 % | 2,068 |
| total | | | 106.17 | | |

Field values by kind (entities and player states): other float delta 21.49 (18.63 of it raw bits), position prediction 16.00, angle prediction 8.03, time prediction 5.61, integral float delta 5.21, integer absolute 3.47, integer delta 3.47, float kind 2.22, angle index delta 0.40.

Findings:

- The field indices (B.2) are 6.5 % of the bytes but 85 % of the stored frequencies: B.2 is mainly a table-size change. Coding the gap instead of the index with the same context (the previous field) is a relabelling of the symbols with the same entropy.
- Server commands are a quarter of the bytes, coded byte by byte without context. On the held-out demos (tables from a quarter of the training demos, each reliable command counted once): order 0 (today) 0.540 bytes per text byte, order 1 (previous byte as context) 0.378, order 2 0.276, order 3 0.236. 87.7 % of the space-separated tokens equal the token at the same position in the previous command with the same verb. The commands are `tinfo` (team overlay, 56 % of the command bytes), `scores` (29 %), `kls` (7 %), `cs` (3 %), chat and prints.
- Every command is sent 2.59 times on average on the held-out demos: the server repeats every unacknowledged command in every message until the client's acknowledgement comes back (one round trip). About 60 % of the command bytes are such repeats; the codec cannot remove them (a repeat is never against an acknowledged message), the transport can (plan step 9).
- Other float deltas (velocities, `trDelta`, ...) are the second target: 21.5 bytes per message, mostly raw mantissa bits.

### Server commands against the commands the client holds (done 6 November 2026, owner's choice before B.2)

A command is coded against an earlier command with the same verb (first token) that the client is known to hold: one earlier in the same message, or one at most `kCommandWindow` (127) sequence numbers back and not after the base's last command (`HeaderRecord::cmdSeq`: the client acknowledged the base, so it holds every command up to it). The encoder takes the most recent such command and sends its distance (0: none); then the difference of the token counts and, per token (tokens separated by single spaces), a "same as the reference token" flag (context: previous flag, reference token numeric or not) or the new token: the difference with a numeric reference token, a number, or text (canonical decimal numbers only, so that printing the number gives the token back). Text (tokens, commands without a reference) is coded byte by byte with the previous byte as context (256 contexts). Interface: `Encode`, `DecodeMessage` and `CountMessage` take a `CommandHistory` (the texts by sequence number; the engine already keeps 128 on each side, the codec needs the last 256: retransmitted commands can run ahead of the window). `netcodec.codec_golden` (Python bytes) is removed; `bench_codec` checks its own round trip.

| held-out set | before | commands against the history | + text with the previous byte |
|---|---|---|---|
| server commands, bytes per message (report) | 26.57 | 8.03 | 5.93 |
| all messages but the gamestate, ratio to the engine | 0.483 | 0.400 | **0.391** |
| snapshot-only messages | 69.0 bytes | unchanged | unchanged |
| trained frequencies | 124,752 | 125,198 | 164,618 |

Commands by context after the change (bytes per message): text bytes 3.60, number differences 1.00, "same" flags 0.76, reference distance 0.21, token counts 0.14, kinds 0.14, numbers 0.08. Training set ratio 0.393, captures 0.373. Round trip 0 failures (recorded and `-0.0` variants; clang x64 Release, Debug, ASan + UBSan, x86); trainer x86 and x64 identical; clang-tidy clean. Speed: the first version split the tokens into vectors (two allocations per command) and cost about 10 % more encoding time (0.37 to 0.38 of the legacy time, decode 0.83 to 0.84); with a token cursor and no allocation, encode 0.33 and decode 0.77 to 0.79 of the legacy time (two runs), as before the change. Owner's Debian 12 (`scripts/validate.sh`, all OK): GCC Release, Debug, ASan + UBSan, x86 and Clang 14 build and pass both tests; held-out 0.391 in Release and under ASan + UBSan; the x86 trainer writes the committed tables; `bench_codec` 0 mismatches, encode 2,937 and 3,153 ns against legacy 8,273 and 8,295 (0.36, 0.38; part 2 A: 0.35), decode 3,082 and 3,123 against 5,341 and 5,370 (0.58; part 2 A: 0.60). The first Windows run (`scripts/validate.bat`) passed every build and test; it showed 6,689 `bench_codec` mismatches, also present here: the benchmark sets `entityState_t::number` in the trace images for the legacy code and the new check compared whole images, while the codec does not send that field (it is not networked); the benchmark now sets it in the decoded images, as the engine glue will. Second Windows run, all OK: MSVC x64 Release and Debug, Win32, x64 with AddressSanitizer build and pass both tests; held-out 0.391 in Release and with AddressSanitizer; the Win32 trainer writes the committed tables; `bench_codec` 0 mismatches, encode 3,709 and 3,666 ns against legacy 9,916 and 9,901 (0.37, 0.37; part 2 A runs: 0.38, 0.35, 0.35), decode 3,359 and 3,308 against 4,270 and 4,271 (0.79, 0.77; part 2 A: 0.78, 0.79, 0.76). Within the spread seen so far, encoding perhaps slightly slower with MSVC; to watch in the next measurements.

### Field modes in C++: tried, not kept (6 November 2026)

Tried, measured and reverted (owner: the byte reduction must be worth the time cost; a large reduction may cost a little time, a marginal one must cost none). Recorded so that it is not tried again without a new idea.

- What was tried: the per-field modes (absolute or delta, prediction) as C++ data per entity field and per player-state field instead of the Python choice keyed by the bare name; a trainer option that measures every candidate (10 configurations in one reading of the traces, about 40 seconds; a field's cost: entropy of its contexts, raw bits, 6 bits per distinct symbol); a new prediction for floats of the "other" kind, `2 * base - base's base` in double rounded once to float32 (deterministic: x86 and x64 gave the same bytes).
- Sizes (all messages but the gamestate, ratio to the engine): held-out 0.391 with the Python modes, 0.387 with the cheapest mode for every field, 0.389 when a field changes only for a gain above 2 %; captures 0.373, 0.381, 0.373. The Python modes were already close to the best; the second order saves 0.1 to 0.3 bytes per message on each velocity component, a quarter of an estimate made on consecutive snapshots (the codec predicts from the base and the base's own base, further apart).
- Speed against the previous commit: encoding about +9 % with the cheapest modes and +2.5 % with the 2 % rule; decoding about unchanged. The changed modes of rare fields cost time without saving bytes.
- With the previous modes kept, the code gave the same bytes and still about 1.5 to 3 % more encoding time (reading the fields through a pointer in `PredCtx`, reloaded at every field because the coder's writes could alias it); no gain for the codec, so everything was reverted.

How to time two versions on this machine (runs vary by 5 to 10 % otherwise): build the reference commit apart (`git archive HEAD codemp/netcodec | tar -x -C <dir>`, configure with `-Dnetcodec_OPENJK_DIR` and `-Dnetcodec_TRACE_DIR` pointing to the repository), run it and the current code alternately on one core of the second CCD with high priority (`start "" /b /wait /affinity 0x100000 /high <program> <trace> 20`; the first CCD, with the 3D V-Cache, is left to the owner's games), and compare the absolute best times over three rounds or more.

### B. Format fixes, one at a time

Each change: implement in the coder and the trainer, retrain, measure held-out bytes and time, keep it only if it is not worse (or if it is a correctness fix), write the result here.

1. **`-0.0` (correctness, required by the no-loss goal):** a float `-0.0` is classified as an integral 0 and decoded as `+0.0` (the 1.01 engine does the same). Fix: the integral kind must exclude the pattern `0x80000000`. **Done (6 November 2026):** `FKind` puts `-0.0` in the "other" kind (its exact pattern is coded). New test `netcodec.roundtrip` (`test/test_roundtrip.cpp`): the captures and the held-out demos (23 streams, 116,842 messages) are coded by the server loop and decoded by a client that keeps its own decoded bases, each message compared field by field; each stream runs twice, as recorded and with every zero float set to `-0.0` (the recordings come from the legacy coding, which sends `-0.0` as 0, so they hold none). Before the fix: recorded 0 failures, `-0.0` variant 116,839 failures; after: 0 and 0 (clang x64 Release, Debug, ASan + UBSan, x86; owner's Debian 12: GCC Release, Debug, ASan + UBSan, x86 and Clang 14, no warning; owner's Windows 11: MSVC x64 Release and Debug, Win32, x64 with AddressSanitizer, no warning; 5 seconds in Release, 30 in GCC Debug, 42 in MSVC Debug). Corpus bytes and trained tables unchanged (the golden tests still pass), speed unchanged (encode 0.34, decode 0.81 of the legacy time). Not measurable on the demos: how often the server's own states hold `-0.0`, and so what this costs on a live server (a `-0.0` costs about a 32-bit value instead of one kind symbol); to measure with the state recorder (plan step 3) or the live statistics (step 7), and if it matters, give `-0.0` its own float kind.
2. **Field index coding:** "index of the next changed field given the previous one" uses alphabets of 133 (entities, per class) and 202 to 205 (player states); these tables hold 101,080 of the 119,724 frequencies (84 %). Try coding the gap to the next changed field (small alphabet), with the previous field or a bucket of it as context; measure bytes and table size. **Measured, not kept (7 November 2026):** coding the gap to the next changed field, capped at G symbols with an escape to a class shared by the table, on the held-out set (field indices now 6.82 bytes per message, 106,330 trained frequencies): G = 32, 7.30 bytes and 24,484 frequencies; G = 16, 7.89 bytes and 13,038; G = 8, 8.54 bytes and 7,310. The tables shrink but the messages grow by 0.5 to 1.7 bytes; the owner puts bytes first. The encoder tables were halved instead without changing the bytes (8-byte encoder symbols, part C).
3. **Prediction enabled for the player state:** today when the base has any non-zero networked field (a scan of 216 words per message); use "the message has a base" instead.
4. **Per-field modes keyed by the full field (entity or player state)** instead of the bare name shared by both. Tried and not kept: the Python modes are close to the best; per-field modes chosen in C++ saved at most 0.5 % for more encoding time (section "Field modes in C++: tried, not kept").
5. **Uniform tables computed at start** instead of stored (the generated source shrinks); store only trained tables, possibly only their non-zero entries (04 section 4.5).
6. **Angle test:** `IsAngleIndex` divides by 360 / 65536 in double; an exact integer test on the float bits is possible (same result, maybe faster); measure.
7. Leads to measure, not decided: variable-length final rANS state (up to 1 byte per message); the area mask coded against the base's mask; server command and config string text with the previous byte as context (commands make the largest messages); entity add / remove operations; header fields contexts.
8. **Time fields against the base's lag (done 7 November 2026).** Per-field report on the held-out set (a local breakdown of the trainer's report): the players (entity type 1, always `TR_LINEAR_STOP`, 3.6 changed per message) cost 3.94 bytes per message in `pos.trTime` (8.8 bits per value), coded against the snapshot time. A player's `trTime` is its `commandTime`, which lags the server time by a steady amount per client: against the base's value advanced by the time between the two snapshots it costs 2.10 bytes (4.7 bits); the other entity types (missiles, movers) do better against the snapshot time, but rarely change this field (0.06 byte per message lost). Rule (all time-predicted fields: `pos.trTime`, `apos.trTime`, `commandTime`): when the base state is the same object's previous state (`PredCtx::pred`) and the base's time is known, the reference is the base's value plus `time - btime`; otherwise the snapshot time. Held-out, all messages but the gamestate: 9,366,405 to 9,129,077 bytes, ratio 0.391 to **0.381** (−2.5 %); snapshot-only messages 69.0 to 66.9 bytes (ratio 0.445 to 0.432); time prediction 5.61 to 3.41 bytes per message; training set 0.393 to 0.386, captures 0.373 to 0.368. Warm timing against the previous commit: encode +0.3 %, decode −0.1 % (within the A/A spread). Values exact (round trip 0 failures, both variants; clang Release, Debug, ASan + UBSan, x86); x86 and x64 trainers write the same tables; clang-tidy clean. Windows 11 VM (`scripts/validate.bat`, all OK): MSVC x64 Release and Debug, Win32, x64 with AddressSanitizer pass both tests; held-out 0.381 in Release and with AddressSanitizer; the Win32 trainer writes the committed tables; `bench_codec` 86.1 bytes, 0 mismatches, warm encode 1,950 and 1,952 ns (previous commit 1,956 and 1,969), warm decode 1,247 and 1,241 (previous 1,230 and 1,229). Debian 12 VM, GCC 12: the first version made encoding about 9 % slower (warm 1,435 to 1,555 ns per message, A/B of the previous commit against the change, three rounds; decoding unchanged; clang and MSVC unaffected, GCC 12 on Windows too). It looked like an inlining decision (GCC took `PredictAngle` out of line, seen with `nm`), and forcing its inlining brought the time back; GCC 16.2 (podman) showed no slowdown either way. The next change (8-byte encoder symbols, part C) made GCC 12 encoding 9 % slower again with no inlining change, and the unchanged legacy encoder of the same executable 19 % slower: the cause is where the code falls in memory. With functions, loops and jumps aligned to 64 bytes in both builds (`-falign-functions=64 -falign-loops=64 -falign-jumps=64`), the previous commit and the change time the same (warm encode 1,422 to 1,436 and 1,422 to 1,432 ns), and the forced inlining makes no difference to encoding (1,424 to 1,464 with it, 1,431 to 1,448 without). It was removed, then restored: without it GCC 12 again took `PredictAngle` out of line (`nm`), and the aligned A/B showed warm decoding about 4 % slower (best 1,262 against 1,315 ns, six rounds, no overlap; encoding and the legacy decoder unchanged); with it, decoding and encoding match the previous commit (best 1,279 and 1,263 ns decode, 1,411 and 1,408 encode). On clang the macro costs about 1 % of encoding (code placement: clang inlines `PredictAngle` either way); GCC on Linux is the server target, so it stays. Lesson: under GCC on Linux, a build's code placement moves the timings by 10 to 20 %; GCC A/B runs (`build-netcodec/ab_gcc.sh`, local) align the code in both builds, and an encoder change is timed on GCC Linux too, not only clang and MSVC.
9. **Position prediction, analysed, not changed.** The players' positions cost 12.3 bytes per message (14.0 bits per value). Their residuals: about 20 % within 32 units in the last place (the vertical component in free fall: the mean of the two velocities is exact under constant gravity), the rest between 2^9 and 2^14 units in the last place, that is 0.1 to 1 unit for a position near 1,000 (the player's inputs between the two states). The mean of the velocities beats either velocity alone (17.4 and 17.6 bits). A pmove-style correction `(vn - vo) * h / 2` with a frame time h of 8 ms saves 0.47 bit per horizontal value (about 0.3 byte per message) and nothing vertically; h depends on the client's frame rate, so it was not kept. The rest is not predictable from the two states.

### C. Performance (after the format settles)

Owner's rule (6 November 2026): a format change's byte reduction must be worth its time cost (a large reduction may cost a little time, a small one none); the final codec must be at least as fast as before the format work (encode 0.33, decode 0.78 of the legacy time with clang). Method to compare two versions: section "Field modes in C++: tried, not kept".

Measured hot spots (part 1): entity scans (equality check, then 132 fields), player state scan (201 fields, plus the 216-word "has a base" scan), copying the unchanged entities in the decoder (the engine's snapshot model does it too), about 25 ns per decoded field, rANS about 18 % of encoding. Leads: cache per stored snapshot what is computed per message; compare the networked words with wider loads; decode straight into the engine structures (no `Message`, no vectors) in the engine interface; encoder symbol 16 to 8 bytes (`xMax` follows from the frequency); the method: temporary `rdtsc` phase timers in a local build (never committed), then the benchmarks on the three compilers.

#### First pass (7 November 2026): same bytes, decoding 11 % faster, encoding 7 % faster on warm data

**Measuring.** `bench_codec` reads the whole 144 MB trace in every pass, so each entity image (532 bytes) and player state (1,552 bytes) comes from main memory: coding every message twice in a row, the second time takes about 1.9 to 2.6 µs to encode instead of 3.4 to 3.6, and 1.5 to 1.8 µs to decode instead of 2.9 to 3.5. About half of the benchmark's time is memory latency, which the engine does not have (the server has just written its snapshot's entities). This "cold" time also varies by up to 9 % between two runs of the same binary (A/A test, pinned to one core of the second CCD), more while a game runs on the machine: changes of a few percent cannot be seen in it. Two tools gave usable numbers:

- a warm timing: each message coded 5 times in a row, the fastest kept (TSC ticks), summed over the messages, best of 3; A/A within 0.25 % (encoding) and 0.1 % (decoding); each change compared with a build of the previous commit, alternately. Now part of `bench_codec` ("warm" column, ticks converted to ns with the pass's duration), for netcodec and the legacy coding, so that the virtual machines report it too;
- a sampling profiler (local, not committed): a thread on another core suspends the benchmark thread every 50 µs and records its instruction pointer, symbolised with `llvm-symbolizer` (inlined functions included) and with the DLL exports for library code. The `rdtsc` phase timers of part 1 cost about 30 cycles each and distort short phases such as the entity scan.

**Profile (warm, before this pass).** Encoding: rANS `Finish` 17 %, the 532-byte equality test of the entity images 16 % (a library `memcmp` call), the field loop of the changed entities 12 %, the float kind tests (`IsAngleIndex`, `FKind`, `floor`) about 7 %. Decoding: rANS symbol search and state update about half (the search itself is short, 0.35 steps per symbol and none for 93 % of the symbols: the cost is the chain of dependent loads), `memmove` 12 % (the copies of the unchanged entities). On the cold benchmark the "has a base" scan of 216 words is 1 % of encoding: nothing to gain there.

**Kept** (each: round trip 0 failures, held-out sizes and trainer tables unchanged; warm ticks per message against the previous step):

| change | encode | decode |
|---|---|---|
| entity images compared inline 8 bytes at a time (`SameEntity`, vectorised by the compiler) instead of a library `memcmp` | −6.1 % | = |
| decoder search entries of 8 bytes holding the symbol and its slot range (`rans::CoarseEntry`): a slot inside the bucket's first symbol is decoded without reading `cum` | = | −9.8 % |
| `floor` and `trunc` replaced by exact integer conversions (`FloorI`; they are library calls on SSE2 targets), on the bounded values where they are used | −1.8 % | −1.5 % |
| `IsAngleIndex`: a multiply by 65536 / 360 instead of the division, the exactness test unchanged; same result for all 2^32 patterns (checked exhaustively) | −0.8 % | = |
| the base's base entities found with a forward cursor (`EntityCursor`) instead of a binary search per changed entity | −0.6 % | −0.5 % |
| a decoded entity is built by copying its base in one go (it was zero-filled, then overwritten) | = | −0.6 % |

**Tried, not kept:** the changed fields found from a mask of the differing words (scalar, then vectorised): no gain or slower; a branchless renormalisation in `Finish` (always store the byte, move only if kept): encoding 20 % slower (the branch predicts better than the longer dependency chain); larger decoder search tables (2^8, 2^10, 2^12 buckets): slower on cold data (cache); the table pointer kept in the coder instead of read through the model: no change.

**Result** (clang 23 x64, five rounds alternated with a build of the previous commit): warm encode 7,043 to 6,519 ticks per message (−7.4 %), warm decode 5,460 to 4,835 (−11.4 %); cold encode unchanged within the noise (3,356 and 3,380 ns), cold decode 2,943 to 2,790 ns (−5.2 %), that is 0.36 and 0.78 of the legacy times of the same runs (the reference commit gave 0.35 to 0.38 for encoding in the same runs). Bytes identical (held-out 9,366,405 bytes, ratio 0.391; 69.0 bytes per snapshot-only message); clang Release, Debug, ASan + UBSan, x86 pass both tests; clang-tidy clean.

`bench_codec` with the warm timing, after this pass (clang 23 x64, two runs): netcodec encode 1,623 and 1,626 ns per message warm (legacy 3,964 and 4,003), decode 1,121 and 1,126 (legacy 1,788 and 1,794): 0.41 and 0.63 of the legacy times warm, 0.37 and 0.79 to 0.83 cold. Warm, encoding is below the 2 µs per snapshot of PLAN step 6 (a message here holds commands too). Windows 11 VM (`scripts/validate.bat`, before the warm column, all OK): MSVC x64 Release and Debug, Win32, x64 with AddressSanitizer build and pass both tests; held-out 0.391 in Release and with AddressSanitizer; the Win32 trainer writes the committed tables; cold encode 0.39 and decode 0.70 to 0.76 of the legacy times (previous commit 0.37, 0.77 to 0.79: encoding perhaps slower with MSVC, to check with the warm column). Debian 12 VM (`scripts/validate.sh`, before the warm column, all OK): GCC Release, Debug, ASan + UBSan, x86 and Clang 14 build with no warning and pass both tests; held-out 0.391 in Release and under ASan + UBSan; the x86 trainer writes the committed tables; GCC cold encode 3,099 and 3,074 ns against legacy 8,328 and 8,179 (0.37, 0.38; previous commit 0.36, 0.38), decode 3,142 and 3,337 against 5,383 and 5,696 (0.58, 0.59; previous 0.58).

Windows 11 VM with the warm column (`scripts/validate.bat`, all OK, same builds and checks as above): MSVC x64 warm encode 1,956 and 1,969 ns per message (legacy 4,442 and 4,457: 0.44), warm decode 1,230 and 1,229 (legacy 2,453 and 2,390: 0.50, 0.51); cold 0.40 and 0.75. The two runs agree within 0.7 %. MSVC's warm encoding is about 20 % slower than clang's on this machine (the legacy encoder about 12 %); whether this pass changed MSVC's speed is not known (no warm figure of the previous commit with MSVC): to check with a warm A/B on the Windows VM if it matters. Debian 12 VM with the warm column (`scripts/validate.sh`, all OK after a fix: `__rdtsc` returns `unsigned long long`, not `uint64_t` on Linux, so `bench_codec` reads it through `Ticks()`): GCC 12 warm encode 1,443 and 1,424 ns per message (legacy 3,646 and 3,603: 0.40), warm decode 1,408 and 1,307 (legacy 2,089 and 2,080: 0.67, 0.63); cold 0.37, 0.38 and 0.60, 0.59. The warm decode runs differ by 7 % on this virtual machine (8 GB, shared with the host).

**Encoder symbols of 8 bytes (7 November 2026, same bytes).** `rans::EncSymbol` keeps the reciprocal (32 bits), the bias and the complement of the frequency (16 bits each); the renormalisation bound (`XMax`, the frequency times 2^16) and the multiply-high shift (`RcpShift`, 63 minus the leading zeros of `(freq - 1) | 1`) follow from the frequency, off the state's dependency chain. The encoder tables go from 2.6 to 1.3 MB (164,618 trained symbols plus the uniform tables). Warm timing against the previous commit, clang on a freshly started machine (five rounds, alternated on core 20): encode 1,514 against 1,624 ns per message (−6.8 %), decode 1,116 against 1,113 (same); a first measurement during the owner's use of the machine had shown no change (+0.3 %). GCC 12 on the Debian VM with aligned code (`ab_gcc.sh`, six rounds): encode 1,408 against 1,411 ns, decode 1,263 against 1,279 (same). GCC 16.2 (podman, aligned code, six rounds, `REF=HEAD~1 ab_gcc_podman.sh`, after the commit): encode 1,511 against 1,515 ns (same), decode 1,051 against 1,068 (−1.6 %); without the forced inlining GCC 16 also takes `PredictAngle` out of line (encode 1,516, decode 1,057). With aligned code GCC 16 decodes 17 % faster than GCC 12 (1,051 against 1,263 ns) but encodes 7 % slower (1,511 against 1,408): to look at in the final measurements. (`std::bit_width` returns `int` in libc++ and `unsigned` in GCC 12's libstdc++, so a cast would be useless for one and required for the other: `std::countl_zero` returns `int` in both.)

**MSVC (7 November 2026): encoding about 1 to 2 % slower, accepted.** Windows 11 VM, `scripts/validate.bat` all OK on the commit (bytes and tables unchanged; `scripts/validate.sh` all OK too). Its single runs (warm encode 2,003 and 2,014 ns against 1,955 and 1,963) could not tell: the unchanged legacy encoder moved by 9 % between two runs. An alternated A/B on the VM (`build-netcodec/ab_msvc.bat`, local: MSVC x64 Release, six rounds on one core with high priority, the reference source extracted beforehand with `git archive`) does: warm encode best 1,939 ns for the previous commit (1,939 to 1,950) against 1,962 (1,962 to 1,998), +1.2 % at best and about +2 % typically, no overlap; decode 1,220 against 1,217 and the legacy encoder 4,658 against 4,667 (same). The suspected cause, `std::countl_zero` (MSVC's library tests `__isa_available` before `lzcnt`), is not it: the shift read from the exponent of `float((freq - 1) | 1)` (identical for every frequency from 1 to 32,768) is slower still in the same A/B against the commit (best 1,990 against 1,965 ns, +1.3 %), and not kept. What is left is the bound and the shift derived from the frequency for each symbol. Kept as is: MSVC encodes only for Windows servers (the server is expected on Linux about 95 % of the time), MSVC decoding is unchanged, clang encodes 7 % faster, GCC 12 and 16 are unchanged, and the encoder tables are half the size.

What remains is mostly inherent to the format and the snapshot model: the rANS dependency chain in both directions (one state per message, decided 5 November 2026), the loads of the entity images to compare (the engine has them in cache), and the copies of the unchanged entities into the decoded snapshot (the engine copies them too).

### D. Robustness and completeness (checks of PLAN step 6)

- `escape`: raw coding of an entity or of a message when the coded size would exceed a cap (04 section 4.8).
- Fuzzing: a libFuzzer target on `DecodeMessage` (clang; llvm-mingw ships libFuzzer) under ASan + UBSan, seeded with real messages; a C++ port of the synthetic generator `fuzz.py` (vehicles, pilots, downloads, extreme values, any base age), 100,000 messages round trip.
- Determinism: explicit tests of the 65,536 angle indices and of the position prediction on recorded positions, run on every preset (MSVC, GCC, Clang, x86, x64). **Done (7 November 2026):** `netcodec.determinism` (`test/test_determinism.cpp`, no trace needed) hashes (FNV-1a 64) the float kind, the angle-index test and the angle index of every 16-bit angle index from -720 to 720 degrees (262,145 values) and of its neighbouring patterns (±1 and ±2), of special patterns (zeros, infinities, NaNs, denormals, the largest float), and the decode of the angle indices -400,000 to 400,000 (the range of the second-order angle prediction); the value must equal the one of the Python reference functions (`tools/python/angle_hash.py`, `codec.py`'s `is_a16`, `old_index`, `fkind`). `netcodec.roundtrip` now hashes the coded bytes of the corpus (both runs, 116,842 messages each; the position and angle predictions on recorded states use floating point) and compares the hash with `netcodec_ROUNDTRIP_HASH` in `test/CMakeLists.txt`, which changes with the format, the tables or the traces (the test prints the new value). Both hashes are the same in clang x64 Release, Debug, ASan + UBSan and x86 (llvm-mingw), MSVC x64 Release and Debug, Win32 and x64 with AddressSanitizer (`validate.bat`), GCC 12 Release, Debug, ASan + UBSan and x86 and Clang 14 (`validate.sh`). The hashes are printed with `PRIX64`: GCC's `-Wuseless-cast` refuses a cast of `uint64_t` to `unsigned long long` on x86, where they are the same type.

### E. Engine-facing interface (prepares PLAN step 7)

A small header for the engine: encode from the server's structures (`clientSnapshot_t`, entities from `svs.snapshotEntities`), decode into the client's (`clSnapshot_t`, `cl.parseEntities`), caller-provided buffers, a 1,024-entry baseline array; `add_subdirectory(codemp/netcodec)` in OpenJK's CMake (check that the target's C++20 wins over OpenJK's global `-std=c++11` for the netcodec sources only).

### F. Precision inventory (after D and E; owner, 7 November 2026)

The codec is lossless: the client gets exactly the values the server holds. What the client loses, it loses before the codec, where the game code rounds networked values to save bandwidth. Precision matters more to the owner than a few tenths of a byte, and the game module, cgame's prediction and interpolation may be rewritten (owner's goals 1 and 2).

- List every place where the server-side game code (and the engine) rounds or truncates a networked value: `SnapVector` and `trap->SnapVector` on positions and velocities, `ANGLE2SHORT` on angles (command angles included), integer casts of times and durations, and the like, with `file:line`.
- For each: what the client loses (in units, in time), the bytes per message its removal would cost with the new codec on the corpus (the recordings hold the rounded values: estimate from the distribution of the dropped bits, or record unrounded states with the state recorder of plan step 3), and the code to change (game, cgame, engine).
- The owner decides case by case; the changes themselves go with the engine integration (plan step 7) or the prediction and interpolation audit (plan step 13).

### Known gaps of the current code

- Decoded entity images do not set `entityState_t::number` (not a networked field): the engine glue must set it, as the legacy decoder does.
- A decoded `vps` is meaningful only when `hasVehicle` is set (left as it was, for reuse).
- `Message` and `GameState` use vectors and strings: fine for tests and tools, not for the engine interface.
- The traces are produced by Python (`export_*.py`), and the per-field modes and prediction modes still come from `train_tables.py` through `gen_netcodec.py`; the tables are trained in C++ (`netcodec_train`). Choosing the modes in C++ is part of B (item 4).
