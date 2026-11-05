# 3. Results

All tables come from the scripts in `tools/` (the script is named under each table). "5min-test" is the last 40 % of the 5-minute capture; "bots" is the second capture with bots; "solo" is the capture with one player. Models are always trained on the first 60 % of the 5-minute capture. Reasons and limits are in [01-methodology.md](01-methodology.md); the weaknesses of these numbers are listed in [05-verification-guide.md](05-verification-guide.md).

Sizes are payload bytes per snapshot message (without netchan header, UDP/IP or encryption tag) unless a table says otherwise.

## 3.1 Corpus

| capture | duration | decodable messages | snapshot-only messages used for size tables |
|---|---|---|---|
| 5min | 357 s | 7,101 | 6,384 (3,830 train, 2,554 test) |
| bots | 82 s | 1,605 | 1,439 |
| solo | 6.5 s (after the gamestate) | 136 | 126 |

## 3.2 Size: coding alone, and the new schema

Script: `bench1.py` (rows 1-4), `bench2.py` (rows 5-9), `bench3.py` (coder families), `bench4.py` (rows 10-11 and real rANS).

| # | method | 5min-test mean | p95 | max | bots | solo | vs engine |
|---|---|---|---|---|---|---|---|
| 1 | Engine today (Huffman, table `msg_hData`) | 337.6 | 571 | 809 | 255.2 | 17.1 | reference |
| 2 | Static canonical Huffman, order 0, engine layout | 322.5 | 544 | 785 | 243.2 | 17.2 | -4.5 % |
| 3 | Ideal entropy coder (rANS/arith/tANS), order 0, engine layout | 321.9 | 542 | 783 | 242.7 | 17.2 | -4.6 % |
| 4 | Ideal, one model per field, engine layout | 227.3 | 367 | 536 | 162.3 | 10.2 | -32.7 % |
| 5 | New schema, static Huffman per context | 186.1 | 316 | 570 | 138.9 | 6.6 | -44.9 % |
| 6 | New schema, rANS 12-bit probabilities | 169.1 | 289 | 535 | 126.3 | 6.6 | -49.9 % |
| 7 | New schema, ideal (equal to rANS 15-bit and to adaptive binary) | 168.5 | 288 | 532 | 125.8 | 6.8 | -50.1 % |
| 8 | New schema, real 32-bit rANS | 172.0 | 291 | 536 | not measured | not measured | -49.1 % |
| 9 | New schema, ideal, with angles as 16-bit index (lossless) | 136.9 | 235 | 477 | 103.5 | 6.4 | -59.5 % |
| 10 | zstd with a 32 KB trained dictionary, level 19, on the raw (non-Huffman) layout | 331.1 | 541 | n.m. | 254.0 | 31.5 | -1.9 % |
| 11 | deflate with a 32 KB preset dictionary | 336.9 | 558 | n.m. | 255.8 | 22.6 | -0.2 % |

Other generic compressors on the raw layout: zstd dictionary 8 KB level 19: 331.3 / 254.3 / 31.5; zstd 32 KB level 3: 337.4 / 259.7 / 31.5; lzma without dictionary: 355.4 / 270.8 / 26.6. The raw layout itself is 353.8 / 268.6 / 22.6.

New-schema variants (ideal bytes, 5min-test / bots / solo):

| variant | result |
|---|---|
| changed fields coded as gaps (used above) | 168.5 / 125.8 / 6.8 |
| one flag per field instead of gaps | 173.7 / 130.7 / 8.5 |
| gaps plus value contexts by entity class | 167.3 / 124.8 / 6.8 |

## 3.3 Where the bytes go (new schema, before the angle trick, 168 bytes)

Script: ad hoc breakdown in the session; reproducible by grouping the contexts of `bench3.event_lists` by their first element.

