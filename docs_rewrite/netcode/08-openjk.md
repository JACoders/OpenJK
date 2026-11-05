# 8. OpenJK: what differs from the 1.01 tree, and where the code is

This repository is a fork of OpenJK (branch `feature/poc-netcode`, forked from OpenJK `260c59c2`, 29 September 2026) and is the proof-of-concept base of the rewrite. The analysis in [02](02-engine-code-findings.md) and [07](07-transport-study.md) cites the 1.01 tree (`E:\Git\jka\jediacademy\codemp`), the code of the server that produced the captures. This page says what is different in OpenJK, what was re-checked here, and where every function is in this tree. Paths are relative to `codemp/` unless stated.

**The 1.01 server stays the reference for the measurements.** Where OpenJK differs, the difference is either a default setting (a cvar that can be set back) or one rule of the pilot player state, described below.

## 8.1 Wire compatibility with 1.01: checked

| item | how it was checked | result |
|---|---|---|
| Field tables (`entityStateFields`, `playerStateFields`, `pilotPlayerStateFields`, `vehPlayerStateFields`) | extracted from both `msg.cpp` files with the decoder's extractor and compared entry by entry | identical: 132, 137, 140, 69 entries, same names, same widths. OpenJK writes two widths as macros (`GENTITYNUM_BITS`, `MAX_POWERUPS` = 16); the extractor accepts both |
| Huffman frequencies (`msg_hData`) | extracted from both files and compared | identical (256 values). The active table is `qcommon/msg.cpp:2801`; the one at `:2539` is commented out |
| Bit layer, delta entity, delta player state, arrays | read | same bitstream. OpenJK adds overflow checks (`MSG_WriteBits`, `MSG_ReadBits`, `Huff_offsetReceive` / `Huff_offsetTransmit` take a bound), `lc` range checks in the readers, and replaces `MSG_WriteShort` of the array masks by `MSG_WriteBits(..., MAX_STATS)` etc., which writes the same 16 bits. In OOB mode it fixes the 32-bit write (`msg->bit += 32` instead of `8`); OOB data does not use the delta path |
| Whole decode of the study's captures with these tables | `jkadec.py` run with `SRC` pointing at `codemp/qcommon/msg.cpp` | 7,101 / 7,101 and 1,605 / 1,605 messages decoded, as with the 1.01 tables |
| Gamestate tail | read | the same as 1.01: after the checksum feed, a short (0; the old random-mission terrain filler, `server/sv_client.cpp:468`) |
| Client demos (`.dm_26`) | read | same layout as 1.01 (`CL_WriteDemoMessage`, `client/cl_main.cpp:206`); the message is the server message after the netchan header, with `svc_EOF`; recording starts at the first snapshot without a base (`client/cl_parse.cpp:260`) |

The only wire difference found:

- **Pilot player state.** In 1.01 a pilot riding inside a vehicle is written with only `ARRAY_LEN(pilotPlayerStateFields) - 82` = **58** fields. OpenJK writes all 140 (writer `qcommon/msg.cpp:2133`, reader `:2382`), since OpenJK commit `3409151cb` (2015, "Stop clobbering number of fields in playerstate for pilots", reported as a cause of the "seeker bug", OpenJK issue 708). An OpenJK client reads a 1.01 server correctly (it accepts `lc` up to 140). A 1.01 client connected to an OpenJK server fails with `invalid playerState field count` as soon as a field after the 58th changes. The captures contain no pilot state, so no measurement depends on it. `codec2.py` and `fuzz.py` follow the 1.01 rule.

**Decision (4 November 2026):** do not restore the `- 82` rule. The new codec sends changed fields only, so the rule saves nothing, and OpenJK removed it because it caused bugs.

## 8.2 Defaults that differ (settings, not format)

