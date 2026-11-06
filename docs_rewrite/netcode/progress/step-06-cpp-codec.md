# Step 6: the codec library in C++

Status: **in progress** (started 5 November 2026; last update 6 November 2026: first stage and part 1 of the second stage done, part 2 next). Plan: [PLAN.md](../PLAN.md), step 6.

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
- Options: `netcodec_BUILD_TESTS`, `netcodec_BUILD_BENCHMARKS`, `netcodec_WARNINGS_AS_ERRORS`, `netcodec_ENABLE_SANITIZER_ADDRESS`, `netcodec_ENABLE_SANITIZER_UNDEFINED`, `netcodec_ENABLE_CLANG_TIDY`, `netcodec_ENABLE_CPPCHECK`.
- Warnings: the template's lists (`-Wconversion`, `-Wsign-conversion`, `-Wshadow`, `-Wold-style-cast`, `-Wdouble-promotion`, ..., GCC extras; MSVC `/W4` and the template's `/w1...`).
- Always on, also inside OpenJK: C++20, floating-point determinism (`-ffp-contract=off`, SSE2 on 32-bit x86, MSVC `/fp:precise`), index checks of spans and containers in debug builds.
- Tests through CTest (`netcodec.rans_golden`, `netcodec.codec_golden`; traces from `netcodec_TRACE_DIR`, default `docs_rewrite/netcode/data/traces`).
- Presets: `msvc-x64`, `msvc-x86`, `msvc-asan`, `mingw-clang-release`, `-debug`, `-asan`, `-x86`, `-tidy`, `gcc-release`, `gcc-debug`, `gcc-asan`, `gcc-x86`, `clang-release`; build directories `build-netcodec/<preset>` at the repository root.
- Not taken: CPM and Catch2 (no external dependency), Emscripten, packaging and CPack, ccache, precompiled headers, unity builds, hardening flags.

clang-tidy (`mingw-clang-tidy` preset, Core Guidelines, bugprone, performance, modernize, readability): the generated source now uses `std::array` and designated initialisers; the remaining findings in `codec.cpp` and `rans.h` are cleared in the second stage.

## Part 2: the format in C++ only (plan for the next session)

The C++ is now the reference of the format (decision of 6 November 2026). The golden tests against the Python bytes stay valid only while the format is unchanged; the first format change replaces `netcodec.codec_golden` by a C++ round trip test (encode, decode, compare every networked field) on the captures, the demos and synthetic messages. `netcodec.rans_golden` stays (the rANS layer does not change).

### A. Data and tools in C++ (do this first)