| category | bytes per snapshot |
|---|---|
| entity field values | 135.5 |
| player state values | 14.4 |
| "which field changed" (entities) | 9.7 |
| entity numbers and end of list | 5.2 |
| other (player state field list, header, operations) | 3.1 |

Dominant fields: `apos.trBase[0]` 28.5, `apos.trBase[1]` 27.1 (player angles), `pos.trBase[1]` 20.5, `pos.trBase[0]` 19.9, `pos.trTime` 8.1, `pos.trBase[2]` 6.3. These six make about 110 of the 168 bytes. On the engine side the same fields cost 210 to 245 bits per snapshot each.

## 3.4 Coder families on the same symbol streams (new schema)

Script: `bench3.py`, `bench4.py`.

| coder | 5min-test | bots | solo |
|---|---|---|---|
| ideal static | 168.5 | 125.8 | 6.8 |
| rANS 15-bit probabilities | 168.5 | 125.8 | 6.7 |
| rANS 12-bit probabilities | 169.1 | 126.3 | 6.6 |
| static Huffman per context | 186.1 | 138.9 | 6.6 |
| adaptive binary, reset per packet | 168.4 | 125.7 | 6.6 |
| real 32-bit rANS (round trip verified on 400 packets) | 172.0 | n.m. | n.m. |

Per snapshot: 283 symbols and 1,510 binary decisions on the test part (310 and 1,645 on the CPU-benchmark sample).

## 3.5 Speed (C benchmark, MSVC Release, owner's machine)

Script: `tools/cpubench/bench.c` and `bench_libs.cpp` with `dump_bench.py`. Nanoseconds per snapshot; sample of 1,000 test snapshots (one in three), so bytes differ from 3.2. Each method is verified by a round trip. Symbol construction (state comparison, delta application) is not included. "Core" columns are the share of one core spent on encoding for N clients at 20 snapshots per second.

| method | encode ns | decode ns | bytes | encode gain | decode gain | 8 clients | 32 | 64 |
|---|---|---|---|---|---|---|---|---|
| A. Engine today | 7,177 | 6,588 | 369.7 | 1.0x | 1.0x | 0.11 % | 0.46 % | 0.92 % |
| B. Static Huffman per context | 1,891 | 1,202 | 202.8 | 3.8x | 5.5x | 0.03 % | 0.12 % | 0.24 % |
| C. rANS 12-bit (own code) | 1,936 | 1,979 | 188.5 | 3.7x | 3.3x | 0.03 % | 0.12 % | 0.25 % |
| D. Adaptive binary (own code) | 3,856 | 4,634 | 184.6 | 1.9x | 1.4x | 0.06 % | 0.25 % | 0.49 % |
| R. ryg_rans | 951 | 1,955 | 188.5 | 7.5x | 3.4x | 0.02 % | 0.06 % | 0.12 % |
| F. FSE tANS, one state, 12-bit | 1,524 | 1,994 | 186.5 | 4.7x | 3.3x | 0.02 % | 0.10 % | 0.20 % |
| Q. qbc, engine byte stream | 8,748 | 13,041 | 363.2 | 0.8x | 0.5x | 0.14 % | 0.56 % | 1.12 % |
| N. nibrans, engine byte stream, reset per packet | 2,806 | 3,301 | 372.5 | 2.6x | 2.0x | 0.04 % | 0.18 % | 0.36 % |
| N. nibrans, persistent model (invalid under packet loss) | n.m. | n.m. | 371.0 | | | | | |

- B, C, D, R and F code the new schema; A, Q and N code the engine layout.
- FSE ends each packet with a 12-bit state, rANS with a 32-bit state: 2.0 bytes less per packet (186.5 against 188.5).
- qbc and nibrans do not beat the engine's trained Huffman table: adaptive coders cannot learn in 300 bytes.
- My own code is simple (division in rANS, bit-by-bit Huffman, no SIMD): the numbers give order of magnitude and ranking, not the speed of tuned libraries.

