# 6. Reproduction: tools, formats, data, commands

## 6.1 Environment used

- Windows 11, Git Bash; Python 3.14.2 (standard library only, apart from the standard `compression.zstd` module used by `bench4.py` for the zstd comparison; no third-party Python package).
- C and C++ benchmarks: CMake and Visual Studio 2022 (MSVC, x64, Release) on the owner's machine. The assistant's environment had no compiler; the C code was written without being compiled by its author and then compiled and run by the owner, who also supplied the executables for the final runs.
- No network access is needed to re-run anything except cloning the third-party sources (6.4).

## 6.2 Capture file format (written by the proxy)

Defined at the top of `src/JKA_YBEProxy/Engine/NetCapture.cpp` in `E:\Git\jka\JKA_YBEProxy` (committed there; verify). Little endian, append mode.

```
Segment (written each time the file is opened, i.e. each map load / restart):
  u32 tag            0x31474553  "SEG1"
  i32 gametype
  i32 svFps
  u32 mapNameLength
  char mapName[mapNameLength]        (no terminator)
Message (any number):
  u32 tag            0x3147534D  "MSG1"
  i32 svsTime
  i32 outgoingSequence               sequence of the packet that carries the message
  i32 serverId
  u32 length
  u8  clientNum
  u8  reserved[3]
  u8  data[length]                   msg->data as built by the engine (bitstream after the netchan header)
```

The cvar is `proxy_sv_captureNet` (default 0, not archived). Switch it on before the map loads. Only human clients are captured (bots receive no messages). The file is flushed when the buffer fills and when the proxy module is unloaded (map change or shutdown); a crash loses the buffer.

## 6.3 Tools

`tools/python/` (all hard-code the paths listed in 6.7; run from that directory without `python -I`, because the scripts import each other):

| file | purpose |
|---|---|
| `gen_fields.py` | Plan step 2: generates `../fieldinfo/fields.inc` from `codemp/qcommon/msg.cpp`, and with `--exe <fieldinfo.exe>` writes `data/fields.json` (field tables, widths, C types, offsets, arrays, schema version and hash). Every other tool reads `fields.json` ([progress/step-02-field-descriptors.md](progress/step-02-field-descriptors.md)). |
| `demo_index.py` | Plan step 4: decodes every client demo under `data/demos` (in parallel) and writes `data/demos/index.tsv` (decode counts, server, mod, map, game type, players, snapshot interval, duration, engine snapshot size). |
| `demo_select.py` | Plan step 4: train and held-out selection from the index, written to `data/demos/selection.tsv`; `selection()` returns it. `bdata.load_demo` and `fullload.load_demo` load one selected demo, cached in `data/demos/cache/`. |
| `run_demos.py` | Plan step 4 check: round trip of the selected demos with `codec2`, tables trained as `run_full.py` (in parallel). |
| `run_step5.py` | Plan step 5: step 1 codec trained on the training demos, measured on all selected demos with three table sets (global, per game type, age as context), sizes by set, game type, interval, base age, map, server build. |
| `sim_loss_demos.py` | Plan step 5: the `sim_loss.py` grid on the held-out demos with the step 1 codec and one global table set. |
| `jkadec.py` | Decoder of the engine read path: Huffman tree, `MSG_ReadBits`, delta entity and player state, `svc_*` operations. `python jkadec.py file.bin` prints a decode summary and writes `file.bin.dec.pkl`. Also the library imported by the others. |
| `bdata.py` | Builds the per-snapshot corpus (states, call lists) from the three captures; cache `bdata.pkl`. |
| `fullload.py` | Builds the whole-message corpus (all operations with payloads); cache `fullmsgs.pkl`. |
| `bench1.py` | Layout-preserving ladder: engine replay, order-0 byte model, canonical Huffman, model per field. |
| `bench2.py` | New schema in ideal bytes: gap or flag coding, value contexts, quantisation, base age, angle-index option (`ANG16`), position rounding option (`POSQ`, `POS_FIELDS`). Defines the data structures used by the others. |
| `bench3.py` | Coder families on the same symbols: ideal, rANS 12 and 15-bit, Huffman per context, adaptive binary. |
| `bench4.py` | zstd, deflate and lzma on the raw layout; a real 32-bit rANS with round trip. |
| `run_pred.py` | Step 1 of the plan: prediction modes and the rANS initial-state trick in the whole-message codec; per-field choice, round trip on the real messages, `--fuzz` (seeds 1 to 5), `--loss` (loss grid); `pf` or `pi` selects the arithmetic. It sets module attributes of `codec.py` (`X0`, `PMODES`) and calls `codec2.enable_prediction()`; with their defaults every earlier script gives the study's numbers (re-checked) |
| `bench5.py` | Prediction variants on top of `bench2` (time fields against `serverTime`, positions predicted from velocity, second-order angle prediction), ideal bytes; added on 4 November 2026 (03, section 3.12). |
| `run2.py`, `run_k.py` | Drivers: gap/flag/value-context/quantisation variants; base ages 1, 2, 4, 8. |
| `rt_float.py` | Independent encoder and decoder of the float kinds; round trip on real and random floats. |
| `codec.py` | Snapshot codec (header, player state, entities) with a real rANS; round trip; `python codec.py [ages...]`. |
| `codec2.py` | Whole-message codec: all operations, pilot and vehicle states. A library. |
| `run_full.py` | Round trip of every real message with `codec2`. |
| `fuzz.py` | Synthetic valid streams and round trip: `python fuzz.py <seed>`. |
| `sim_loss.py` | Loss and latency simulation with exact round trip. |
| `frag_rate.py` | Fragmentation, rate limiting and gamestate retransmission calculations. |
| `engine_enc.py` | A model of the engine's snapshot writer; `python engine_enc.py` checks it against the measured sizes. |
| `worst.py` | Crowded and adversarial snapshots; writes `bench_worst_*.bin` for the C benchmark. |
| `dump_bench.py` | Writes `bench_input.bin` (6.8) for the C benchmark. |

