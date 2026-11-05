# 6. Verification guide: how to re-check this work, where it may be wrong, and what was not tried

This document is for a reviewer, human or model, who is asked to re-verify the study and to look for better solutions. It lists the claims that matter, how much confidence they deserve, how each can be re-checked, the weak points of the evidence, the mistakes already made and corrected, and ideas that were not explored. Read it before the other documents if your task is to challenge the work.

## 6.1 How to approach the re-verification

1. Re-run the cheap checks first (section 6.2, claims C1 to C6): they validate the foundation (the decoder and the corpus). If the decoder is wrong, everything after it is wrong.
2. Re-run the size experiments with different seeds, splits and smoothing values to see whether the conclusions depend on a choice (claims C7 to C12).
3. Audit the code readings against the source (02): each has a `file:line`.
4. Try to beat the proposed schema (section 6.7). The study measured one design; it is not claimed to be optimal.
5. Be suspicious of anything that rests on bots-only, single-map, single-human data (section 6.5).

Tools and commands are in [07-reproduction.md](07-reproduction.md).

## 6.2 Claims, confidence, and how to re-check each

Confidence: **high** = checked several independent ways; **medium** = checked once or relies on a stated assumption; **low** = reasoned or modelled.

| id | claim | evidence | confidence | how to re-check |
|---|---|---|---|---|
| C1 | The Python decoder reproduces the engine's read path exactly | every message ends within 1 to 8 bits of its recorded length; replaying the call list reproduces the measured bit count of 2,554 test snapshots with 0 differences | high | `jkadec.py`, `bdata.py`, `bench1.py` (prints "L0 replay mismatches"); change one field width and watch the decode fail |
| C2 | The field tables used are the right ones for the 1.01 retail binary | same as C1 (a wrong table desynchronises the stream) | high | as above |
| C3 | The captured data are complete server messages before the XOR encoding and without the trailing `svc_EOF` | read in the proxy hook and `sv_net_chan.cpp:140`; decode lengths | high | read `NetCapture.cpp` in the proxy repository |
| C4 | The engine writer model reproduces the sizes of real snapshots | 1,499 of 1,500 real snapshots (06, section 6.4) | high | `engine_enc.py` |
| C5 | The codec round-trips exactly on all real messages and on the synthetic corpus | 8,842 real messages and 27,225 synthetic, 0 failures; 517,000 real floats and 400,000 random floats | high for the covered branches | `run_full.py`, `fuzz.py` (several seeds), `rt_float.py`; add new generators for branches not covered |
| C6 | Remaining equivalence caveat: `-0.0` | the engine turns it into `+0.0`; the codec does the same; one real snapshot differs in the engine model because of a hidden server-side bit pattern | medium | search the captures for snapshots where the engine model and the measured bits differ |
| C7 | Replacing only Huffman gains 4.6 % | ideal order-0 coder on the engine's byte stream, test part: 321.9 against 337.6 | high | `bench1.py` |
| C8 | The model, not the coder, carries the gain | rows 4 to 9 of the size table: 227.3, 168.5, 136.9 | high on this data | `bench1.py`, `bench2.py` |
| C9 | The coder family hardly matters on trained tables | ideal 168.5; rANS 12-bit 169.1; real rANS 172.0; adaptive binary 168.4; Huffman per context 186.1 | high | `bench3.py`, `bench4.py` |
| C10 | Adaptive coders and generic compressors do not beat the engine's trained Huffman on these packets | qbc 363.2 and nibrans 372.5 against 369.7; zstd and deflate within 2 % of 337.6 | high on this data | `bench_libs.cpp`, `bench4.py` |
| C11 | 99.9 % of angles are already on the 16-bit grid | 60,676 of 60,682 etc. (03, section 3.6) | high | the grid test in section 3.6; re-count |
| C12 | Angles as indices are lossless and save 19 % | round trip on 165,554 real indices; size 168.5 to 136.9 | high | `rt_float.py`, `bench2.py` with `ANG16=True` |
| C13 | CPU is not a constraint | C benchmark: 0.12 to 0.25 % of a core at 64 clients for the new codec, 0.92 % for the engine | medium: symbol construction not included, my coders are simple, one machine | `tools/cpubench`; add the cost of building symbols from real `clientSnapshot_t` |
| C14 | Latency, not loss, drives size | `sim_loss.py` grid | medium: independent losses, no bursts, acknowledgements never lost, constant RTT | change the loss model (bursts, ack loss, jitter) |
| C15 | Default `rate` 4000 makes the engine delay 93 % of snapshots | formula read at `sv_snapshot.cpp:623-690` applied to real message sizes | medium: the effect of the lower rate on delta age is not fed back; rounding to 50 ms frames is an assumption | simulate the closed loop |
| C16 | Positions must stay exact | reading of `cg_predict.c`, `cg_ents.c` | medium: other cgame files only counted, not read | read `cg_players.c`, `cg_weapons.c`, `cg_event.c`; test prediction error counters (`cg_showmiss`) with rounded positions |
| C17 | The gamestate shrinks from 7048 to 4024 bytes | codec measurement, strings coded order-0 | medium: strings of this gamestate are in the training data | train without it, or test on another map |
| C18 | Expected gamestate loss recovery times | model only | low | measure on a live server with loss emulation |
| C19 | Worst case is 1.34 times the engine's size | scenario B | high as a bound; not a realistic case | `worst.py` |
| C20 | No licence conflict for ryg_rans, FSE, qbc, nibrans | licence files read | medium: legal reading of file headers across CleanJK is a grep | read the headers file by file |
| C21 | Multi-human traffic behaves like bot traffic | assumed by the cloning scenario | **low, untested** | record two or more humans |
| C22 | Prediction (time fields against `serverTime`, positions from velocity, second-order angles) saves a further 22 % (136.9 to 106.1 ideal bytes) | `bench5.py`, ideal bytes only | medium: no real encoder yet, bots only, 20 Hz | port the three modes into `codec.py` and check the round trip and real bytes |
| C23 | OpenJK (this repository) is wire-compatible with the 1.01 server except for the pilot player state | tables and Huffman frequencies compared; captures decoded with OpenJK's tables (09) | high | `jkadec.py` with `SRC` set to `codemp/qcommon/msg.cpp` |

