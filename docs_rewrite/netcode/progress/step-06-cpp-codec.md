# Step 6: the codec library in C++

Status: **in progress** (started 5 November 2026). Plan: [PLAN.md](../PLAN.md), step 6.

## Decisions (owner, 5 November 2026)

- Directory `codemp/netcodec/`, its own `CMakeLists.txt`, usable as a subdirectory of OpenJK and standalone (tests and benchmark without the rest of the engine). Not `shared/` (common to the single-player, JK2 and multiplayer trees) and not `lib/` (third-party libraries); the directory holds JKA-specific data (field schema, trained tables).
- C++17, plain C++ interface (a few functions and plain structures, no STL or exceptions in the public header); no C interface.
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

## Next

1. `codemp/netcodec/` skeleton: CMake (standalone and inside OpenJK), `rans` with its tests and a benchmark against the original `rans_byte.h`.
2. Export from Python: schema (from `fields.json`), tables and per-field modes (generated source), golden traces (local).
3. `fieldcodec`, `msgcodec`, `escape`; byte identity with Python on all captures and demos; fuzz; sanitizers; determinism on MSVC, GCC, Clang, x86 and x64.
4. Profile the whole codec and optimise where the time goes.