Memory of the tables (the benchmark uses dense tables: 316 contexts, 31,997 entries; sparse tables with an escape code would hold about 1,867 non-zero entries, around 7 KB):

| method | memory |
|---|---|
| Engine today | about 57 KB (32-bit pointers), 102 KB (64-bit) |
| Static Huffman per context | 224 KB |
| rANS (own code) | 129 KB plus a 1.29 MB decode table (4,096 bytes per context; can be replaced by a binary search) |
| ryg_rans | about 0.5 MB of encode symbols (estimate) |
| FSE | 2.8 MB (encode) plus 5.2 MB (decode) at table log 12 per context |
| Adaptive binary | 8.6 KB initial plus 25.8 KB state |

## 3.6 Precision

Script: ad hoc analysis in the session (grid test over changed float fields) and `rt_float.py` (round trip).

What the engine sends, counted over the 5-minute capture (changed values):

| field | changed values | already a multiple of 360/65536 | off grid |
|---|---|---|---|
| player angle `apos.trBase[1]` | 60,682 | 60,676 | 6 |
| player angle `apos.trBase[0]` | 58,898 | 58,898 | 0 |
| player-state view angle `viewangles[1]` | 4,144 | 4,141 | 3 |
| player-state view angle `viewangles[0]` | 3,956 | 3,953 | 3 |
| player-state `velocity[0..2]` | all | integral | 0 |
| player position `pos.trBase[0]`, `[1]` | about 51,000 each | under 2 % | over 95 % |
| player-state `origin[0..2]` | about 5,500 | under 4 % | over 96 % |

Size by precision policy (ideal bytes, 5min-test / bots / solo; p95 and max for 5min-test):

| variant | mean | p95 | max | bots | solo | vs engine | values received by the client |
|---|---|---|---|---|---|---|---|
| Engine today | 337.6 | 571 | 809 | 255.2 | 17.1 | reference | exact |
| New schema | 168.5 | 288 | 532 | 125.8 | 6.8 | -50.1 % | identical |
| New schema + angles as 16-bit index | 136.9 | 235 | 477 | 103.5 | 6.4 | -59.5 % | identical |
| + other entities' positions at 1/8 unit (player-state origin exact) | 111.5 | 199 | 429 | 84.8 | 6.4 | -67.0 % | rounded, max error 1/16 |
| + other entities' positions at 1/16 unit (player-state origin exact) | 113.9 | 203 | 434 | 86.6 | 6.4 | -66.3 % | rounded, max error 1/32 |
| + all positions at 1/16 unit (player-state origin too) | 108.9 | 190 | 406 | 82.9 | 6.4 | -67.7 % | rounded, max error 1/32 |

The three rounded rows were computed before the angle delta was corrected; the difference with the corrected code is under 0.3 byte. They are rejected for the reasons in 02, section 2.8 and 04, section 4.3.

Round trip of the float coding (`rt_float.py`, independent encoder and decoder): 517,000 changed floats from the three captures, including 165,554 angle indices, and 400,000 random patterns (including NaN, infinities, 360.0, negative angles): 0 mismatches. The only exception is `-0.0`, which becomes `+0.0` exactly as in the engine today.

## 3.7 Full codec: round trip and real sizes

Script: `codec.py`, `codec2.py`, `run_full.py`, `fullload.py`. Static rANS, 32-bit state, 15-bit probabilities, one stream per message. Models trained on the first 60 % of messages of the 5-minute capture.

Snapshot-only messages, base age 1 (header base known), real bytes (`codec.py`):

| capture | snapshots tested | failures | mean bytes | p95 | max |
|---|---|---|---|---|---|
| 5min-test | 2,278 | 0 | 139.6 | 238 | 479 |
| bots | 1,284 | 0 | 105.2 | 215 | 564 |
| solo | 119 | 0 | 9.1 | 15 | 21 |

