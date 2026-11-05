# 1. Methodology

This document describes how the study was carried out, in the order it happened, so that a reviewer can judge each step and re-run it. Results are in [03-results.md](03-results.md); the facts read in the code are in [02-engine-code-findings.md](02-engine-code-findings.md).

## 1.1 Starting point and initial hypotheses

The study began from a hand-off note written after an earlier analysis session (kept outside the repository). Its main points, and what happened to each:

| initial statement | outcome |
|---|---|
| Replacing only Huffman gains "a few percent to about 15 %" (not measured) | Measured: 4.6 % at best (section 3.2 of the results). The low end was right. |
| The gain comes from the model (context per field, delta base as context), quantisation, priority, budget, interest culling | Model gain confirmed (about 50 %). Quantisation analysed and rejected for positions (prediction code). Priority, budget and culling were **not** studied. |
| A heavy context-mixing coder (lpaq family) would cost 30 to 130 % of a core at 32 clients; a simple adaptive or static coder 2 to 7 % | Wrong by one to two orders of magnitude for the simple coders: measured 0.03 to 0.2 % at 32 clients. Context mixing was not measured. |
| An adaptive model whose state persists across packets desynchronises on the first lost UDP packet | Accepted as a design rule. All measured coders reset per packet or use static tables. A persistent-model variant of nibrans is reported only for information. |
| Integer-only arithmetic in the coding path so that client and server stay bit-exact | Applied. The only floating-point step is the decoding of a 16-bit angle index, which is exact (see 04, section 4.3). |
| Candidate libraries: ryg_rans, FSE, Draco `ans.h`, fsc, nibrans, Turbo-Range-Coder, constriction, qbc, OpenZL | Measured: ryg_rans, FSE, qbc, nibrans. Excluded by licence: Draco, fsc (Apache-2.0), Turbo-Range-Coder (GPLv3). Not tested: constriction (Rust), OpenZL (framework, licence file not found). |
| Transport candidates: yojimbo, GameNetworkingSockets | **Not evaluated.** Only the existing netchan was analysed. |

## 1.2 Capturing real traffic

No real traffic was available, so a capture tool was written in the server module proxy (`E:\Git\jka\JKA_YBEProxy`). That proxy hooks the original 1.01 dedicated server at run time.

- Hook point: `Proxy_SV_SendMessageToClient` (`src/JKA_YBEProxy/Engine/Hooks/server/sv_snapshot.cpp`), just before `SV_Netchan_Transmit`. At that point the message is complete and has not yet gone through `SV_Netchan_Encode` (the XOR "encryption"). The trailing `svc_EOF` byte is appended later, in `SV_Netchan_Transmit` (`server/sv_net_chan.cpp:140`), so captured messages do not include it.
- Output: `ybeproxy_netcapture.bin` in the server working directory, written in append mode through a 256 KB buffered `FILE`, enabled by the cvar `proxy_sv_captureNet 1`. The format is described in 06, section 6.2.
- Only messages sent to human clients are captured. Bots are never sent snapshots (`SV_SendClientSnapshot`, `server/sv_snapshot.cpp:768`), so their view of the world is not available.
- The capture must be switched on **before** the map loads, because the gamestate (which carries the entity baselines the decoder needs) is only sent at connection or map load. A first capture made after joining contained 1,105 undecodable messages for that reason.
- The capture code was written without a compiler in the assistant's environment; the owner compiled and ran it.

Captures used (all map `mp/ffa3`, game type 6 = FFA, `sv_fps` 20, `sv_maxclients` 28 with bots for two of them):

| file | what | duration | messages | decodable |
|---|---|---|---|---|
| `ybeproxy_netcapture_bots_5mins.bin` | one human plus bots filling 28 slots | 357 s | 7,101 | all (starts at the gamestate) |
| `ybeproxy_netcapture_with_bots.bin` | same set-up, another game | 82 s | 1,605 | all |
| `ybeproxy_netcapture_no_bots.bin` | one human, no bots | 79 s of server time | 1,240 | 136 (capture started after joining) |

