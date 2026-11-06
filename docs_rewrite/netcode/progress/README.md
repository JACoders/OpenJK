# Netcode rewrite: progress

This folder records where the work stands, so that a new session (human or model) can resume without re-reading everything. One document per finished step of [PLAN.md](../PLAN.md); this page is the summary and is updated at the end of every session.

Last update: 6 November 2026 (step 6 in progress: C++ codec identical to the Python reference, restructured and faster than OpenJK; contexts, table training and size benchmark in C++, step 5 figures reproduced; next are the format fixes in C++).

## Where we are

| step of the plan | status | result | document |
|---|---|---|---|
| before the plan: study moved into this repository (OpenJK), re-checked | done | every number of the study and of the demo round trip reproduced here; OpenJK is wire-compatible with 1.01 except the pilot player state | [../08-openjk.md](../08-openjk.md), [../06-reproduction.md](../06-reproduction.md) section 6.7 |
| 1. prediction modes and rANS trick in the Python codec | **done** | 139.1 to **107.3** real bytes per snapshot (engine 337.6), 0 round-trip failures | [step-01-prediction-codec.md](step-01-prediction-codec.md) |
| 2. field descriptors from one source | **done** | `data/fields.json` (schema 2) generated from `msg.cpp`, offsets and C types by the compiler; tools read it; 139.1 and 107.3 unchanged; runtime overrides removed | [step-02-field-descriptors.md](step-02-field-descriptors.md) |
| 3. state recorder in the server | later | not needed before step 5 since demos are used | |
| 4. demo corpus | **done** | 1,116 demos indexed (148 h); 60 train + 20 held-out demos (held out by map), 427,468 messages; round trip 0 failures; pilot rule removed from `codec2.py` | [step-04-demo-corpus.md](step-04-demo-corpus.md) |
| 5. measurements on the demos | **done** | held-out snapshots 69.0 bytes against 155.1 for the engine (ratio 0.445; all messages 0.483); held-out within 6 % of training; one global table set, no age context; 0 round-trip failures | [step-05-demo-measurements.md](step-05-demo-measurements.md) |
| 6. C++ codec library | in progress | `codemp/netcodec/`: rANS unit and whole-message codec in C++20, byte-identical to the Python reference on 35,842 messages (captures and demos), restructured into units, clang-tidy clean; against OpenJK's own `msg.cpp` / `huffman.cpp` on the same messages: 2.3 times fewer bytes, encoding 2.8 to 3.4 times faster, decoding 13 to 34 % faster (MSVC, GCC, Clang); validated on MSVC x64 / Win32, GCC x64 / x86 / ASan, Clang. Part 2 A: contexts defined in C++, tables trained in C++ (`netcodec_train`, same tables as Python), size benchmark (`netcodec_bench_size`: held-out 69.0 bytes, ratio 0.483, as step 5). Next: format fixes (part 2 B) | [step-06-cpp-codec.md](step-06-cpp-codec.md) |
| 7 to 13 | to do | | |

## Next actions

1. Step 6, second stage, part 2 B: the format fixes one by one, each measured with `netcodec_train` and `netcodec_bench_size` (bytes and time) and kept only if it helps; first `-0.0` (correctness), and a C++ round-trip test replaces `netcodec.codec_golden` with the first format change. Order and leads in [step-06-cpp-codec.md](step-06-cpp-codec.md), section "Part 2: the format in C++ only".
2. Then in step 6: worst-case `escape` path, libFuzzer on the decoder, determinism tests (angle indices, positions) on every compiler, the engine-facing interface for step 7.
3. Corpus scope (owner, 5 November 2026): no race, no JA+ or JAPRO demos will come. Possible addition: one FFA bot demo on this branch (base game module), only for the FFA game type ([step-04-demo-corpus.md](step-04-demo-corpus.md), "Corpus scope").

## Working practices (owner's requests, keep them)

- Language: answers to the owner in French, short; everything persisted (docs, comments, commits) in English, full sentences. Never mention the earlier proof-of-concept code base.
- The owner commits, never Claude: give a Conventional Commits title with the step first, for example `feat(netcodec): step 6, ...`; do not stage files.
- C++: C++20 inside netcodec, C++ Core Guidelines (named casts, `noexcept`, `[[nodiscard]]`, `constexpr`, `std::span`, RAII, initialised members), unsigned arithmetic for bit manipulation; strict warnings as errors (top-level project); clang-tidy at zero (`mingw-clang-tidy` preset); no exceptions. Comments describe the format and the code, never the Python prototype.
- Every change: build and run the golden tests on several presets (Release, Debug with index checks, x86, ASan + UBSan), then the benchmarks; a cleanup must show no performance regression. Compare timings with the legacy timings of the same run (this machine varies by up to 8 % between runs).
- GCC-only warnings exist (`-Wuseless-cast` on 32 / 64-bit size types: use `ToU32`); MSVC is stricter on old engine code (the legacy benchmark is built in C++11 without the project settings).
- Mod compatibility is never a criterion and never a question to ask; no loss of precision; as few settings as possible.

