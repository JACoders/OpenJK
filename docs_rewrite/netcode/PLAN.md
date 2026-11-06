# Netcode rewrite: implementation plan

This plan turns [04](04-recommendations.md) into steps to do one after the other. Each step has a deliverable, a check that says when it is done, and the steps it depends on. Progress, results of finished steps and the next action are in [progress/README.md](progress/README.md). The plan follows the owner's goals listed in the study's [README](README.md): no loss of precision, no compatibility constraint, as few network settings as possible, netcode that other JKA forks can reuse, and Windows and Linux (x86 and x64) as targets.

The work happens in this repository (a fork of OpenJK, branch `feature/poc-netcode`). Paths are relative to `codemp/`; where a function is cited, its line in this tree is in [08-openjk.md](08-openjk.md) section 8.6. Effort labels: S = days, M = weeks, L = months, for one engineer who knows the code.

## 1. Principles

1. **Values are never less precise than today.** For any message, the values the client decodes are equal to what the original code delivers (truncation to the field widths, `-0.0` as `+0.0`), or more precise. Every test compares field by field.
2. **One independent coded stream per message.** No coder state survives from one packet to the next; only acknowledged snapshots are used as bases.
3. **The codec is a library.** It knows the field descriptors, not the game: no dependency on the server, the client, `game` or `cgame` beyond `entityState_t`, `playerState_t` and their field tables. Another fork links it, supplies its own field tables, trains or loads its own probability tables, and works with the uniform fallback until it does.
4. **The codec is isolated from the transport.** The transport can change later without touching it.
5. **Data-driven.** Field lists, widths, modes and tables are data generated from one source of truth, versioned together.
6. **No legacy path is kept for compatibility.** The protocol number changes and old clients and servers are refused at connection. A switch between the old and the new coder is kept only while it helps comparison and is removed afterwards.
7. **Settings are measured, not configured.** A new network cvar is added only for debugging or statistics (04 section 4.10).
8. **Game and cgame may change.** The codec keeps giving them the same structures, so changes there are optional and separate (for example a later audit of prediction and interpolation).

## 2. Steps

### Step 1: confirm the prediction modes with the real codec (Python, S)

Depends on: nothing.

**Done on 5 November 2026** (03 section 3.13): 107.3 real bytes per snapshot with integer arithmetic, 0 failures on real, synthetic and simulated-loss messages. The integer arithmetic is kept.

- Add to `codec.py` and `codec2.py` the modes measured in 03 section 3.12, offered to the per-field mode chooser rather than forced:
  - T: time fields (`pos.trTime`, `apos.trTime`, `commandTime`) against the message's `serverTime`;
  - P2: positions against `old + mean velocity * elapsed`, residual on the 32-bit patterns; two versions of the prediction arithmetic: float (computed in double from float32, rounded once) and integer only;
  - A2: angle index against `2 * old - older`, where `older` is the base's own base (the client must find it by the base's delta number, not by `seq - 2 * age`).
- Add the rANS initial-state trick (05 section 5.9) and measure the end-of-stream cost.
- Check: 0 round-trip failures on the 8,842 real messages, on `fuzz.py` seeds 1 to 5, and in `sim_loss.py`; mean real bytes on 5min-test at most 110 (study: 139.6 without prediction, 106.1 ideal with it); the integer version of P2 within 2 bytes of the float version, or the float version kept with its determinism test of step 6.

### Step 2: one source of truth for the field descriptors (S)

Depends on: nothing (can run alongside step 1).

**Done on 5 November 2026** ([progress/step-02-field-descriptors.md](progress/step-02-field-descriptors.md)): `data/fields.json`, offsets and C types computed by the compiler (`tools/fieldinfo`); schema version 2 adds the C type of each field (bool, int, float, enum).

- Generate, from the arrays of `qcommon/msg.cpp` (`entityStateFields`, `playerStateFields`, `pilotPlayerStateFields`, `vehPlayerStateFields`, the four player-state arrays), a descriptor list: name, offset, width, float flag, array membership, plus a schema version and hash.
- Decide the fate of the runtime width overrides (`netf_overrides.txt`, `psf_overrides.txt`, 02 section 2.3). Decided: removed from the new protocol; `user*` fields are coded by C type.
- Check: the descriptors reproduce the tables extracted by `jkadec.py` (132, 137, 140, 69 entries); the Python tools read the same generated file.