Checksums are in 06, section 6.5.

## 1.3 Decoding the captures offline

The captured bytes are the engine's own bitstream (bits and Huffman-coded bytes mixed). To compare coders, the values of each field are needed, so a decoder was written in Python (`tools/python/jkadec.py`) as a direct port of the engine's read path.

1. **Huffman tree.** `huffman.cpp` builds an adaptive tree at start-up by calling `Huff_addRef` `msg_hData[i]` times for each byte value `i` (the active table is at `qcommon/msg.cpp:2958`). The tree is then used statically for decoding. The port reproduces the node structure exactly (the C free-list of pointer cells is replaced by fresh Python cells, which is equivalent).
2. **Bit layout.** `MSG_ReadBits` (`msg.cpp:211`) reads the low `bits & 7` bits raw, least significant bit first, then one Huffman symbol per remaining byte. The port keeps the same order, and the same sign-extension quirk (the sign bit is tested with the width reduced by the raw part; harmless because every negative width in the tables is a multiple of 8).
3. **Field tables.** The 132 entity fields, the player-state tables and their widths are **extracted from `msg.cpp` by regular expression**, not retyped. This once failed (see 05, mistake M1): the expression also matched lines commented out with `//`. Comments are now removed first.
4. **Which build options are active.** `_OPTIMIZED_VEHICLE_NETWORKING` is defined (`game/q_shared.h:2154`) and `_ONEBIT_COMBO` is not (`q_shared.h:2150`, commented out). So the player-state tables used are the optimised ones (normal 137 fields, pilot, vehicle) and each non-vehicle player state starts with a pilot bit.
5. **Message structure.** `svc_*` operations (`qcommon/qcommon.h:233`), `CL_ParseSnapshot`, `CL_ParsePacketEntities`, `CL_ParseGamestate` are followed as written. State needed for deltas (previous snapshots, entity baselines) is kept per client.
6. **Validation of the decoder.** A decoded message must end within 1 to 8 bits of its recorded length (the engine writes `cursize = (bit >> 3) + 1`). On the main captures all 7,101 + 1,605 messages satisfy this, except one 3-byte `svc_mapchange` message in each file that was first reported as an error and is now handled. A wrong table, wrong width or wrong order would desynchronise the bitstream long before the end, so this test is strong evidence that the tables and the read path are right. It also means the retail 1.01 binary matches the source tree on these paths.
7. **Header bookkeeping.** `lastClientCommand` (message acknowledge), `serverTime`, the delta byte, the flags and the area-mask bytes are recorded for every snapshot.

## 1.4 From decoded messages to a comparison corpus

`bdata.py` and `fullload.py` build the corpus:

- Per snapshot: the full player state, the full entity list (number plus all field values), the baselines in force, the header values, and the exact list of `MSG_ReadBits` calls (tag, width, value) in order. The call list lets the current encoding be priced exactly and rebuilt.
- Two views: "snapshot-only" messages (6,384 + 1,439 + 126 snapshots, used for the size comparisons) and "full" messages with server commands, gamestate, `svc_mapchange` (8,842 messages, used for the whole-message codec).
- About 10 % of snapshots in the main capture also carry server commands; they are excluded from the snapshot-only size tables (the same packets are excluded for every method) and included in the whole-message results.

## 1.5 Train and test split

- The 5-minute capture is split in time: the first 60 % trains the models, the last 40 % tests. Models never see the test packets.
- The two other captures are used as controls and are always evaluated with models trained on the 5-minute capture only. They are other games on the same map, not other maps.
- Smoothing constants of the probability tables (`alpha`) were chosen on a validation slice taken from the training part (75 / 25), never on the test part.

## 1.6 What "cost" means in each comparison