## Decisions taken (with the date)

| date | decision | where it is written |
|---|---|---|
| 4 Nov 2026 | Measurements stay referenced to the original 1.01 server (the captures) | [../08-openjk.md](../08-openjk.md) |
| 4 Nov 2026 | Owner's goals: no loss of precision (values identical to today's or more precise, never less); no compatibility constraint; as few network settings as possible, computed at run time; netcode reusable by other JKA forks | [../README.md](../README.md), "Owner's goals" |
| 4 Nov 2026 | Do not restore the 1.01 pilot rule (`size - 82`): it was a bug | [../08-openjk.md](../08-openjk.md) section 8.1 |
| 4 Nov 2026 | No rounding of positions, neither on the client nor on the server (float32 is finer than any useful grid) | [../04-recommendations.md](../04-recommendations.md) section 4.3 |
| 4 Nov 2026 | Settings to remove and compute at run time; new debug cvars and commands are welcome | [../04-recommendations.md](../04-recommendations.md) section 4.10 |
| 5 Nov 2026 | Position prediction in integer arithmetic (0.5 byte more than float, same bits on every compiler) | [step-01-prediction-codec.md](step-01-prediction-codec.md) |
| 5 Nov 2026 | The plan is a separate document, [PLAN.md](../PLAN.md); progress is recorded here; the study documents are renumbered 01 to 08 | this folder |
| 5 Nov 2026 | Client demos (`.dm_26`) replace new proxy recordings: same server message, checked in the client code; the state recorder (step 3) moves later | [PLAN.md](../PLAN.md) step 4 |
| 5 Nov 2026 | Target platforms: Windows and Linux, x86 and x64, client and dedicated server; not macOS or ARM for now. Linux is built and tested alongside Windows from step 6 | [../README.md](../README.md), "Owner's goals" item 5; [../07-transport-study.md](../07-transport-study.md) section 7.7 |
| 5 Nov 2026 | Every legacy choice may be redone if it improves the result, including field types and the time base (owner) | [../README.md](../README.md), "Owner's goals" item 2 |
| 5 Nov 2026 | Descriptors carry the C type of each field (schema version 2); the new codec sends booleans as "changed" only and codes `user*` fields by C type | [../04-recommendations.md](../04-recommendations.md) section 4.2; [step-02-field-descriptors.md](step-02-field-descriptors.md) |
| 5 Nov 2026 | `serverTime` stays the game clock; a real send timestamp is evaluated in step 10 | [../04-recommendations.md](../04-recommendations.md) section 4.9; [../02-engine-code-findings.md](../02-engine-code-findings.md) section 2.4 |
| 5 Nov 2026 | Runtime width overrides (`netf_overrides.txt`, `psf_overrides.txt`) removed from the new protocol; mod compatibility is never a criterion, not even a question to ask | [step-02-field-descriptors.md](step-02-field-descriptors.md); [../README.md](../README.md), "Owner's goals" item 2 |
| 5 Nov 2026 | Demo selection: exclude files with more than 5 % undecodable messages or under 3 minutes; held-out set by map (about 20 % of the maps per game type, seed 1); 60 training and 20 held-out demos, first 5,400 messages each | [step-04-demo-corpus.md](step-04-demo-corpus.md) |
| 5 Nov 2026 | `codec2.py` codes all 140 pilot fields (1.01 rule dropped): real servers send more than 58 | [step-04-demo-corpus.md](step-04-demo-corpus.md) |
| 5 Nov 2026 | Branch defaults: `sv_fps` 20 (upstream 40), `sv_clientRate` 25000 (upstream 50000); other defaults upstream (`sv_snapsPolicy` 1, `cl_maxpackets` 63); rate and snaps policies and the pilot fix are kept | `codemp/server/sv_init.cpp:960`, `:983`; [../08-openjk.md](../08-openjk.md) section 8.2 |
| 5 Nov 2026 | Target game module: the base one (OpenJK's), players sent as `TR_LINEAR_STOP` with `g_smoothClients` 1, never rounded; game code keeps the base game's precision; no `TR_INTERPOLATE` recording needed | [../08-openjk.md](../08-openjk.md) section 8.3 |
| 5 Nov 2026 | One global table set, without the base age as a context (per game type and age context each save about 2 %, less than the differences between maps); one table set must work at any `sv_fps` | [step-05-demo-measurements.md](step-05-demo-measurements.md); [../04-recommendations.md](../04-recommendations.md) section 4.5 |
| 5 Nov 2026 | Ideal-byte benchmarks (`bench2.py`, `bench5.py`) not re-run on the demos, real codec instead; `TR_INTERPOLATE` check dropped (base game module) | [step-05-demo-measurements.md](step-05-demo-measurements.md) |
| 5 Nov 2026 | rANS: ryg_rans `rans_byte.h` design (32-bit state, byte renormalisation, reciprocal encoder), with the initial-state trick (`X0 = 1`), a decoder that stops at the end of the input, and the final state little-endian at the start of the message (`codec.py` changed; sizes and round trips unchanged) | `tools/python/codec.py`; step 6 |
| 5 Nov 2026 | Step 6 layout: directory `codemp/netcodec/`, its own CMake target (named after the role, not the entropy coder, which may be replaced; not in `shared/`, which is common to the single-player, JK2 and multiplayer trees); C++20 inside the library (newer than OpenJK's C++11; `std::span`, C++ Core Guidelines), the engine sees only a public header kept compatible with its own standard; plain C++ interface, no C interface (the engine and the forks are C++); CMake builds with MSVC, GCC and Clang; trained tables compiled into the binary (generated source), not a data file; test traces generated locally from the demos, never committed (player names) | [../PLAN.md](../PLAN.md) step 6 |
| 6 Nov 2026 | Contexts are numbered by the C++ code (`src/contexts.h`); tables are trained in C++ (`netcodec_train`); only trained tables are stored, the others are computed when the model is built | [step-06-cpp-codec.md](step-06-cpp-codec.md), part 2 A |
| 6 Nov 2026 | C++ codec in two stages: first byte-identical to the Python reference (the oracle), then restructured and optimised under that test. Once identical, the C++ becomes the only reference of the format: later format fixes (prototype oddities) are made and measured in C++ only, Python stays for the published measurements | [step-06-cpp-codec.md](step-06-cpp-codec.md) |
| 5 Nov 2026 | All netcodec C++ follows the C++ Core Guidelines; each cleanup is checked with the benchmark for performance regressions | [step-06-cpp-codec.md](step-06-cpp-codec.md) |
| 5 Nov 2026 | No interleaved rANS states: about 1.5 times faster but about 4 bytes more per message, while CPU is not a constraint (04 section 4.4) | step 6 |
| 5 Nov 2026 | Step 2 before step 4, since no demo is available yet; field offsets computed by a small CMake program (`tools/fieldinfo`) built by the owner, not by parsing headers | [step-02-field-descriptors.md](step-02-field-descriptors.md) |

## Open questions for the owner


- Five transport questions of [../07-transport-study.md](../07-transport-study.md) section 7.7 (encryption, player count, IPv6, server browser, Apache-2.0 / OpenSSL licence position; platforms is answered). Not needed before step 9.

## Known issues found on the way (not fixed)

- Legacy client (`CL_ParseSnapshot`, `codemp/client/cl_parse.cpp`): when the old snapshot had no vehicle, the server deltas the vehicle player state from nothing (`MSG_WriteDeltaPlayerstate(msg, NULL, &frame->vps, qtrue)`) but the client deltas it from `old->vps`. Seen while writing the legacy benchmark; not tested (the benchmark follows the server and the client as written and found no failure on the recordings).


- The `user*` mod fields are networked 1 bit wide, floats included: only the lowest bit reaches the client unless a mod widens them with the overrides files ([../02-engine-code-findings.md](../02-engine-code-findings.md) section 2.2).

- With `sv_ratePolicy 2` and `sv_maxRate 0`, the client rate becomes 0 and `SV_RateMsec` divides by zero (`codemp/server/sv_client.cpp:1090`, `codemp/server/sv_snapshot.cpp:679`). Read only, not tested.

## State of the repository

- Nothing of this work is committed unless the owner did it. Check `git status` and `git log` at the start of a session.
- This repository is the OpenJK fork `E:\Git\jka\OpenJK_Netcode`, branch `feature/poc-netcode`.
- Branch changes outside `docs_rewrite/netcode/`: the two defaults in `codemp/server/sv_init.cpp` (`sv_fps` 20, `sv_clientRate` 25000), `.gitignore` (`*.dm_26`, `docs_rewrite/netcode/data/traces/`) and the new library `codemp/netcodec/` (not yet part of OpenJK's CMake: that is step 7). Nothing in the game module.
- `codemp/netcodec/`: `CMakeLists.txt`, `CMakePresets.json`, `.clang-tidy`, `cmake/` (warnings, sanitizers, static analyzers), `include/netcodec/` (`codec.h`, `rans.h`), `src/` (`schema.h`, `contexts.h`, `numeric.h`, `model.h` / `.cpp`, `coder.h`, `field_codec.h`, `message_codec.cpp`, `generated/jka_schema.inc` from `gen_netcodec.py`, `generated/jka_tables.inc` from `netcodec_train`), `test/` (golden tests and trace readers, `demo_trace.h`), `bench/` (`bench_rans.cpp`, `bench_codec.cpp`, `bench_size.cpp`, `legacy/` glue of OpenJK's `msg.cpp` / `huffman.cpp`, `ryg/rans_byte.h` kept for comparison), `tools/` (`train.cpp`).
- Local data, not committed (rebuilt by scripts): `docs_rewrite/netcode/data/traces/` (`tables.pkl` from `train_tables.py`, `rans_trace.bin` from `export_rans_trace.py`, `codec_trace.bin` from `export_codec_trace.py`, 144 MB; `train.bin`, `holdout.bin`, `captures.bin` from `export_demo_traces.py`, 1.8 GB); `data/demos/` (demos and their caches); build directories `build-netcodec/<preset>` at the repository root.
- Python caches (`*.pkl`) are rebuilt by the tools and ignored by git. Delete `bdata.pkl` and `fullmsgs.pkl` after regenerating `data/fields.json`.
- `tools/fieldinfo/build/` is ignored by git; the owner builds `fieldinfo.exe` there.

## How to resume

1. Read this page, then [step-06-cpp-codec.md](step-06-cpp-codec.md) (the step in progress; its last sections say what comes next and how), then step 6 of [PLAN.md](../PLAN.md).
2. Python checks, from `docs_rewrite/netcode/tools/python` (about 3 minutes): `python run_full.py` (study codec: snapshot mean 139.1, 0 failures) and `python run_pred.py pi --fuzz` (step 1: 107.3, 0 failures).
3. C++ checks, from `codemp/netcodec` in Git Bash on this machine (tools: llvm-mingw clang 23 in `C:\Logiciels\llvm-mingw-20261104-ucrt-x86_64`, CMake 4.4.5 in `C:\Logiciels\cmake-4.4.5`, Ninja in `C:\Logiciels\ninja`; none in the `PATH`, nothing installed as a service):
   ```
   export PATH="/c/Logiciels/llvm-mingw-20261104-ucrt-x86_64/bin:/c/Logiciels/cmake-4.4.5/bin:/c/Logiciels/ninja:$PATH"
   cmake --preset mingw-clang-release && cmake --build --preset mingw-clang-release && ctest --preset mingw-clang-release
   ../../build-netcodec/mingw-clang-release/bench/netcodec_bench_codec.exe ../../docs_rewrite/netcode/data/traces/codec_trace.bin 10
   ```
   ```
   T=../../docs_rewrite/netcode/data/traces
   ../../build-netcodec/mingw-clang-release/bench/netcodec_bench_size.exe $T/train.bin $T/holdout.bin $T/captures.bin
   ../../build-netcodec/mingw-clang-release/tools/netcodec_train.exe $T/train.bin src/generated/jka_tables.inc
   ```
   Expected: both golden tests 0 mismatches; benchmark 35,831 messages, netcodec 96.6 bytes against 218.5, encoding about 0.33 and decoding about 0.82 of the legacy time of the same run; size benchmark held-out snapshot-only 69.0 bytes (ratio 0.445), all messages ratio 0.483, 0 refused; the trainer rewrites `jka_tables.inc` unchanged (`git diff` empty) as long as the format and the modes are unchanged. Other presets: `mingw-clang-debug`, `mingw-clang-asan`, `mingw-clang-x86`, `mingw-clang-tidy` (clang-tidy, must report nothing). If the traces are missing: `python train_tables.py`, `python export_rans_trace.py`, `python export_codec_trace.py`, `python export_demo_traces.py` (needs the demos and their caches; about 10 minutes).
4. The owner validates on two VirtualBox machines sharing this repository (`Z:\OpenJK_Netcode` on Windows, `/media/sf_jka/OpenJK_Netcode` on Debian): Windows 11 with Visual Studio 2026 (presets `msvc-x64`, `msvc-x86`; 16 GB), Debian 12 with GCC 12.2 and Clang 14 (presets `gcc-release`, `gcc-debug`, `gcc-asan`, `gcc-x86`, `clang-release`; 8 GB, so keep programs well below that). Give the owner the full command list from `codemp/netcodec` (configure, build, `ctest`, benchmark); the build directories are shared, so deleting them on the VMs also deletes this machine's builds.
5. At the end of the session, write the step document, update the tables above, and record new decisions with their date.