| setting | 1.01 | OpenJK upstream | this branch | where |
|---|---|---|---|---|
| server `sv_fps` | 20 | 40 | **20** | `server/sv_init.cpp:983` |
| `sv_ratePolicy` / `sv_clientRate` | (no policy: client `rate` clamped 1000..90000) | 1 / 50000: every non-LAN client gets `sv_clientRate`; policy 2 clamps the client value to `sv_minRate..sv_maxRate` | 1 / **25000** | `server/sv_init.cpp:958-960`, `server/sv_client.cpp:1076-1092` |
| `sv_snapsPolicy` | (no policy: `snaps` clamped 1..30) | 1: every client gets `sv_fps` snapshots per second | unchanged | `server/sv_init.cpp:979-981`, `server/sv_client.cpp:1105` |
| client `rate` / `snaps` | 4000 / 20 | 25000 / 40 | unchanged | `client/cl_main.cpp:2840-2841` |
| `cl_maxpackets` | 30 | 63 (clamped 20..1000) | unchanged | `client/cl_main.cpp:2770`, `client/cl_input.cpp:1459-1463` |
| `cl_packetdup` | 1 | 1 (clamped 0..5) | unchanged | `client/cl_main.cpp:2771` |
| LAN rate | 99999 | 100000 when `sv_lanForceRate` 1 and not `com_dedicated 2` | unchanged | `server/sv_client.cpp:1074-1075` |
| `PACKET_BACKUP` | 32 | 32 | unchanged | `qcommon/qcommon.h:117` |
| `SV_RateMsec` | `(min(size,1500)+48)*1000/rate` | the same, with `sv_minRate` / `sv_maxRate` and `timescale` applied | unchanged | `server/sv_snapshot.cpp:679` |
| `g_smoothClients` / `cg_smoothClients` | 1 / 1 | 1 / 1 | unchanged | `game/g_xcvar.h:157`, `cgame/cg_xcvar.h:129` |

Branch decision (4 November 2026): keep OpenJK's code, policies and the pilot fix; set `sv_fps` to 20 so that the snapshot rate matches the captures (with `sv_snapsPolicy` 1 every client then receives 20 snapshots per second); set `sv_clientRate` to 25000 (the client default). At 25000 bytes per second and 50 ms, a message is rate-delayed from 1,202 bytes (`(size + 48) * 1000 / 25000 >= 50`); on the test part of the 5-minute capture this delays 0.0 % of snapshots for both the engine and the new codec (03, section 3.10, row 25,000). OpenJK's upstream pair (50000 bytes per second, 40 snapshots per second) has the same threshold.

Consequences for the results of [03](03-results.md):

1. **Rate table (3.10).** The table describes a 1.01 server with client rates from 4000 up. With this branch's or OpenJK's defaults, only messages from 1,202 bytes are delayed.
2. **Base age.** The loss and latency results (3.9) convert RTT to base age with 50 ms per snapshot, which is this branch's rate. At OpenJK's upstream 40 snapshots per second the same RTT gives twice the age; sizes at 40 Hz were not measured.
3. **To stay on the 1.01 conditions**, set `sv_ratePolicy 2`, `sv_maxRate 90000` (see 8.4 for why `sv_maxRate` must not stay 0), and on the client `rate 4000`, `snaps 20`. No code change is needed.

## 8.3 Other differences relevant to the netcode

| item | OpenJK | where |
|---|---|---|
| Challenge | `HMAC-MD5(secret, address + timestamp)`, stateless, valid about 16 s; no longer `rand()` | `server/sv_challenge.cpp:108-185`, called by `SV_GetChallenge` at `server/sv_client.cpp:62` |
| OOB rate limiting | per-address leaky buckets (`SVC_RateLimitAddress`) on the connectionless packets | `server/sv_main.cpp:369-408` |
| `connect` string | still `Huff_Decompress(msg, 12)` | `server/sv_main.cpp:743` |
| Downloads | UDP only, as in 1.01 | |
| Server-side demos | `sv_autoDemo`: the server records the messages it sends; same format as client demos | `server/sv_ccmds.cpp:1562`, `:1785` |
| IPv6 | none: `qcommon/net_ip.cpp` opens `AF_INET` sockets only | `qcommon/net_ip.cpp:175` |
| Clients | `MAX_CLIENTS` 32 | `qcommon/q_shared.h:890` |
| Player trajectories | the base game module: with `g_smoothClients` 1 players are sent as `TR_LINEAR_STOP` with `trTime` = `commandTime` through `BG_PlayerStateToEntityStateExtraPolate(..., qfalse)`, never rounded ("Don't snap the origin of players! It screws prediction all up"), as in the 1.01 module and the captures | `game/g_active.c:3326`, `:3768` |
| Licence | 447 files under `codemp/` and `shared/` carry "GNU General Public License version 2 as"; 27 carry "version 2 or any later version" (or "either version 2 of the License, or"). The GPL-v2-only conclusion of 02 section 2.9 holds for this tree | grep, not a legal reading |

## 8.4 A bug found on the way

With `sv_ratePolicy 2` and the default `sv_maxRate 0`, `SV_UserinfoChanged` clamps the client rate with `Com_Clampi(sv_minRate, sv_maxRate, i)` = `Com_Clampi(0, 0, i)`, so the rate becomes 0 (`server/sv_client.cpp:1090`). `SV_RateMsec` then divides by `rate * timescale` = 0 (`server/sv_snapshot.cpp:679`). Not tested; read only. Set `sv_maxRate` when using policy 2, or fix the clamp.

