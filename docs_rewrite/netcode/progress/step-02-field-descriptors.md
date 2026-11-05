# Step 2: field descriptors from one source

Status: **done**, 5 November 2026. Plan: [PLAN.md](../PLAN.md), step 2.

## Goal and check

Generate the network field descriptors from `codemp/qcommon/msg.cpp`, with a schema version and hash, and make the Python tools read the generated file. Check: the descriptors reproduce the tables extracted by `jkadec.py` (132, 137, 140, 69 entries).

The check is met.

## What was done

- `tools/python/gen_fields.py` (new) reads the four tables of `msg.cpp` (`entityStateFields`, the `_OPTIMIZED_VEHICLE_NETWORKING` `playerStateFields`, `pilotPlayerStateFields`, `vehPlayerStateFields`) and `STAT_WEAPONS`, and writes `tools/fieldinfo/fields.inc`. Widths are copied as written (a number or a macro such as `GENTITYNUM_BITS`).
- `tools/fieldinfo/` (new, CMake): `fieldinfo.cpp` includes `qcommon/q_shared.h` and `fields.inc` and prints, for every field, the width with macros resolved, `offsetof`, `sizeof` and the C type (`decltype`: `bool` for `qboolean`, `int`, `float`, `enum` for other enums such as `trType_t`); for the four player-state arrays (`stats`, `persistant`, `ammo`, `powerups`), the count, offset and element size; and `sizeof(entityState_t)`, `sizeof(playerState_t)`, `MAX_WEAPONS`, `STAT_WEAPONS`. The compiler, not a parser, computes offsets and macro values.
- `gen_fields.py --exe <fieldinfo.exe>` checks that the program was built from the current `fields.inc` (same tables, same names in the same order), that every field is one 32-bit word, and writes `data/fields.json`.
- `data/fields.json` (generated, committed): schema version 2 (version 1 had no types and a float flag); schema hash = SHA-256 of table order, names, widths, C types and arrays (with the `stats[STAT_WEAPONS]` exception), not of the offsets; per field: name, `bits` (the width `msg.cpp` sends today, 0 = float coding), `type` (C type), offset, size; per array: name, count, value width (`-16` short, `-32` long), offset, element size, and the special width of `stats[4]` (19 bits).
- `jkadec.py` loads the tables and the array layout from `fields.json`; its regular-expression extractor is removed (`msg.cpp` is still read for the Huffman counts `msg_hData`). `codec.py`, `bench2.py` and `engine_enc.py` build the player-state array fields from `fields.json` instead of hard-coded lists (checked: same field lists as before; `engine_enc.py` self-check unchanged, 1 mismatch in 1,500 snapshots as before). No tool keeps its own copy of the tables or arrays.

## Results

| check | result |
|---|---|
| Tables from the repository `msg.cpp` and from the study's 1.01 copy `data/msg.cpp`, against the old extractor | identical: 132, 137, 140, 69 entries, names and widths |
| `sizeof(entityState_t)` | 532 = 133 words (the engine's `numFields + 1 == sizeof / 4` assertion) |
| `sizeof(playerState_t)` | 1,552 |
| Every field | 4 bytes |
| Schema hash, version 2 | `007482d1162a52c69b0d17418749ecd093008b6d8dd4fc1ab6a5e42c37aa5c32` (version 1 was `603c6ddd...`) |
| C types: ent / ps / pilot / veh | int 76, float 46, enum 2, bool 8 / int 96, float 26, bool 15 / int 96, float 29, bool 15 / int 37, float 30, bool 2 |
| Every `qboolean` field | 1 bit wide; no boolean has another width |
| Width not matching the C type | the 12 `user*` fields in every table (1 bit); `rocketLockTime`, `rocketTargetTime` (float sent as a 32-bit integer) in ps, pilot, veh; `fd.forceGripCripple` (int, 1 bit) in ps and pilot |
| Decoder tables after schema 2 | unchanged (same names and widths as the pickled caches) |
| Caches `bdata.pkl` and `fullmsgs.pkl` rebuilt with the new decoder | identical to the old ones |
| `run_full.py` | 139.1, 0 failures (unchanged) |
| `run_pred.py pi --fuzz` | 107.3, 0 failures, fuzz seeds 1 to 5 with 0 failures (unchanged) |

## Findings of schema version 2 (5 November 2026)

1. **Booleans.** All 8 entity and 15 or 2 player-state `qboolean` fields are 1 bit wide. Today a changed boolean costs 2 or 3 bits (02 section 2.2); the value bit is redundant. The new codec sends only "changed" for them (04 section 4.2).
2. **`user*` fields are truncated to 1 bit**, floats included (02 section 2.2). Coded by C type in the new codec.
3. **`rocketLockTime` and `rocketTargetTime`** are floats that hold a `serverTime` (`game/bg_pmove.c:5961`, `:5966` in OpenJK) and are sent as their raw 32-bit pattern (width 32, integer path): lossless, but not coded as floats. Coded by type, they become integral floats that the T mode (time against the message's `serverTime`) can predict. Separate game-side limit: a float holds integers exactly only up to 2^24 ms (4.7 hours of server time); beyond that the stored time is already rounded by the game, before the network.
4. **`fd.forceGripCripple`** is an `int` used as a flag (set to 0 or 1, `game/w_force.c:497`, `:5258`, `:5268` in OpenJK) and sent in 1 bit. Treating it as a boolean needs its type changed to `qboolean` in `q_shared.h` (allowed by the owner's goal 2); until then it stays an integer for the codec.

## Decision: runtime width overrides are removed (5 November 2026)

`MSG_CheckNETFPSFOverrides` (`codemp/qcommon/msg.cpp:1890` in OpenJK) reads `ext_data/MP/netf_overrides.txt` and `psf_overrides.txt` from the game's files and changes the widths of the entity and player-state tables at run time ([../02-engine-code-findings.md](../02-engine-code-findings.md) section 2.3). A mod that ships these files changes the wire format; a demo of such a server does not decode with the default tables. The new protocol removes the mechanism: the `user*` fields are coded by their C type, so every value is sent whole and nothing needs widening. Compatibility with existing mods, their files or the old protocol is never a criterion (owner's goal 2, restated 5 November 2026): do not ask whether a mod uses something before changing or removing it. The legacy path in the branch keeps the mechanism until step 7 replaces it. According to the owner (5 November 2026), no mod ships `netf_overrides.txt` or `psf_overrides.txt`. Demos therefore decode with the default tables.

## Regenerate

From `docs_rewrite/netcode/tools/python`, after a change to the tables in `msg.cpp`:

```
python gen_fields.py                                    # writes ../fieldinfo/fields.inc
cmake -S ../fieldinfo -B ../fieldinfo/build && cmake --build ../fieldinfo/build --config Release
python gen_fields.py --exe ../fieldinfo/build/Release/fieldinfo.exe   # writes ../../data/fields.json
```

Delete `bdata.pkl` and `fullmsgs.pkl` afterwards so that the caches are rebuilt with the new tables.

`fieldinfo` has been built only with MSVC (Release) so far. The offsets and sizes in `data/fields.json` are therefore checked on that compiler only; building it with GCC and Clang, x86 and x64, and comparing the output belongs to the Linux builds of step 6.

## What the next steps need from this one

- Step 6 (C++) reads the same descriptors: either `fields.json` at build time, or a C++ table generated from it, checked against the schema hash. The 1.01 pilot rule (`size - 82`) is a protocol rule, not a descriptor; `codec2.py` dropped it in step 4 ([step-04-demo-corpus.md](step-04-demo-corpus.md)).