Fixed base ages (`codec.py 2 4 8`): 5min-test 172.1 / 223.0 / 291.2 bytes mean for ages 2 / 4 / 8; bots 132.4 / 173.8 / 229.4; solo 9.4 / 10.7 / 11.7; all with 0 failures. No base (full state from the baselines, 256 snapshots, one in ten): 737 bytes mean, 1,299 maximum, 0 failures.

Whole messages (`run_full.py`; 8,842 messages, 0 failures):

| message type | 5min: count, real bytes | bots | solo |
|---|---|---|---|
| snapshot only | 6,384; mean 139.1 (p95 240, max 744) | 1,439; mean 106.5 | 126; mean 9.3 |
| server commands + snapshot | 715; mean 210.9 (p95 321, max 1,117) | 164; mean 263.9 (max 1,804) | 8; mean 138.0 |
| gamestate | 4,024 bytes (engine 7,048, -43 %) | 2,024 (engine 3,211, -37 %) | 2,023 (engine 3,212, -37 %) |
| `svc_mapchange` | 9 bytes | 9 | 9 |

The gamestate figures are optimistic: the configuration strings of the 5-minute gamestate are in the training data and the two other captures use the same map. Strings are coded with an order-0 model.

## 3.8 Synthetic correctness corpus

Script: `fuzz.py`. 5 seeds, each 30 streams of 120 messages: 3,630 messages with uniform tables and 1,815 with trained tables per seed, 27,225 messages in all, 0 failures. Coverage for seed 1: 3,054 snapshots (2,282 normal, 322 with a vehicle, 450 pilot player states), 292 without a base, 2,465 with base age above 1, 4,297 server commands, 98 download blocks, 89 `setgame`, 42 `svc_mapchange`, 34 `nop`, 30 gamestates. A first run found 9 mismatches, all caused by the comparison code treating an empty vehicle state as absent.

## 3.9 Loss and latency

Script: `sim_loss.py`. Mean real bytes per snapshot message sent, test part of the 5-minute capture, tables and modes trained once on a mix of conditions, 0 round-trip failures in all 20 runs.

| loss | RTT 50 ms (base age 1) | RTT 100 ms (age 2) | RTT 200 ms (age 4) | RTT 400 ms (age 8) |
|---|---|---|---|---|
| 0 % | 152.2 | 183.8 | 235.0 | 322.4 |
| 2 % | 152.9 | 184.2 | 235.6 | 322.5 |
| 5 % | 153.6 | 185.3 | 236.2 | 322.7 |
| 10 % | 156.2 | 187.0 | 237.5 | 323.2 |
| 20 % | 159.7 | 189.8 | 241.3 | 324.2 |

p95 ranges: 260-279 (50 ms), 324-336 (100 ms), 425-444 (200 ms), 573-576 (400 ms). Maximum: 675-882. No run needed a snapshot without a base. The 0 % / 50 ms figure (152.2) is higher than the 139.1 of the age-1 codec table (3.7), but the two are not comparable: this simulation counts every snapshot message of the test part including those with server commands, and its tables are trained on a mix of ages. The effect of age-specific tables was not isolated.

Age sensitivity without the loss model (ideal bytes, `bench2.py`): age 1 168.5, age 2 202.4 (+20 %), age 4 254.4 (+51 %), age 8 325.1 (+93 %); bots 125.8, 153.1, 196.3, 254.2.

## 3.10 Network layer

Script: `frag_rate.py`. Formulas from the source (02, section 2.7), sizes from the codec.

Messages of 1,300 bytes or more (fragmented) among the 8,836 snapshot messages of the three captures: engine 3 (max 3,255 bytes), new codec 2 (max 1,804 bytes).

Rate limiting, test part of the 5-minute capture, `snaps` 20 (50 ms), delayed share and effective snapshots per second:

| rate (bytes/s) | engine | new codec |
|---|---|---|
| 3,000 | 98.5 %, 6.3 | 80.2 %, 10.9 |
| 4,000 (default) | 93.4 %, 8.2 | 38.9 %, 14.6 |
| 5,000 | 87.7 %, 9.6 | 17.6 %, 17.2 |
| 8,000 | 44.0 %, 14.1 | 0.4 %, 19.9 |
| 12,000 | 8.9 %, 18.6 | 0.1 %, 20.0 |
| 16,000 | 0.6 %, 19.9 | 0 %, 20.0 |
| 25,000 and above | 0 %, 20.0 | 0 %, 20.0 |

Gamestate fragments and loss (model; independent losses; retry about 1,000 ms plus RTT 100 ms later):

| capture | engine bytes, fragments | new codec bytes, fragments |
|---|---|---|
| 5min | 7,048, 6 | 4,024, 4 |
| bots | 3,211, 3 | 2,024, 2 |
| solo | 3,212, 3 | 2,023, 2 |

| loss | engine (6 fragments): first-try success, attempts, added join delay | new codec (4 fragments) |
|---|---|---|
| 1 % | 94.1 %, 1.06, 0.07 s | 96.1 %, 1.04, 0.05 s |
| 5 % | 73.5 %, 1.36, 0.40 s | 81.5 %, 1.23, 0.25 s |
| 10 % | 53.1 %, 1.88, 0.97 s | 65.6 %, 1.52, 0.58 s |
| 20 % | 26.2 %, 3.81, 3.10 s | 41.0 %, 2.44, 1.59 s |

Time to send the gamestate fragments at a given rate: at 4,000 bytes per second 2.0 s (engine) against 1.3 s; at 8,000 1.0 s against 0.7 s; at 25,000 0.3 s against 0.2 s.

## 3.11 Worst cases

Script: `worst.py` with `engine_enc.py`. Updated entities per snapshot, round trip exact in every row.

Scenario A (clones of real player-entity updates):

| updated entities | new codec: bytes, fragments | engine: bytes, fragments | ratio |
|---|---|---|---|
| 16 | 182, 1 | 463, 1 | 0.39 |
| 32 | 362, 1 | 926, 1 | 0.39 |
| 64 | 675, 1 | 1,771, 2 | 0.38 |
| 128 | 1,324, 2 | 3,480, 3 | 0.38 |
| 256 | 2,618, 3 | 6,921, 6 | 0.38 |

About 11 bytes per updated player entity against 29. The engine reaches 1,300 bytes at about 45 entities, the new codec at about 115.

Scenario B (every field of every entity random; absolute bound, not reachable in play): new codec 15,362 bytes at 32 entities against 11,396 for the engine (ratio 1.34), 60,140 against 44,751 at 128, 239,274 against 178,060 at 512, 467,054 against 347,509 at 1,000. The new codec would pass `MAX_MSGLEN` (49,152) at about 105 entities against about 140 for the engine.

CPU on scenario A (C benchmark):

| updated entities | method | encode ns | decode ns | bytes | core at 64 clients |
|---|---|---|---|---|---|
| 32 | A. engine | 12,093 | 9,941 | 925.5 | 1.55 % |
| 32 | B. static Huffman per context | 1,758 | 1,287 | 404.5 | 0.22 % |
| 32 | C. rANS 12-bit | 2,876 | 4,076 | 362.5 | 0.37 % |
| 32 | D. adaptive binary | 6,521 | 8,087 | 356.2 | 0.83 % |
| 128 | A. engine | 59,911 | 63,182 | 3,479.7 | 7.67 % |
| 128 | B. static Huffman per context | 9,223 | 5,218 | 1,500.0 | 1.18 % |
| 128 | C. rANS 12-bit | 13,153 | 13,549 | 1,329.3 | 1.68 % |
| 128 | D. adaptive binary | 26,898 | 37,033 | 1,307.7 | 3.44 % |

The engine writer model reproduces 1,499 of 1,500 measured snapshot sizes.

## 3.12 Prediction variants (added 4 November 2026)