## 8.5 Where the functions of 02 and 07 are in this tree

Generated with a script that searches each definition (lines of 5 November 2026). Functions only; sub-ranges cited in 02 are inside these functions.

| function or item (02 / 07 section) | OpenJK location |
|---|---|
| `MSG_WriteBits` / `MSG_ReadBits` (2.1) | `qcommon/msg.cpp:137` / `:231` |
| `Huff_offsetReceive` / `Huff_offsetTransmit` / `Huff_Decompress` / `Huff_Compress` / `Huff_addRef` | `qcommon/huffman.cpp:272` / `:327` / `:333` / `:387` / `:186` |
| `msg_hData` (active table) | `qcommon/msg.cpp:2801` |
| `entityStateFields` (2.2), user fields, `FLOAT_INT_BITS` | `qcommon/msg.cpp:832`, `:1010`, `:1026` |
| `MSG_WriteDeltaEntity` / `MSG_ReadDeltaEntity` | `qcommon/msg.cpp:1040` / `:1161` |
| `playerStateFields` / `pilotPlayerStateFields` / `vehPlayerStateFields` (2.3); unoptimised table | `qcommon/msg.cpp:1303` / `:1461` / `:1625`; `:1716` |
| `MSG_WriteDeltaPlayerstate` / `MSG_ReadDeltaPlayerstate` (pilot table chosen at `:2133` / `:2382`) | `qcommon/msg.cpp:2096` / `:2334` |
| `MSG_CheckNETFPSFOverrides` | `qcommon/msg.cpp:1890` |
| `_ONEBIT_COMBO` (commented) / `_OPTIMIZED_VEHICLE_NETWORKING` | `qcommon/q_shared.h:1052` / `:1056` |
| `SV_EmitPacketEntities` / `SV_WriteSnapshotToClient` (2.4), `serverTime` written | `server/sv_snapshot.cpp:56` / `:123`, `:200` |
| `SV_UpdateServerCommandsToClient` | `server/sv_snapshot.cpp:282` |
| `SV_RateMsec` (division by `rate`) | `server/sv_snapshot.cpp:679` |
| `SV_SendMessageToClient` / `SV_SendClientSnapshot` (overflow warning `:852`) / `SV_SendClientMessages` | `server/sv_snapshot.cpp:692` / `:772` / `:865` |
| `svc_EOF` appended in `SV_Netchan_Transmit` | `server/sv_net_chan.cpp:155` |
| Server frame loop: `frameMsec`, `sv.time += frameMsec` | `server/sv_main.cpp:1169`, `:1228` |
| Rate and snaps policies | `server/sv_client.cpp:1090`, `:1105` |
| `SV_CreateClientGameStateMessage` / `SV_SendClientGameState` (2.6) | `server/sv_client.cpp:423` / `:483` |
| `SV_WriteDownloadToClient` / `SV_ExecuteClientMessage` / `SV_DirectConnect` | `server/sv_client.cpp:714` / `:1459` / `:131` |
| `CL_ParseServerMessage` / `CL_ParseSnapshot` / `CL_ParsePacketEntities` (2.5) | `client/cl_parse.cpp:807` / `:217` / `:105` |
| `CL_ParseGamestate` / `CL_ParseDownload` | `client/cl_parse.cpp:501` / `:634` |
| `CL_WriteDemoMessage` / `CL_Record_f` (gamestate rebuilt `:341-396`) / demo write in `CL_PacketEvent` | `client/cl_main.cpp:206` / `:273` / `:2103` |
| `CL_ReadyToSendPacket` / `CL_WritePacket` (7.1) | `client/cl_input.cpp:1424` / `:1496` |
| `Netchan_TransmitNextFragment` / `Netchan_Transmit` / `Netchan_Process` (2.7) | `qcommon/net_chan.cpp:102` / `:159` / `:222` |
| `PACKET_BACKUP`, `MAX_RELIABLE_COMMANDS`, `MAX_MSGLEN` | `qcommon/qcommon.h:117`, `:127`, `:159` |
| `CG_PredictPlayerState` (2.8) / `CG_CalcEntityLerpPositions` | `cgame/cg_predict.c:927` / `cgame/cg_ents.c:3082` |
| `g_smoothClients` use / `BG_PlayerStateToEntityState` / `...ExtraPolate` | `game/g_active.c:3326`, `:3768` / `game/bg_misc.c:2762` / `:2912` |