| name | meaning |
|---|---|
| measured / engine | the bits the engine actually wrote, read back from the capture (sum of Huffman code lengths plus raw bits); replayed from the call list it matches exactly (0 differences on 2,554 test snapshots) |
| ideal | `sum(-log2 p)` under smoothed static counts trained on the training part, plus raw bits; the limit a static arithmetic or ANS coder approaches |
| rANS 12 / 15-bit | the same with probabilities normalised to 2^12 or 2^15 (min frequency 1), as an actual rANS or tANS coder would use |
| real | bytes produced by an actual encoder (a 32-bit rANS written in Python, whose output is decoded back) |
| per-context Huffman | canonical Huffman code lengths computed from the same counts, per context |
| adaptive binary | each symbol binarised MSB first, node probabilities initialised from the trained counts and adapted inside the packet with a shift-5 update, reset at every packet |

The ideal numbers are produced for many variants because they are cheap; real encoders were then written for the chosen design to check the ideal figures.

## 1.7 Experiments in the order they were run

1. **Layout-preserving ladder** (`bench1.py`): same bitstream layout as the engine, only the entropy coding changes. Replay of the engine (L0), static order-0 byte model (L1), canonical Huffman on bytes (L1h), static model per field (L2).
2. **New schema** (`bench2.py`): re-tokenise the same values with a different layout: delta against a base snapshot, no `lc`, changed fields as gaps or as flags, per-field absolute or delta mode chosen on the training data, class-plus-mantissa coding of numbers, float kinds, `eType` first, entity-number gaps, header deltas. Variants: gap or flag coding, value contexts by entity class, float quantisation, base age 1/2/4/8.
3. **Coder families** (`bench3.py`): the same symbol streams under ideal, rANS 12/15 bits, per-context Huffman and adaptive binary.
4. **Generic compressors and real rANS** (`bench4.py`): zstd with a trained dictionary, deflate with a preset dictionary, lzma on the engine's raw (non-Huffman) layout; a real 32-bit rANS encoder and decoder with round trip on 400 packets.
5. **CPU benchmark in C** (`tools/cpubench/bench.c`): own implementations of the engine Huffman, a static per-context Huffman, a 12-bit rANS and a binary adaptive coder, replaying real symbol streams exported by `dump_bench.py`. Timing covers coding of already-built symbol lists only.
6. **Third-party library benchmark** (`bench_libs.cpp`): ryg_rans and FSE (one tANS state shared across contexts by switching tables) on the new-schema streams; qbc and nibrans on the engine byte stream. Source commits are listed in 06, section 6.4.
7. **Precision study**: what the engine sends for floating-point fields, which values lie on a grid, size with angles as 16-bit indices, size with positions rounded (rejected afterwards, see 04), and a read of the client prediction code.
8. **Full codec** (`codec.py`, `codec2.py`): a complete encoder and decoder of the new schema (header, player state, entities, server commands, gamestate, downloads, pilot and vehicle states), static rANS, one stream per message, and an exact round-trip check on real and synthetic data.
9. **Network behaviour** (`sim_loss.py`, `frag_rate.py`): loss and latency simulation, netchan fragmentation, `rate` limiting, gamestate retransmission.
10. **Worst cases** (`worst.py`): crowded and adversarial snapshots, compared with a model of the engine's own writer (`engine_enc.py`).

## 1.8 The round-trip test of the codec

For each message the harness plays both roles, with separate state:

- the **server** side encodes the message from the real state, the base it chose and the baselines;
- the **client** side decodes the bytes using only its own stored copies of the earlier decoded snapshots, the baselines it decoded from the gamestate and the header of the base;
- the decoded header, player state(s) and entity list are compared field by field with the original (all fields of each table, taken by value, not by key presence), and the decoded header record (ack, time, last server-command number) is compared with the encoder's.

"Equal" means equal to **what the current client would see**, not to the server's memory: for instance a `-0.0` float is received as `+0.0`, and a field is truncated to its declared width. This is the right equivalence for a protocol replacement, and it explains one known mismatch in the engine-writer model (05, section 5.4).

## 1.9 Synthetic data