Script: `bench5.py`. Ideal bytes, same split, smoothing and per-field mode chooser as `bench2.py`, angles as 16-bit indices (row 9 of 3.2 is the base). Each variant is lossless: the decoder has every input of the prediction before the value it predicts. Not yet confirmed with a real encoder.

| variant | 5min-test mean | p95 | max | bots | solo |
|---|---|---|---|---|---|
| base (row 9 of 3.2) | 136.9 | 235 | 477 | 103.5 | 6.4 |
| T: `pos.trTime`, `apos.trTime` (entities) and `commandTime` (player state) coded against the message's `serverTime` | 128.9 | 223 | 444 | 97.6 | 7.4 |
| P1: positions predicted from the old position and the old velocity over the `trTime` / `commandTime` interval | 127.4 | 222 | 453 | 96.2 | 6.4 |
| P2: as P1 with the mean of the old and the new velocity (velocity fields moved before positions) | 121.3 | 212 | 447 | 91.8 | 6.4 |
| A2: angle indices predicted to second order from the base and the base's base | 129.7 | 222 | 465 | 98.4 | 6.4 |
| T + P2 + A2 | **106.1** | 189 | 410 | 80.7 | 7.4 |

Against the engine (337.6) the combined variant is -68.6 %; against the recommended schema with angle indices it is -22.5 %. The three gains add up almost exactly (8.0 + 15.6 + 7.2 = 30.8 bytes), so they act on different fields.

Details and limits:

- In the captures (1.01 base game module, `g_smoothClients` 1) players are sent as `TR_LINEAR_STOP` with `trTime` = the client's `commandTime` (`BG_PlayerStateToEntityStateExtraPolate`, 08 section 8.5), so `trTime - serverTime` is small and the elapsed time of the prediction is known exactly from `trTime`, which is the first entity field.
- **This depends on the game module, not on the cvar value.** The captures were made with `g_smoothClients 1`, the value servers use; in the 1.01 base module any non-zero value selects the extrapolated form (`E:\Git\jka\jediacademy\codemp\game\g_active.c:3420`, `:3856`). A game module that calls `BG_PlayerStateToEntityState` instead (`g_smoothClients 0`, or some mods) sends players as `TR_INTERPOLATE` and does not update `trTime`. Then variant T has nothing to gain on entities, and P1 / P2 must take the elapsed time from the two snapshots' `serverTime` instead of `trTime` (the decoder knows both). The mode chooser handles the first point; the second needs one more prediction rule. Not measured, because no capture uses such a module.
- The residual of P1 / P2 is the difference of the 32-bit patterns of the value and of the prediction rounded to float32. The prediction is computed in double from float32 inputs and rounded once; an implementation must fix this arithmetic (no fused multiply-add, same operation order) so that both sides compute the same 32 bits.
- T is forced on in this run. It loses on the solo capture (6.4 to 7.4: a human's `commandTime` is further from `serverTime` than a bot's), so in the codec it must be one more mode offered to the per-field chooser, not a fixed rule.
- A2 needs the base's own base. The client has it (it was needed to decode the base) as long as it is still in its snapshot ring, but the server must use the base's real delta source; `bench5.py` uses `seq - 2 * age`, which is right for these captures only (all ages are 1).
- All captures are at 20 snapshots per second with bots; P2 and A2 should gain more at 40 Hz (smaller motion per delta) and less with human aiming. Not measured.

## 3.13 Prediction modes in the real codec (step 1 of [PLAN.md](PLAN.md), 5 November 2026)

Script: `run_pred.py` (whole-message codec of `codec2.py`, run as in `run_full.py`), `run_pred.py pi --fuzz`, `run_pred.py pf --fuzz --loss`. Real bytes; tables trained on the first 60 % of the 5-minute capture.