## 6.3 The synthetic corpus: what it covers and what it proves

`fuzz.py` creates structurally valid streams. It covers: every operation type; normal, pilot and vehicle player states (the pilot state's untransmitted fields are generated to equal the old values, as the engine would leave them); zero, minimum, maximum, integral, angle-grid, NaN and infinite values; entities added, changed, removed, equal to baseline; base ages 1 to 11 and no base; downloads with and without the block-0 size; random per-field absolute or delta modes; uniform and trained tables. It proves the exactness of encoder and decoder on those inputs. It does not prove anything about sizes.

Ideas to extend it: states built from real snapshots with random mutation of a few fields (closer to real distributions and good for finding table-boundary bugs); streams with a gamestate in the middle (map change); error injection (truncation, bit flips) to test decoder robustness; very large entity counts (the worst-case study reached 1,000).

## 6.4 Determinism checks still to do

The study ran on one machine with one Python and one MSVC build. Before shipping:

- check the angle decode `(float)(k * (360.0 / 65536))` for every `k` in the signed range used (about -131,072 to 131,072) against the Python result on MSVC, GCC and Clang, x86 and x64, with and without `/fp:fast`-style options (the product is exact in double, so there should be no difference; verify, do not assume);
- check that no part of the coder uses floating point beyond that, and that the probability tables are produced by integer code (the study computed them with Python floats; the C++ implementation must use fixed rules for normalisation, and the encoder and decoder must read the same table file);
- check `signed` arithmetic: the delta wrap for 32-bit fields relies on modular arithmetic;
- check behaviour on big-endian assumptions (none were made, but the engine has byte-order helpers).

### Engine writer model mismatch (C4 detail)

`engine_enc.py` differs from the measured bits on one snapshot of 1,500 (sequence 984): the engine wrote a larger last-changed index for one entity than the decoded states imply. The probable cause is a server-side field that differs in bits (`-0.0` against `+0.0`) while both decode to the same value, so the engine counts it as a change. It is harmless for the study but shows that "equal values" is the observable criterion and server memory can differ.

## 6.5 Weaknesses of the evidence, in order of importance

1. **One map, one mode, one human, bots.** The captures are `mp/ffa3`, FFA. Human aiming, saber combat, force powers, vehicles, team modes and larger maps are absent from the data. The size results may be optimistic on varied content (tables fit this content) and the entity-count scenarios clone bot behaviour. The control captures are other games on the same map.
2. **Training and test come from the same sessions.** The time split avoids sample leakage, but game state is autocorrelated (same map, same bots, same players). The gamestate result is optimistic (its strings are in the training data).
3. **The idealised sizes use smoothed static counts.** The real rANS round trip confirms them on 400 packets and in the full codec, but the smoothing constant was tuned on the same session's validation slice.
4. **The per-field absolute/delta choice and the field contexts were found on the training part** and applied unchanged to the test part; a different choice rule could do better (see 6.7).
5. **CPU numbers come from one machine** and from simple implementations, and exclude building symbols from the engine's structures.
6. **The network-layer results are models, and they use 1.01 defaults** (`sv_fps` 20, `rate` 4000). OpenJK's upstream defaults (`sv_fps` 40, rate forced to 50000) change the base age for a given RTT; this branch sets `sv_fps` 20 (09, section 9.2); no capture at 40 Hz exists. Loss is independent, acknowledgements are lossless, the rate model assumes 50 ms server frames, and the effect of fewer snapshots on delta age is not fed back. No live test was done.
7. **Client code coverage.** Only `cg_predict.c` and parts of `cg_ents.c` and `cg_draw.c` were read. The position-rounding decision depends on the rest (`cg_players.c` has 122 uses of `lerpOrigin`).
8. **Transport libraries were not evaluated.** The recommendations in 04, section 4.7 are reasoning from the measured behaviour of the existing netchan.
9. **The client-to-server direction was not studied.** `usercmd` packets are small but frequent; their share of the total bandwidth is unknown.
10. **No test of the retail binary other than through the captures.** The captures prove the read path matches; the write path was inferred from the source plus the engine-writer model check.

## 6.6 Mistakes made during the study and corrected (so you can watch for similar ones)

| id | mistake | how it was found | fix |
|---|---|---|---|
| M1 | The table extractor matched lines commented out with `//`, which shifted field indices after about index 75 of the player state | decoding a later message failed with a strange entity order | strip `//` and `/* */` comments before the regular expression; check table lengths (before the fix the three player-state tables had 152, 152 and 80 entries; after it 137, 140 and 69, and the pilot table length matters for the "minus 82" rule) |
| M2 | Capture started after joining: no gamestate, so baselines and bases were missing | 1,105 undecodable messages | the capture cvar must be on before the map loads |
| M3 | "Speed does not matter" in the first reasoning | the owner corrected it | speed was then measured; it turned out not to be a constraint, but as a result of measurement, not assumption |
| M4 | Estimated CPU of simple coders at 2 to 7 % of a core | C benchmark | measured 0.03 to 0.25 % |
| M5 | Angle index delta computed modulo 16 bits | review of negative angles (`viewangles[PITCH]`) | use an unbounded integer delta; the cost estimate was recomputed (136.7 became 136.9) |
| M6 | Banker's rounding in Python `round` against floor in C | review of the specification | both sides define `floor(x + 0.5)` |
| M7 | A comparison treated an empty vehicle state `{}` as absent | the synthetic corpus reported 9 failures | compare with `is not None` |
| M8 | The worst-case generator produced a random non-zero `m_iVehicleNum` without a vehicle state | the first run of scenario B failed | generator now sets `m_iVehicleNum` to 0 |
| M9 | An unsupported explanation: the 152.2 against 139.1 bytes difference was attributed to mixed-age tables | review before the documents were written | the two figures are not comparable; the statement was removed from the reports |
| M10 | A sentence reported two delays as if the new codec were worse | the owner asked | the pairs were engine first, new codec second; rewritten |
| M11 | Shell quoting in this environment: Windows paths with backslashes were mangled (`\n`, `\t`, `\r`) when written through Python in a shell here-document | report text with line breaks inside a file name | use `chr(92)` or raw strings when writing paths into generated text; look at the output |
| M12 | A compiler downloaded from PyPI (zig) was refused by the sandbox and not worked around | permission denial | the owner compiled the C benchmarks; do not rely on running downloaded executables |
| M13 | A first benchmark of nibrans failed to compile as C++ (it uses C99 compound literals) | the owner's compiler output | its implementation is compiled as C |
| M14 | The `K=18` class alphabet of the angle delta in the cost estimate was too small for the unbounded delta | found while writing the real codec (K=24 needed) | the real codec uses 24 |
| M15 | The documents said a pilot state sends "the first 122 fields"; `140 - 82` is 58 | re-reading the code (09) | corrected in 01 and 02; the code (`codec2.py`) was right |

## 6.7 Ideas that were not explored, with a rough sense of potential

These are suggestions, not findings. Sizes quoted are the study's, so potential gains are relative to them.

**Within the entropy-coding and modelling scope**

1. **Time fields relative to the snapshot time.** *Measured since (03, section 3.12, variant T): -8.0 bytes on 5min-test, but worse on the solo capture; make it a per-field mode.* `pos.trTime` costs 8.1 bytes per snapshot, `apos.trTime`, `time`, `time2` and the player state's `commandTime` are similar. Coding them as differences from the message's `serverTime` instead of from the entity's own previous value could save a large part of these bytes. Untested; the per-field mode chooser only compared "absolute" and "delta from the old value".
2. **Motion-model prediction.** *Measured since for positions and angles (03, section 3.12, variants P1, P2, A2): -15.6 and -7.2 bytes.* Positions (about 40 of 137 bytes after the angle trick) are coded as differences of float bit patterns, which leaves roughly 20 bits per coordinate. Predicting the next position from `trBase`, `trDelta` and the previous displacement, then coding the residual, may shrink them, and may work with exact floats if the residual is small in the pattern domain. The same for angles (angular velocity).
3. **Temporal and cross-field contexts.** The previous delta class of the same field of the same entity; the sign and size of the previous coordinate's delta; the entity's animation state. Quick test: add the context to the token and re-run `bench2.py`.
4. **Joint coding.** `pos.trBase[0..2]`, `pos.trDelta[0..2]` and `apos.trBase[0..2]` move together; code a vector as one symbol or condition each component on the previous one.
5. **Parametric value models.** Replace empirical class tables by parametric families (geometric / Laplace mixtures) with a few trained parameters per context: smaller tables, smoother behaviour on unseen content.
6. **Static context mixing.** Mix two or three context predictions with weights trained offline and frozen; no state crosses packets. Cost per decision is higher than plain tables; the study did not measure it.
7. **Removing the coder flush.** A rANS state can be initialised with data bits (the bits-back trick), recovering part of the 4-byte final state (3.5 bytes measured) in every packet. FSE already pays 2 bytes less.
8. **Text.** Server commands (10 % of messages, up to 1,804 bytes) and the gamestate strings use an order-0 model. An order-2 trained model, or a shared dictionary of configstrings known from the game files (model and sound names), should cut the gamestate further.
9. **Field order.** Coding the most informative fields first (as `eType` was) and conditioning later fields on them; not searched.
10. **Entity-to-entity deltas.** New entities of the same type could be coded against a typical entity instead of an all-zero baseline.

**Outside the entropy-coding scope (likely larger)**

11. **What to send.** Per-client priority, bandwidth budgets, distance-based update rates, interest culling; the study only coded updates well, it did not choose them.
12. **Snapshot rate and tick structure.** Higher server rates with cheaper deltas; different rates by entity class.
13. **Transport design.** Ack bitfields, reliable fragment recovery, authenticated packets; and header compression (the 32 bytes of header exceed the payload in the solo case).
14. **Client to server.** `usercmd` coding and its rate.

## 6.8 Questions a reviewer should try to answer

- Does the conclusion "the coder family does not matter" survive a richer model (items 3 to 6 above)? A stronger model class could change which coder is best.
- Does the angle-index trick still hold on human input (view angles come from `usercmd` shorts, so yes by construction, but check the ratio of on-grid values in human captures)?
- Is the claim that rounding positions would affect prediction strong enough to rule out rounding for every entity type, or can some types be rounded safely? Read the rest of cgame and test.
- Are there fields whose width in the tables is wasteful and whose real range is much smaller (the study tokenised by value, so this is already exploited, but a schema review could simplify)?
- Is the single-state FSE across tables correct in general, or only for the same table log? (It round-tripped in the benchmark; the library does not document it.)
- How much of the 32-byte network header can be removed or shared?

## 6.9 What published work says (web reading, 4 November 2026)

Read from articles and talk summaries, not reproduced. Each line says what it implies for this design.

| source | what it does | implication here |
|---|---|---|
| Glenn Fiedler, "Snapshot Compression" (gafferongames.com) | Delta against the newest acknowledged baseline; changed objects sent either as delta-coded indices or as one changed bit per object, chosen per snapshot with a header bit; bounded small / large deltas found by a greedy search over captures; quantised positions and "smallest three" quaternions; he reports that an arithmetic coder by Fabian Giesen beat his best bit-packing by 25 % | Same structure as the recommended schema (acked base, gap list of changed fields, trained distributions instead of hand-picked ranges). His data is quantised by design; JKA's prediction needs exact values (02, section 2.8), so quantisation stays out unless the game itself quantises (next rows) |
| Fiedler, "State Synchronization" (**excluded by the owner's goal 1 in the README: a grid coarser than float32 loses precision, even when applied to the server's own state**) | Quantises the state **on both sides** (4096 values per metre, 15-bit quaternion components) so that both simulations run on the same values; falls back to absolute coding when the baseline is too old | A lossless way to get quantisation gains: quantise the **server's own state** after each move (Quake 3 did this with `SnapVector` in `BG_PlayerStateToEntityState(..., qtrue)`; JKA calls it with `qfalse`, positions are not snapped). Then the client receives the server's exact values, and rounding costs nothing in prediction. It changes movement physics, which matters for racing (09, section 9.3). Not measured; the 1/16-unit rounding rows of 3.6 give the order of the size gain (136.9 to about 109) |
| RAD Oodle Network (radgametools.com, cbloom rants 2013 and 2016) | Static model trained offline on captured packets, shipped with the game, shared by all channels, each packet compressed independently; claims 25 to 80 % on packets that are already delta-coded and bit-packed; models of 4 to 8 MB; retrain when the game changes | The same principle as the recommendation (static trained tables, no cross-packet state, retrain with the content). Proprietary, so not usable here. Its gain on bit-packed streams comes from a large context model; the per-field model of this study plays that role with about 7 KB of non-zero entries |
| Burtscher and Ratanaworabhan, FPC (IEEE TC 2009); fpzip; the FCBench survey (VLDB 2024) | Lossless float compression by prediction then residual: FPC XORs value and prediction and counts leading zero bytes; fpzip maps both floats to integers and codes their difference | `bench5.py` P1 / P2 use the fpzip form (difference of 32-bit patterns), which keeps the residual small when prediction and value share sign and exponent; an XOR residual is the alternative to try if sign changes around zero turn out to be frequent |
| Jarek Duda on encode.su; Fabian Giesen, "rANS in practice" | The final 32-bit rANS state costs about 4 bytes per stream; it can be recovered by starting the encoder from a state that carries data bits (start at 1 and encode until the state passes the renormalisation bound), or used as a free checksum (start from a known state, check it at the end of decoding) | The real rANS of the study costs 3.5 bytes above the ideal per packet (03, section 3.4). The initial-state trick should recover most of it: about 3 % of a 106-byte packet, more on idle packets. The checksum variant gives an integrity check of the decode for free. Not implemented |
| David Aldridge, "I Shot You First: Networking the Gameplay of Halo: Reach" (GDC 2011); Tribes networking model (GDC 1999) | Per-object, per-client priority (distance, direction, size, speed, damage), packets filled in priority order under flow control; profiling recorded in replays; Reach used 20 % of Halo 3's bandwidth | Confirms that "what to send" (6.7 item 11) is a larger lever than coding |
| Tim Ford, "Overwatch Gameplay Architecture and Netcode" (GDC 2017) and summaries | Server-authoritative ECS with heavy prediction; floats quantised to low precision so client and server do not drift; every unacknowledged input resent in each packet | Same remark as State Synchronization: quantisation is safe only when the simulation itself uses the quantised values. The "resend unacknowledged inputs" scheme is what `cl_packetdup` approximates |
| Unreal Engine Iris (Epic forum and bug tracker) | Serialisation split into quantise, then serialise the quantised form; delta compression per class against baselines | No new technique for this study; quantised form plus per-class baselines is the same idea as per-type contexts |

Sources: [Snapshot Compression](https://gafferongames.com/post/snapshot_compression/), [State Synchronization](https://gafferongames.com/post/state_synchronization/), [Fiedler's delta compression gist](https://gist.github.com/gafferongames/bb7e593ba1b05da35ab6), [Oodle Network](https://www.radgametools.com/oodlenetwork.htm), [Oodle packet compression for UDP](https://cbloomrants.blogspot.com/2013/11/11-14-13-oodle-packet-compression-for.html), [Oodle Network usage notes](http://cbloomrants.blogspot.com/2016/01/oodle-network-usage-notes.html), [FPC](https://userweb.cs.txstate.edu/~burtscher/papers/tc09.pdf), [FCBench](https://www.vldb.org/pvldb/vol17/p1418-tao.pdf), [rANS in practice](https://fgiesen.wordpress.com/2015/12/21/rans-in-practice/), [encode.su multithreaded rANS overhead](https://encode.su/threads/3949-Multithreaded-rANS-Compression-Overhead), [encode.su AVX-512 interleaved rANS](https://encode.su/threads/2607-AVX-512-and-interleaved-rANS), [Halo: Reach GDC Vault](https://gdcvault.com/play/1014346/I-Shot-You-First-Networking), [Wolfire summary](https://www.wolfire.com/blog/2011/03/GDC-Session-Summary-Halo-networking/), [Overwatch GDC Vault](https://www.gdcvault.com/play/1024001/-Overwatch-Gameplay-Architecture-and), [Iris NetSerializers thread](https://forums.unrealengine.com/t/iris-why-are-netserializers-so-complex/2585061).