`tools/fieldinfo/`: `fieldinfo.cpp` and `CMakeLists.txt`, a program that prints the offsets, sizes and C types of the fields listed in `fields.inc` (built by the owner; `build/` is not committed).

`tools/cpubench/`: `bench.c` (own coders A to D), `bench_libs.cpp` (ryg_rans, FSE, qbc, nibrans), `nibrans_impl.c` (nibrans compiled as C), `CMakeLists.txt` (expects the third-party sources under `third_party/`).

Also in this folder:

- `data/`: the three captures, `msg.cpp` (a copy of `codemp/qcommon/msg.cpp` as used by the study, from which the decoder reads the Huffman frequencies; until step 2 also the field tables), `fields.json` (the generated field descriptors, step 2), `bench_input.bin` (13 MB, the C benchmark input), `bench_worst_A32.bin` and `bench_worst_A128.bin` (the crowd scenarios). Checksums are in 6.5.
- `bin/`: `jkabench.exe` and `jkabench_libs.exe`, the Windows x64 Release builds used for the C measurements (checksums in 6.5). They run on the files in `data/`, for example `bin\jkabench.exe data\bench_input.bin`. They need no installation. Timings vary by about 10 % from run to run and between machines (a re-run on 4 November gave 6.4 µs where the study table has 7.2 µs for the engine's encoder).

Not copied (rebuild or re-fetch): the Python caches (`*.pkl`), `bench_worst_B128.bin` and `bench_worst_B1000.bin` (10 MB and 37 MB, produced by `worst.py` in seconds, and unusable by the C benchmark anyway), the CMake `build/` folder, and `third_party/`. A `.gitignore` in this folder excludes the caches.

The Python tools find the data and `msg.cpp` through paths relative to their own location (`../../data`; the repository's `codemp/qcommon/msg.cpp` is only a fallback), so the folder can be copied anywhere and run without editing. This was tested on a copy outside the repository. Only two things are not self-contained: the `file:line` references in these documents point into the repository's source tree, and rebuilding `jkabench_libs` needs the third-party sources of 6.4.

## 6.4 Third-party sources used by the C++ benchmark

Cloned with `git clone --depth 1` into `third_party/<name>`. Commits used:

| name | repository | commit | licence |
|---|---|---|---|
| ryg_rans | github.com/rygorous/ryg_rans | c9d162d996fd600315af9ae8eb89d832576cb32d | CC0 |
| FiniteStateEntropy | github.com/Cyan4973/FiniteStateEntropy | 9f30e0918f87bd835fa040d922a208d7b219e50b | BSD-2 (library), GPL v2 (programs) |
| qbc | github.com/ec-/qbc | 328acfd24c81f3fbecdba1af9c145ffe4c2f2597 | MIT |
| nibrans | github.com/BareRose/nibrans | f5b255323f1fa6515ee7a9f8a10d7f8d4a4c78ff | CC0 |
| fsc | github.com/skal65535/fsc | faee546dedb2671a1f4bf0f4cf7937be6b11f77c | Apache-2.0 (cloned, not built) |
| Turbo-Range-Coder | github.com/powturbo/Turbo-Range-Coder | 310c5fe605702340a455f3bd32e454c04eb19d31 | GPL v3 (cloned, not built) |

The FSE static library is built from `fse_compress.c`, `fse_decompress.c`, `entropy_common.c`, `hist.c`, `debug.c`.

## 6.5 Data checksums

SHA-256 and sizes of the captures used (map `mp/ffa3`):

| file | bytes | SHA-256 |
|---|---|---|
| `ybeproxy_netcapture_bots_5mins.bin` | 2,656,109 | `90081a3488711914304b3aae7b330d30fab46cef61809fadfe76f8f6b0e300da` |
| `ybeproxy_netcapture_with_bots.bin` | 488,994 | `0f322890c71b1efdb76dd830ee2aa473b7ffb9fcb9871fee50d5426359cb5273` |
| `ybeproxy_netcapture_no_bots.bin` | 79,612 | `3a6454092798762adf5abbfb2f02658081d0ecc184d6d05b94b80e44c35fa9c2` |

Other files in `data/` and `bin/`:

| file | bytes | SHA-256 |
|---|---|---|
| `data/bench_input.bin` | 13,092,870 | `0f98a2e51007794907051f0eb7fe2c4ad79d4293de95a754e6db90cab94ca6fa` |
| `bin/jkabench.exe` | 23,040 | `b1e5b46b62aa917185703da947bdc0a8b9fe5c911a3b9fc6630aafcee4fa4fc3` |
| `bin/jkabench_libs.exe` | 43,520 | `b82ff095f1efacd5eb38b01c4e52e885d732bfd366026b3297be95c1c9d47164` |

The captures are the only copy of the data. Keep them (or a derived recording) with the repository if the study is to be re-run. `bench_input.bin` is derived from the 5-minute capture by `dump_bench.py`; the executables are built from `tools/cpubench` (6.6).

## 6.6 Commands, in the order of the study

From `docs_rewrite/netcode/tools/python` (approximate run times on the study machine in brackets):

```
python jkadec.py <capture.bin>        # decoder check on one capture                       [1 s]
python bdata.py                       # builds bdata.pkl                                    [5 s]
python bench1.py                      # engine replay, order-0 byte model, per-field model  [30 s]
python bench2.py gap flag             # new schema, ideal bytes                             [15 s each]
python run2.py                        # variants (value context, 1/16 quantisation)         [90 s]
python run_k.py                       # base age 1, 2, 4, 8                                 [90 s]
python bench3.py                      # coder families                                      [25 s]
python bench4.py                      # zstd / deflate / lzma, real rANS                    [60 s]
python bench5.py                      # prediction variants base T P1 P2 A2 T+P2+A2          [110 s]
python run_pred.py pi --fuzz          # step 1: real codec with prediction, integer arithmetic [90 s]
python run_pred.py pf --fuzz --loss   # the same with float arithmetic, plus the loss grid    [4 min]
python rt_float.py                    # float round trip                                    [5 s]
python fullload.py                    # whole-message corpus (builds fullmsgs.pkl)          [5 s]
python codec.py                       # snapshot codec round trip, real bytes               [8 s]
python codec.py 2 4 8                 # same with older bases                               [25 s]
python run_full.py                    # every real message                                  [17 s]
python fuzz.py 1                      # synthetic corpus, seeds 1 to 5                      [15 s each]
python sim_loss.py                    # loss and latency grid                               [3 min]
python frag_rate.py                   # fragmentation, rate, gamestate                      [10 s]
python engine_enc.py                  # engine writer model against measured sizes          [10 s]
python worst.py                       # worst cases; writes bench_worst_*.bin               [16 s]
python dump_bench.py                  # writes bench_input.bin                              [20 s]
```

C benchmarks (Visual Studio 2022):

```
# ready-made executables, from docs_rewrite/netcode:
bin\jkabench.exe      data\bench_input.bin       # engine, static Huffman, rANS, adaptive binary
bin\jkabench_libs.exe data\bench_input.bin       # ryg_rans, FSE, qbc, nibrans
bin\jkabench.exe      data\bench_worst_A32.bin   # crowd scenarios A32 and A128

# to rebuild them: from docs_rewrite/netcode/tools/cpubench, with third_party/ cloned (7.4)
cmake -S . -B build -A x64
cmake --build build --config Release
build\Release\jkabench.exe      ..\..\data\bench_input.bin
```

The worst-case files `bench_worst_B*.bin` exceed the benchmark's fixed buffers (and the engine's message limit) and must not be passed to it.

To change an option of `bench2.py` from a driver, set the module attributes before running: `B.ANG16 = True`, `B.POSQ = 16`, `B.POS_FIELDS = {...}`.

## 6.7 Hard-coded paths and caches

The copies in `tools/python` use paths relative to the script, so nothing needs editing in a normal checkout. The constants are:

| file | constant | value |
|---|---|---|
| `jkadec.py` | `SRC` | `../../data/msg.cpp` (SHA-256 `d0eea2162bb3b2c917ede93fc4582aff7af88cb2cc706d881176966742fa9d63`, 91,189 bytes), falling back to `../../../../codemp/qcommon/msg.cpp` (this repository's copy, OpenJK; its tables and `msg_hData` are identical, see 08); `msg_hData` is extracted from it. The field tables come from `../../data/fields.json` (`FIELDS`) since step 2 |
| `bdata.py` | `TMP` | `../../data` (the three capture files) |
| `dump_bench.py` | `OUT` | `../../data/bench_input.bin` |
| `worst.py` | `OUTDIR` | `../../data` (writes `bench_worst_*.bin` there) |

The study itself ran with the absolute paths `E:\Git\jka\jediacademy\codemp\...` and `E:\Git\jka\jediacademy\tmp\...`; results are identical. A complete re-run in this repository on 4 November 2026 (every command of 6.6 except `dump_bench.py`, whose output `bench_input.bin` was checked by checksum instead) reproduced every size, count and failure figure of 03 exactly; `worst.py` rewrote `bench_worst_A32.bin` and `bench_worst_A128.bin` byte for byte. The executables of `bin/` (checksums unchanged) gave the same bytes and timings within 10 % of the table (for example engine encode 6,692 ns against 7,177; ryg_rans 954 against 951; engine at 128 crowd entities 54,938 against 59,911).

Caches are created next to the scripts: `hufftree.pkl` (decode table), `bdata.pkl`, `fullmsgs.pkl`. Delete them after changing `jkadec.py`, the capture files, the source of `msg.cpp` or `data/fields.json`. Regenerating `fields.json` needs the repository's `codemp/` and a C++ compiler; using it does not, so the folder stays self-contained.

## 6.8 `bench_input.bin` format (read by `bench.c` and `bench_libs.cpp`)

Little endian.

```
char magic[4] = "JKB1"
u32 nctx, nnode, nsnap, ntree
for each context: u32 K; u16 freq12[K] (sums to 4096); u8 huffmanLength[K]
u16 nodeInit[nnode]                     (12-bit initial probabilities of the binary decomposition)
for each of ntree: i32 left, right, symbol, parent     (engine Huffman tree; symbol -1 = internal node)
u32 root
for each snapshot:
  u32 nsym;  nsym x { u16 ctx; u16 symbol; u8 rawBits }
  u32 ndec;  ndec x { u32 node; u8 bit }
  u32 ncall; ncall x { i8 bits; u32 value }            (the engine's MSG_Write calls)
  u32 nbits; u32 nbytes; u8 packet[nbytes]             (the engine bitstream)
```

## 6.9 Recording more data

1. Build the proxy (`E:\Git\jka\JKA_YBEProxy`), start the dedicated server, set `proxy_sv_captureNet 1` **before** the first `map`.
2. Load the map, join, play. Change map or stop the server cleanly to flush.
3. Move the file away (it is opened in append mode) and name it.
4. Decode it with `jkadec.py`: every message after the first gamestate must end within 1 to 8 bits of its length.
5. Add it to `FILES` in `bdata.py`; for new maps, check that `ffa3`-specific assumptions do not appear (the gamestate and baselines are read from each capture, nothing is hard-coded by map).

For the study's open gap, record sessions with several human players, and on other maps and modes, and keep some recordings out of any training.

## 6.10 Known fragilities of the tooling

- Several scripts rely on module-level switches (`B.ANG16`); set them before calling functions that tokenise values.
- `jkadec.py` returns the decoded snapshot states by reference; scripts that mutate them would corrupt the corpus. They copy where they write.
- The scripts are research code: lint warnings (long functions, names such as `T`) were ignored on purpose.
- Python is slow; the largest run (`sim_loss.py`) takes minutes. The C benchmarks measure speed; the Python codec does not.