The modes are offered to a per-field choice made on the training part: a mode is kept when it saves more than 8 bits over the whole training part. Of 64 candidates, 11 were kept: `pos.trTime` against `serverTime` (entities); positions `pos.trBase[0..2]` (entities) and `origin[0..2]` (player state) predicted from the mean velocity; angles `apos.trBase[0..1]` (entities) and `viewangles[0..1]` (player state) predicted to second order. `commandTime` against `serverTime` was not kept.

Snapshot messages (without server commands), mean real bytes; p95 and max for the 5-minute capture:

| configuration | 5min (p95, max) | bots | solo |
|---|---|---|---|
| study codec (3.7) | 139.1 (240, 744) | 106.5 | 9.3 |
| same, fields reordered for prediction (velocity before position) | 138.8 (239, 754) | 106.4 | 9.3 |
| + rANS initial-state trick | 137.3 (238, 753) | 105.1 | 8.1 |
| + prediction modes, float arithmetic (`pf`) | **106.8** (191, 650) | 82.3 | 8.0 |
| + prediction modes, integer arithmetic (`pi`) | **107.3** (191, 650) | 82.7 | 8.0 |

Server commands + snapshot: 210.9 to 178.4 bytes (`pi`). Gamestate: 4,024 to 4,014 bytes. `svc_mapchange`: 9 to 6 bytes.

Correctness: 0 round-trip failures on the 8,842 real messages in every configuration; 0 failures on 18,150 synthetic messages (`fuzz.py` seeds 1 to 5, uniform tables) with both arithmetics; 0 failures in the 20 runs of the loss and latency grid (`pf`).

Loss and latency with prediction (`pf`, same grid and training as 3.9; mean real bytes per snapshot message, the value without prediction from 3.9 in brackets):

| loss | RTT 50 ms | 100 ms | 200 ms | 400 ms |
|---|---|---|---|---|
| 0 % | 118.3 (152.2) | 150.9 (183.8) | 206.7 (235.0) | 291.5 (322.4) |
| 10 % | 123.4 (156.2) | 155.0 (187.0) | 209.6 (237.5) | 292.6 (323.2) |
| 20 % | 127.9 (159.7) | 158.4 (189.8) | 213.6 (241.3) | 294.0 (324.2) |

The gain in bytes stays about the same with the base age (-34 bytes at age 1, -31 at age 8), so in relative terms it shrinks; acknowledgement latency (step 9 of the plan) matters as much as before.

Decisions taken from these numbers:

- **Integer arithmetic for the position prediction.** It costs 0.5 byte per snapshot more than the float version and does not depend on how a compiler handles floating point. Definition (both sides): `r(x) = floor(x * 65536 + 0.5)` on the float32 inputs widened to double (exact); `p = r(old) + floor((r(v_old) + r(v_new)) * dt / 2000)` in 64-bit integers; prediction = `(float)(p / 65536.0)`; no prediction when an input is not finite or has a magnitude of 2^24 or more, or when `|dt|` is 65,536 ms or more. The residual is the signed difference of the 32-bit patterns of the value and of the prediction.
- **Elapsed time:** `trTime` difference for `TR_LINEAR_STOP` entities; `serverTime` difference between the message and its base for `TR_INTERPOLATE` entities (not exercised by the captures); `commandTime` difference for the player state. No prediction for other trajectory types, for entities coded from a baseline, or when the base state is all zero.
- **Second-order angles** use the base's own base. The client keeps, with each snapshot, the snapshot it was decoded against (`Ref` in `codec2.py`); the server keeps the same with what it sent.
- **Two rules found by the fuzz test**, which a C++ implementation must keep because both sides must take the same decision: the vehicle state of an older snapshot exists only if that snapshot's player state had a vehicle; "the base state exists" is tested on its values (missing and zero fields are the same state), not on the presence of keys.
- **The rANS initial-state trick** (the encoder starts at state 1; the decoder stops reading at the end of the buffer) saves 1.5 bytes per snapshot (1.2 on the solo capture, 3 on `svc_mapchange`).