### Step 3: state recorder in the server (S)

Depends on: step 2. **No longer needed before step 5**: client demos give the data (step 4). The recorder is still wanted later, to record states on this branch's server without a Huffman decoder and to feed the training of step 12.

- A server option that writes, per client and per message, the decoded content the codec needs: header, player state(s), entity list, baselines, server commands, gamestate, downloads, the chosen base and the send time. Simple binary format with the schema hash.
- It replaces the proxy capture of the study (which recorded bitstreams and needed a Huffman decoder). The proxy stays usable on the original 1.01 server.
- Check: replaying a recording through the legacy writer gives back the same bytes as the live server sent (or the same values, if byte identity is not possible because of server-side `-0.0`, 05 section 5.4).

### Step 4: demo corpus (S; the owner supplies the demos)

Depends on: nothing (decided 5 November 2026: client demos replace new recordings).

**Done on 5 November 2026** ([progress/step-04-demo-corpus.md](progress/step-04-demo-corpus.md)): 1,116 demos (148 hours, CTF, team FFA, siege, duel; 1.01 protocol; no JA+ or JAPRO) indexed; 60 training and 20 held-out demos (held out by map), first 5,400 messages each; round trip 0 failures after removing the 1.01 pilot rule from `codec2.py`. All demos stay in `data/demos/train/` as supplied; the sets are a column of `data/demos/selection.tsv`, not folders.

Why demos work: the client writes into a demo exactly the server message it received, after removing the netchan header (fragments reassembled) and the XOR, and before nothing else is changed (`CL_PacketEvent` and `CL_WriteDemoMessage`, `codemp/client/cl_main.cpp:2052-2104`, `:206`). The proxy captures of the study hold the same message taken on the server just before `SV_Netchan_Transmit`. Differences, all handled by the reader:

- a demo message ends with the `svc_EOF` the server appends after the proxy's capture point (remove it before measuring sizes);
- the first message of a demo is a gamestate rebuilt by the client (`CL_Record`) from its configstrings and baselines, not the one the server sent: use it for decoding, never for gamestate sizes;
- messages lost on the way are absent (the demo is the client's view, with the real base ages, losses and rate-delay flags of that player);
- recording starts at the first snapshot without a base (`demowaiting`), so a demo decodes from its start.

Work:

- A `.dm_26` reader in `tools/python/jkadec.py` (per message: `int32` sequence, `int32` length, the bytes; end marker `-1 -1`), and its use in `bdata.py` and `fullload.py` in place of the capture files.
- Validation as for the captures: every message must end within 1 to 8 bits of its length after `svc_EOF`; a demo written by another client that differs is rejected and listed.
- The owner supplies demos in `data/demos/train/` and `data/demos/holdout/` (the held-out set is chosen before any measurement and never used for training). `data/demos/` is not committed (`.gitignore`); a list file records each demo: map, mode, mod, number of players, `sv_fps` if known, client or server demo, protocol.
- Wanted: protocol 1.01 (`.dm_26`; 1.00 `.dm_25` was not checked); several humans per game; many maps; FFA, CTF, team FFA, duel, siege, race; base, JA+, JAPRO and other mods; both game-module behaviours (players as `TR_LINEAR_STOP` and as `TR_INTERPOLATE`, 03 section 3.12); server-side demos (`sv_autoDemo`) as well, which use the same format.
- Check: every demo of the list decodes with the validation above; the list file exists; train and held-out sets are separate.

### Step 5: re-measure on the new data, finish the model (S)

Depends on: steps 1 and 4. **Done** 5 November 2026: [progress/step-05-demo-measurements.md](progress/step-05-demo-measurements.md). One global table set, no age context; held-out ratio to the engine 0.445 on snapshots; `bench2.py` / `bench5.py` and the `TR_INTERPOLATE` check dropped.

- Re-run `bench2.py`, `bench5.py`, `run_pred.py` and the loss simulation on the demos: sizes per map, mode, mod and `sv_fps`; training on the train set, test on the held-out set. Compare with the study's figures (one map, bots).
- The demos carry real base ages: report sizes by base age as well, and compare with `sim_loss.py`.
- Check the elapsed-time rule for `TR_INTERPOLATE` players (P2 with the two snapshots' `serverTime`) on demos of servers whose game module sends players that way.
- Decide: one global table set or per-mode sets; age as a context or not (04 section 4.5).
- Check: documented mean / p95 / max on the held-out set per map and mode; if held-out sizes exceed training sizes by more than 15 %, widen the training data first.

### Step 6: the codec library in C++ (M)

Depends on: steps 2 and 5. **In progress** (6 November 2026): rANS and whole-message codec in `codemp/netcodec/`, identical to the Python reference, restructured, faster than OpenJK; next the format in C++ only ([progress/step-06-cpp-codec.md](progress/step-06-cpp-codec.md), "Part 2").

- New directory `codemp/netcodec/` (decided 5 November 2026; not `shared/`, which holds code common to the single-player, JK2 and multiplayer trees), C++20 inside the library (`std::span` for buffers, C++ Core Guidelines I.13) with a plain C++ interface (a few functions and plain structures, no STL or exceptions in the public header; no C interface, since the engine and the forks that would reuse it are C++), no dependency on server or client; CMake builds it with MSVC, GCC and Clang; the trained tables are compiled in:

| unit | content | reference |
|---|---|---|
| `rans` | 32-bit byte-renormalising rANS, 12 to 15-bit probabilities, initial-state trick | ryg_rans (CC0); `codec.py` |
| `schema` | generated descriptors of step 2, pilot and vehicle subsets, arrays | step 2 output |
| `tables` | loader of trained tables and per-field modes, version and schema hash, uniform fallback | `codec.py` `Tables` |
| `fieldcodec` | absolute / delta classes, float kinds (zero, integral, angle index, other), prediction modes T, P2, A2; booleans by C type (changed = inverse, no value); `user*` fields by C type, not by their 1-bit legacy width (04 section 4.2) | `put_int`, `put_float`, `get_int`, `get_float` in `codec.py` |
| `msgcodec` | whole message: header, operations, snapshot, gamestate, commands, setgame, download, mapchange | `codec2.py` |
| `escape` | raw coding of an entity or a message and the rule that picks it (worst case, 04 section 4.8) | not in the study |
| `tests` | golden traces, synthetic fuzz, garbage-input fuzz, determinism | section 4 |

- Interfaces (shape only): `bytes = Encode(base*, header, ops, baselines, tables)`; `ops = Decode(bytes, lookupBase(delta) -> base*, baselines, tables)`, where `lookupBase` is called after the delta has been decoded (as `msg_get` in `codec2.py`). All bounds are checked while decoding (entity numbers below `MAX_GENTITIES`, field indices below the table length, string and download lengths, end of input); a failure drops the message.
- Check: with the same tables, the C++ encoder produces byte-identical output to the Python codec on all recordings; 0 mismatches on at least 100,000 synthetic messages; random and mutated bytes never crash under ASAN / UBSAN; angle decode and P2 prediction bit-identical on MSVC, GCC and Clang, x86 and x64, optimised and not, for all 65,536 indices and for the recorded positions; encode about 2 microseconds per snapshot on an optimised build (study: 1 to 2 µs for 300 symbols).

### Step 7: integration in the engine (M)

Depends on: step 6.

- Server: build every message with the codec in `SV_SendClientSnapshot`, `SV_SendClientGameState`, `SV_WriteDownloadToClient` and `SV_SendClientMapChange`; keep the base selection of `SV_WriteSnapshotToClient` for now (step 9 changes it).
- Client: decode in `CL_ParseServerMessage` into `clSnapshot_t`, `cl.parseEntities` and `cl.entityBaselines`, with the validity rules of `CL_ParseSnapshot` (base valid, matching message number, entities not overwritten) and the `%` sanitation of strings.
- Cover: bots (built but not sent), spectators, vehicles and pilots, downloads (UDP; HTTP downloads are separate), `svc_setgame`, `svc_mapchange`, and both demo systems: client demos and the server-side demos (`sv_autoDemo`, present in OpenJK) record the message stream, so the demo format changes with the protocol (old demos need the old reader or a converter; decide).
- New protocol number; refuse a mismatch at the `connect` handshake. Out-of-band messages keep their own format.
- Server commands are coded against earlier commands (step 6, part 2 B): server and client give the codec their command history (`CommandHistory`), and both keep the last 256 commands (the engine keeps 128 today, `MAX_RELIABLE_COMMANDS`).
- Statistics cvars or commands: bytes per snapshot, symbols, escapes, base age.
- Check: a normal game plays with no visible difference; `cg_showmiss` shows no new prediction misses; the statistics match step 5 within 5 %; client and server demos record and play back.

### Step 8: safety and limits (S)

Depends on: step 7.

- Escape path per entity, chosen when its coded size would exceed the raw size.
- Size cap: on overflow, retry with fewer entities or a full-state message; never drop a snapshot silently (today the engine clears the message and prints a warning, `server/sv_snapshot.cpp:852`).
- Fuzz corpus and sanitizer builds in CI.
- Check: the synthetic worst case (every field random, 1,000 entities) stays within the raw layout plus a small constant and decodes.

### Step 9: acknowledgements, base selection, reliable data (M)

Depends on: step 7. The options and the decision procedure are in [07](07-transport-study.md).

- Acknowledgement record in every client packet (bitfield of the last 32 or more messages); the server uses the newest acknowledged snapshot as base.
- Gamestate, server commands and client commands on a reliable path with its own fast resend, instead of the 1,000 ms gamestate resend wait; snapshots never wait behind reliable data. Measured on the held-out demos (step 6): every server command is sent 2.59 times on average (repeated in every message until acknowledged), about 60 % of the command bytes.
- Client input: every `usercmd` not yet acknowledged is resent in each packet, up to a cap; removes `cl_packetdup`.
- Snapshot history sized from the supported RTT; replaces the fixed `PACKET_BACKUP`.
- Check: simulated loss, latency and jitter (as `sim_loss.py`, extended to burst loss and lost acknowledgements) and two-machine tests; 0 decode failures; base age equal to RTT / frame time plus at most one frame.

### Step 10: settings computed at run time (M)

Depends on: step 9.

- Bandwidth estimate per connection from the acknowledgement record; it sets the snapshot and packet rates. Removes `rate`, `sv_maxRate`, `sv_minRate`, `sv_ratePolicy`, `sv_clientRate`, `sv_lanForceRate`, `snaps`, `sv_snapsMin`, `sv_snapsMax`, `sv_snapsPolicy`, `cl_maxpackets`, `cl_lanForcePackets`.
- Interpolation delay computed from the measured snapshot interval and jitter. Removes `cl_timeNudge`, `cl_timenudgeDuration` and the user choice of `cg_smoothClients`.
- Evaluate a real send timestamp (or the server's frame-to-send delay) in each snapshot, for the client's clock synchronisation; `serverTime` stays the game clock (04 section 4.9, "Time base").
- Timeouts derived from the keep-alive interval.
- Debug cvars to force each value and to print the measured ones.
- Check: on links from LAN to 400 ms RTT and from 2 % to 20 % loss, no setting is needed to reach the snapshot rate the link allows; the forced-value debug cvars reproduce the old behaviour for comparison.

### Step 11: transport security and library decision (M to L, optional)

Depends on: steps 9 and 10, and the owner's answers in 07 section 7.7.

- Decide on authenticated encryption and the connection model; evaluate an in-place AEAD layer, netcode + reliable, GameNetworkingSockets against the requirements of 07 section 7.3, on a prototype with the real codec payloads.
- Check: the decision is recorded with measured overhead, CPU at 64 clients, behaviour under loss, and licence review.

### Step 12: live measurement and tuning (M)

Depends on: steps 7 to 10.

- Playtest servers with the recorder: bytes per client per second, snapshots per second, prediction misses, CPU per client, escape rate.
- Retrain the tables from the new recordings; size regression check.

### Step 13: beyond the codec (L, optional)

Not studied; the largest unexplored potential (05 section 5.7):

- per-client priority and bandwidth budget, distance-based update rates, interest culling;
- the client-to-server `usercmd` stream;
- an audit of the client's prediction and interpolation code (`cg_predict.c`, `cg_ents.c`, time synchronisation in `CL_AdjustTimeDelta`), now that compatibility does not constrain it;
- a fixed physics step (`pmove_*`, 04 section 4.10), a gameplay decision.

## 3. Order at a glance

| step | what | depends on | effort |
|---|---|---|---|
| 1 | prediction modes and rANS trick in the Python codec | none | S |
| 2 | field descriptors from one source | none | S |
| 3 | state recorder (later; not needed before step 5) | 2 | S |
| 4 | demo corpus: `.dm_26` reader, demos from the owner | none | S |
| 5 | re-measure, finish the model | 1, 4 | S |
| 6 | C++ codec library | 2, 5 | M |
| 7 | engine integration, demos | 6 | M |
| 8 | safety and limits | 7 | S |
| 9 | acknowledgements, reliable data | 7 | M |
| 10 | settings computed at run time | 9 | M |
| 11 | transport security decision | 9, 10 | M to L |
| 12 | live measurement | 7 to 10 | M |
| 13 | beyond the codec | | L |

## 4. Test matrix

| test | method | when |
|---|---|---|
| Golden round trip | decode recordings, re-encode, decode again, compare field by field; compare with the values of the original decoder | every build |
| Python and C++ identity | same tables, same input, same bytes | every codec change |
| Synthetic fuzz | `fuzz.py` generator ported to C++ (any base age, vehicle and pilot, download, extreme values) | every build |
| Garbage fuzz | random and mutated byte streams into the decoder under ASAN / UBSAN | nightly |
| Determinism | MSVC, GCC, Clang, x86 and x64, optimised and not: all angle indices, P2 predictions on recorded positions | every build |
| Platforms | build and run the tests above on Linux (GCC and Clang); a Windows client against a Linux server and the reverse | every build from step 6 (decided 5 November 2026: Linux is built and tested alongside Windows) |
| Size regression | evaluator on held-out recordings; fail if the mean grows by more than 3 % | table or schema change |
| Loss, latency, jitter | simulator over recordings: loss 0 to 20 % with bursts, RTT 0 to 400 ms, jitter, lost acknowledgements | transport change |
| Worst case | adversarial snapshots stay within the cap and decode | every build |
| CPU | C benchmark over recordings on the target machine | codec change |
| Live | two or more machines, several humans, loss emulation | step 12 |

## 5. Risks

| risk | consequence | mitigation |
|---|---|---|
| Training data too narrow | real packets larger than measured | step 4 demo corpus, held-out set, escape path |
| Table drift when content or mods change | larger packets, never wrong ones | versioned tables, uniform fallback, retrain in the release procedure |
| P2 prediction computed differently on client and server | wrong positions on the client | integer version, or fixed float arithmetic with the determinism test |
| A2 base's base not available on the client | decode failure | the server uses the base's real delta source; fall back to plain delta when the client cannot have it |
| Server-side `-0.0` and similar hidden bit patterns | the study treats `-0.0` and `+0.0` as equal because the engine does | keep the equality rule explicit in tests |
| Message growth under unusual states | overflow, fragmentation | escape path, cap, fallback |
| Acknowledgement redesign breaks base selection | corrupted deltas | round-trip tests under simulated loss |
| Bandwidth estimation oscillates or is too slow | stutter, delayed snapshots | simulator with changing links before live tests; debug cvars to force values |
| Demo format change | old demos unreadable | keep a legacy reader or write a converter |
| Scope creep into priority, culling, prediction audit | schedule | keep step 13 separate |

## 6. Targets taken from the study

Targets for the study's content (one map, bots, 20 snapshots per second). Re-measured in step 5 on the demos: these targets hold for the study's content only; on the held-out demos the codec gives 0.445 of the engine size for snapshots (69.0 bytes against 155.1) and 0.483 for all messages ([progress/step-05-demo-measurements.md](progress/step-05-demo-measurements.md)):

- snapshot payload mean at most 110 bytes against 337.6 today, p95 about 190, maximum below 450 for ordinary snapshots;
- gamestate about 4 KB instead of 7 KB on the study map;
- encoding and decoding each below 0.3 % of a core for 64 clients at 20 snapshots per second;
- zero decode mismatches in every test of section 4.