The captures contain no vehicle, no pilot player state, no download, no snapshot without a base and no base older than one snapshot. `fuzz.py` therefore generates messages that are valid under the engine's own rules and exercises every branch: random values over the whole domain of each field (zero, minimum, maximum, integral floats, angle-grid values, NaN, infinities), entities added, changed, removed or equal to their baseline, pilot player states (only the first 58 fields of the pilot table, `size - 82`, are sent, as in the 1.01 engine; OpenJK sends all 140, see 08), vehicle player states, downloads, server commands, `setgame`, `nop`, `svc_mapchange`, gamestates, base ages 1 to 11 and no base. Two table sets are used: uniform (empty) and trained on half of the generated streams.

**Synthetic data proves exactness. It says nothing about size or speed**, because its values are random and unrelated to real play. It was used for size only in the worst-case study, where an upper bound is wanted (03, section 3.11).

## 1.10 Loss and latency simulation

`sim_loss.py` replays the real 5-minute stream with a server and a client model:

- the server uses as base the newest snapshot it knows the client received (acknowledgements arrive `r` snapshots late, `r = RTT / 50 ms`), and sends without a base when that snapshot is 29 or more snapshots old (`outgoingSequence - deltaMessage >= PACKET_BACKUP - 3`, `server/sv_snapshot.cpp:117`);
- a lost message is encoded, counted in the bandwidth, and not delivered; the client decodes only delivered messages and uses only its own decoded copies;
- late and duplicated packets are dropped by the netchan (`Netchan_Process` rejects `sequence <= incomingSequence`), so reordering behaves as loss;
- assumptions: independent losses (no bursts), acknowledgements never lost, constant RTT, the client acknowledges every packet. Tables and per-field modes are trained once on a mix of conditions.

## 1.11 Network-layer calculations

`frag_rate.py` combines the codec's measured sizes with formulas taken from the source:

- fragmentation: a message of at least 1300 bytes is split (`net_chan.cpp:34`, `:159`); a message of exactly `k * 1300` bytes sends an extra empty fragment (`:130`); the client drops the whole message when a fragment is missing (`:294`);
- rate limiting: `rateMsec = (min(size, 1500) + 48) * 1000 / rate` and a message is "rate delayed" when `rateMsec >= snapshotMsec` (`sv_snapshot.cpp:623-690`); effective interval is rounded up to the 50 ms server frame; defaults `rate` 4000 and `snaps` 20 (`client/cl_main.cpp:2636-2637`); LAN clients get `rate` 99999 (`sv_client.cpp`, around line 1460);
- gamestate: resent when the client acknowledges a later message without having the current `serverId` (`sv_client.cpp:1819`); a client that is not active is sent a message at most every 1000 ms (`sv_snapshot.cpp:704`); the expected extra connection delay is `(attempts - 1) * (1000 ms + RTT)` with `attempts = 1 / (1 - p)^fragments`.

These are models, not measurements on a live server.

## 1.12 Worst cases

`worst.py` builds snapshots with many updated entities and prices them with the new codec (real encoder, round trip checked) and with `engine_enc.py`, a Python model of the engine's writer (`MSG_WriteDeltaEntity`, `MSG_WriteDeltaPlayerstate`, `SV_WriteSnapshotToClient`). The model was checked on the real snapshots: it reproduces the measured bit count of 1,499 out of 1,500 snapshots; the one difference is a field whose server-side bits differ (probably `-0.0` against `+0.0`) while the decoded values are equal. Two scenarios: A clones real player-entity updates (realistic upper scale), B sets every field of every entity to a random value (absolute bound, unreachable in play).

## 1.13 What was deliberately not done

- No change to the engine, client or game code. No protocol was implemented.
- No measurement with several human players, other maps or other modes.
- No study of the client-to-server direction (`usercmd`), of entity prioritisation, interest management or bandwidth budgets.
- No evaluation of transport libraries (yojimbo, GameNetworkingSockets) beyond the notes in the hand-off.
- No context-mixing coder, no temporal context, no velocity-based prediction (05, section 5.7 lists these as candidates).
- No test on a live server with real network conditions.