1. Export the decoded demos once from Python (the Python stays only as the `.dm_26` reader): the same trace format as `export_codec_trace.py`, without the Python bytes, for the 60 training demos, the 20 held-out demos and the captures (`train.bin`, `holdout.bin`, `captures.bin` in `data/traces/`). 427,000 messages would take about 1.7 GB in memory at once: readers and tools must process one stream (demo) at a time; the Debian machine has 8 GB.
2. A C++ trainer (`netcodec_train`, tool target of the project): runs the encoder in a counting mode (an `Out` that counts symbols per context instead of coding), chooses the per-field modes (absolute or delta) and the prediction modes by their measured cost on the training demos (as `train_tables.py` and `run_pred.py` did: a prediction is kept when it saves more than 8 bits on the training set), builds the tables (additive smoothing 0.01, 15-bit quantisation, at least 1 per symbol, the remainder on the most probable symbol) and writes `src/generated/jka_tables.inc` and the modes. The schema part (offsets, widths, C types from `data/fields.json`) can stay generated by `gen_netcodec.py`. Contexts should be defined by the C++ code structurally (field and kind, class and previous field, ...), so that the trainer and the coder share one definition.
3. A C++ size benchmark (`netcodec_bench_size`): bytes on the held-out and training demos by game type, snapshot interval, map, base age, against the legacy bytes of the same messages (OpenJK's `msg.cpp`, as `bench_codec.cpp`), plus the time. First check: with the current format, the C++ trainer must give the step 5 figures (held-out snapshot-only messages 69.0 bytes, ratio 0.483 on all messages); ideally the same tables as Python.

### B. Format fixes, one at a time

Each change: implement in the coder and the trainer, retrain, measure held-out bytes and time, keep it only if it is not worse (or if it is a correctness fix), write the result here.

1. **`-0.0` (correctness, required by the no-loss goal):** a float `-0.0` is classified as an integral 0 and decoded as `+0.0` (the 1.01 engine does the same). Fix: the integral kind must exclude the pattern `0x80000000`.
2. **Field index coding:** "index of the next changed field given the previous one" uses alphabets of 133 (entities, per class) and 202 to 205 (player states); these tables hold 101,080 of the 119,724 frequencies (84 %). Try coding the gap to the next changed field (small alphabet), with the previous field or a bucket of it as context; measure bytes and table size.
3. **Prediction enabled for the player state:** today when the base has any non-zero networked field (a scan of 216 words per message); use "the message has a base" instead.
4. **Per-field modes keyed by the full field (entity or player state)** instead of the bare name shared by both.
5. **Uniform tables computed at start** instead of stored (the generated source shrinks); store only trained tables, possibly only their non-zero entries (04 section 4.5).
6. **Angle test:** `IsAngleIndex` divides by 360 / 65536 in double; an exact integer test on the float bits is possible (same result, maybe faster); measure.
7. Leads to measure, not decided: variable-length final rANS state (up to 1 byte per message); the area mask coded against the base's mask; server command and config string text with the previous byte as context (commands make the largest messages); entity add / remove operations; header fields contexts.

### C. Performance (after the format settles)

Measured hot spots (part 1): entity scans (equality check, then 132 fields), player state scan (201 fields, plus the 216-word "has a base" scan), copying the unchanged entities in the decoder (the engine's snapshot model does it too), about 25 ns per decoded field, rANS about 18 % of encoding. Leads: cache per stored snapshot what is computed per message; compare the networked words with wider loads; decode straight into the engine structures (no `Message`, no vectors) in the engine interface; encoder symbol 16 to 8 bytes (`xMax` follows from the frequency); the method: temporary `rdtsc` phase timers in a local build (never committed), then the benchmarks on the three compilers.

### D. Robustness and completeness (checks of PLAN step 6)

- `escape`: raw coding of an entity or of a message when the coded size would exceed a cap (04 section 4.8).
- Fuzzing: a libFuzzer target on `DecodeMessage` (clang; llvm-mingw ships libFuzzer) under ASan + UBSan, seeded with real messages; a C++ port of the synthetic generator `fuzz.py` (vehicles, pilots, downloads, extreme values, any base age), 100,000 messages round trip.
- Determinism: explicit tests of the 65,536 angle indices and of the position prediction on recorded positions, run on every preset (MSVC, GCC, Clang, x86, x64).

### E. Engine-facing interface (prepares PLAN step 7)

A small header for the engine: encode from the server's structures (`clientSnapshot_t`, entities from `svs.snapshotEntities`), decode into the client's (`clSnapshot_t`, `cl.parseEntities`), caller-provided buffers, a 1,024-entry baseline array; `add_subdirectory(codemp/netcodec)` in OpenJK's CMake (check that the target's C++20 wins over OpenJK's global `-std=c++11` for the netcodec sources only).

### Known gaps of the current code

- Decoded entity images do not set `entityState_t::number` (not a networked field): the engine glue must set it, as the legacy decoder does.
- A decoded `vps` is meaningful only when `hasVehicle` is set (left as it was, for reuse).
- `Message` and `GameState` use vectors and strings: fine for tests and tools, not for the engine interface.
- The product tables and the traces are produced by Python today (`train_tables.py`, `gen_netcodec.py`, `export_*.py`); part 2 moves the training to C++.
