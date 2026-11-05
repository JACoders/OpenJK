# 2. What was read in the code

All references are to the original 1.01 tree (`E:\Git\jka\jediacademy\codemp`, the code of the server that produced the captures) unless stated otherwise. The same functions in this repository (OpenJK), and the differences found there, are listed in [08-openjk.md](08-openjk.md). Line numbers are those of the files as they were on 3 November 2026. CleanJK (`E:\Git\jka\CleanJK\Game\code`) has the same structure; its files were not re-read line by line, so check the numbers there before relying on them.

Everything below was read directly. Items that are inferences are marked "inferred".

## 2.1 The bit layer and the Huffman coder

- `qcommon/msg.cpp:129` `MSG_WriteBits` / `:211` `MSG_ReadBits`. A value of width `w` is written as the low `w & 7` bits raw (least significant bit first), then one Huffman symbol per remaining byte, least significant byte first. A negative width means a signed value. In OOB mode (connect strings) widths 8, 16, 32 are written as plain bytes.
- `qcommon/huffman.cpp`: an adaptive Huffman tree (Sayood's algorithm). In the data path the tree is never updated after start-up: `Huff_offsetTransmit` (`:298`) and `Huff_offsetReceive` (`:252`) only walk it. `MSG_initHuffman` (`msg.cpp:3219`) builds it by calling `Huff_addRef` `msg_hData[i]` times for every byte value `i`. The table in use is the one at `msg.cpp:2958`; an older table at `:2696` is inside a comment.
- `Huff_Compress` / `Huff_Decompress` (`huffman.cpp:358`, `:304`) compress a whole buffer adaptively; they are used only on OOB data such as the `connect` string (`NET_OutOfBandData`, `server/sv_main.cpp`). The calls in `server/sv_net_chan.cpp` and `client/cl_net_chan.cpp` are commented out.
- Tooling left by the original authors: with `_NEWHUFFTABLE_` the write path appends every Huffman-coded byte to `c:\netchan.bin`; with `_USINGNEWHUFFTABLE_` the start-up code reads that file and prints a new `msg_hData` (`msg.cpp:16-17`, `:200`, `:255`, `:3238-3270`). Both defines are commented out. So the table was trained once, by hand, on byte frequencies of real traffic.
- Memory: `huff_t` has 768 `node_t` plus 768 node pointers plus 257 location pointers (`qcommon/qcommon.h`, around line 1044-1075); the engine holds two (`huffman_t` = compressor + decompressor). About 57 KB with 32-bit pointers, about 102 KB with 64-bit.

## 2.2 Entity state layout

- `entityStateFields[]` at `msg.cpp:859`: 132 fields, each with a width in bits (`0` = float). `assert( numFields + 1 == sizeof( *from )/4 )` (`msg.cpp:1085`) shows that every field is a 32-bit integer or float and that `number` is the 133rd word.
- Widths include 32, 16, 10 (`GENTITYNUM_BITS`), 8, 6, 4, 2, 1, `-16`, `0` (float). The last 12 entries are mod fields `userInt1..3`, `userFloat1..3`, `userVec1[3]`, `userVec2[3]`, each declared with width 1 (`msg.cpp:1038-1049`).
- Floats (`FLOAT_INT_BITS` = 13, bias 4096, `msg.cpp:1055-1056`): in an entity, after the "changed" bit: a bit for "zero", else a bit for "integral and in range" followed by 13 bits, else 32 raw bits. In a player state there is no "zero" bit.
- Integer fields in an entity: after the "changed" bit, a bit for "is zero", else the value in `width` bits. In a player state the value follows directly.
- `MSG_WriteDeltaEntity` (`:1069`): remove = 10-bit number + bit 1; unchanged but forced = number + 0 + 0; changed = number + 0 + 1 + `lc` byte (last changed field index + 1) + one flag per field up to `lc` + the values. `MSG_ReadDeltaEntity` (`:1228`) is the exact mirror.
- Entities with no change are skipped entirely by the delta writer unless `force` is set (new entity from a baseline, baseline records in the gamestate).
- Booleans (added 5 November 2026; OpenJK lines, 08 section 8.5). `qboolean` is an enum (`shared/qcommon/q_platform.h:360`), 4 bytes in memory. The boolean fields are declared 1 bit wide in the tables, but they go through the integer path: in an entity a changed boolean costs 3 bits when it becomes true (changed, not zero, value) and 2 bits when it becomes false (changed, zero); in a player state 2 bits (changed, value). The value bit is redundant: a changed boolean can only be the inverse of the old one.
- The mod fields `userInt1..3`, `userFloat1..3`, `userVec1[3]`, `userVec2[3]` are declared 1 bit wide in all four tables (`msg.cpp:1010-1021` for entities), floats included. The "changed" test compares the whole 32-bit word, but `MSG_WriteBits` masks the value to the width (`value &= (0xffffffff>>(32-bits))`), so only the lowest bit of the integer or of the float's bit pattern reaches the client. A mod that stores anything else in them must widen them with the overrides of section 2.3. OpenJK's game module does not use them.
- Other fields whose width does not match their C type (from the generated descriptors, step 2): `rocketLockTime` and `rocketTargetTime` are floats sent through the integer path with 32 bits (lossless raw pattern); `fd.forceGripCripple` is an `int` used as a 0/1 flag and sent in 1 bit.

## 2.3 Player state layout

- Three tables in `msg.cpp` under `_OPTIMIZED_VEHICLE_NETWORKING`: normal `playerStateFields` (`:1410`, 137 entries after removing commented lines), `pilotPlayerStateFields` (`:1570`) and `vehPlayerStateFields` (`:1736`). A fourth table at `:1829` is for the unoptimised build and is not used (`game/q_shared.h:2154` defines the optimised build; `_ONEBIT_COMBO` at `:2150` is commented out).
- Table lines starting with `//` exist (for example `hyperSpaceAngles` entries at `msg.cpp:1526`, `:1550`, `:1715`). They are not fields.
- Writer (`:2211`): for a non-vehicle player state one bit says whether it is a pilot state (`to->m_iVehicleNum && (to->eFlags & EF_NODRAW)`, `:2249`; `EF_NODRAW` = `1<<8`, `game/bg_public.h:581`). A pilot state uses the pilot table with only `size - 82` = 58 of its 140 fields (`:2253`; OpenJK sends all 140, see 08); the other fields are copied from the previous state and never sent. A vehicle state (`isVehiclePS`) uses the vehicle table with no pilot bit.
- Then a `lc` byte, one flag per field up to `lc`, the values, and the four arrays `stats`, `persistant`, `ammo`, `powerups` (`:2581-2658` in the reader): one "any array" bit; per array a bit and a 16-bit mask, and one value per set bit (shorts, except `stats[STAT_WEAPONS]` which is `MAX_WEAPONS` = 19 bits, `game/q_shared.h:2137`, `game/bg_public.h:527`; `powerups` are longs).
- `SV_WriteSnapshotToClient` writes a second player state (`vps`) when `frame->ps.m_iVehicleNum` is non-zero (`server/sv_snapshot.cpp:170-203`). When the old frame had no vehicle the vehicle state is written from zero.
- Runtime overrides: `MSG_CheckNETFPSFOverrides` (`msg.cpp:2006`) reads `ext_data/MP/netf_overrides.txt` and `psf_overrides.txt` and rewrites field widths. It runs when the client receives `svc_setgame` (`client/cl_parse.cpp:341-373`). A mod can widen the user fields this way. According to the owner (5 November 2026), no mod ships `netf_overrides.txt` or `psf_overrides.txt`. A new protocol must define what happens to this mechanism.

## 2.4 The snapshot writer (server)

- `SV_SendClientSnapshot` (`server/sv_snapshot.cpp:719`): builds the snapshot, returns immediately for bots (`:768`, bots are never sent anything), then writes `lastClientCommand` (long), the unacknowledged server commands, the snapshot, any download data, and calls `SV_SendMessageToClient`. The first message to a client that has not yet received the game directory also carries `svc_setgame` (`:723-761`).
- `SV_UpdateServerCommandsToClient` (`:225`) writes every reliable command from `reliableAcknowledge + 1` to `reliableSequence` in **every** message until they are acknowledged. This is the engine's reliable channel.
- `SV_WriteSnapshotToClient` (`:103`): `svc_snapshot` (byte), `serverTime` (long), delta distance (byte), snapshot flags (byte), area-mask length (byte) and bytes, delta player state (and vehicle state), then `SV_EmitPacketEntities` (`:36`).
- `SV_EmitPacketEntities`: merge of the new and old entity lists in ascending entity number; entity in both lists = delta from the old state (nothing written if unchanged); only in the new list = delta from the entity's baseline with `force`; only in the old list = remove; terminated by the 10-bit value `MAX_GENTITIES - 1` (1023).
- Base selection (`:110-134`): no base when `deltaMessage <= 0`, when the client is not active, when `outgoingSequence - deltaMessage >= PACKET_BACKUP - 3` (`PACKET_BACKUP` is 32, `qcommon/qcommon.h:98`), or when the base's entities have left the circular buffer. `deltaMessage` is the sequence the client last acknowledged.
- Padding for rate debugging: `sv_padPackets` adds `svc_nop` bytes (`:210-214`).
- `serverTime` (added 5 November 2026; OpenJK lines) is `sv.time` (`server/sv_snapshot.cpp:200`; `sv.time + client->oldServerTime` while a client loads after a map restart, `:198`). `sv.time` is the game clock: `SV_Frame` adds the real elapsed time to `sv.timeResidual` and runs game frames while the residual holds a whole frame, each advancing `sv.time` by `frameMsec = 1000 / sv_fps * com_timescale` ms (`server/sv_main.cpp:1169`, `:1226-1228`). The game clock therefore follows real time; with an `sv_fps` that does not divide 1000 the frames are slightly more frequent (33 ms frames, 30.3 per second at `sv_fps` 30), with no drift. Positions and trajectories (`trTime`) are computed for that clock. The real time a snapshot leaves the server is not sent.
- `server/sv_net_chan.cpp`: messages are XOR-"encrypted" with a key derived from challenge, server id and the acknowledged sequence; it has no security value. `svc_EOF` is appended here (`:140`).

## 2.5 The snapshot reader (client)

- `CL_ParseServerMessage` (`client/cl_parse.cpp:854`): reads `reliableAcknowledge` (long), then loops over `svc_*` operations (`qcommon/qcommon.h:233`: `svc_nop`, `svc_gamestate`, `svc_configstring`, `svc_baseline`, `svc_serverCommand`, `svc_download`, `svc_snapshot`, `svc_setgame`, `svc_mapchange`, `svc_EOF`) until `svc_EOF`.
- `CL_ParseSnapshot` (`:207`): reads server time, delta distance, flags, area mask, the player state (and the vehicle state when the new player state has a vehicle), then `CL_ParsePacketEntities` (`:95`). A snapshot is "valid" when its base is valid, has the right message number and its entities have not been overwritten (`:243-263`). The base for entities is the old snapshot's entity list, or the baseline of the entity number.
- `MSG_ReadString` and `MSG_ReadBigString` replace `%` by `.` (`msg.cpp:459-519`), a client-side sanitation applied after decoding.

## 2.6 Gamestate and downloads

- `SV_SendClientGameState` (`server/sv_client.cpp:697`): `lastClientCommand`, pending server commands, `svc_gamestate`, `reliableSequence`, every non-empty configstring (`svc_configstring`, short index, big string), every non-zero baseline (`svc_baseline` + `MSG_WriteDeltaEntity` from a zero entity with `force`), `svc_EOF`, the client number (long), the checksum feed (long), and, only when a random-mission manager exists, a compressed height map. It sets `gamestateMessageNum` to the current outgoing sequence (`:722`) and the client state to `CS_PRIMED`. In the captures no random-mission data was present.
- Re-send: `SV_ExecuteClientMessage` (`:1773`): a client message with the wrong `serverId` triggers `SV_SendClientGameState` again only if the client has acknowledged a message later than the gamestate (`:1819`). Nothing resends it otherwise.
- A client that is not active gets at most one message every 1000 ms (`server/sv_snapshot.cpp:694-706`).
- Downloads: `SV_WriteDownloadToClient` (`sv_client.cpp:1090`): `svc_download`, block number (short), file size (long) for block 0 only, block length (short), the data bytes. An error is signalled by block 0 with file size `-1` followed by a string (`:1136-1139`). The client (`cl_parse.cpp`, `CL_ParseDownload`) mirrors this.

## 2.7 Netchan

- `qcommon/net_chan.cpp:6-27` documents the packet header: 4 bytes sequence (high bit = fragmented), 2 bytes `qport` (client to server only), and for fragments 2 bytes start and 2 bytes length.
- `FRAGMENT_SIZE` = `MAX_PACKETLEN - 100` = 1300 (`:33-34`, non-Xbox). `Netchan_Transmit` (`:145`) splits a message when `length >= FRAGMENT_SIZE` (`:159`) and sends only the first fragment at once; `Netchan_TransmitNextFragment` (`:88`) sends the following ones. A message that is an exact multiple of 1300 bytes sends a last empty fragment (`:126-133`). Starting a new message while fragments are pending prints "Stomping Unsent Fragments" (`:154-157`).
- `Netchan_Process` (`:208`): rejects `sequence <= incomingSequence` (out of order or duplicate); for fragments requires `fragmentStart == fragmentLength` and otherwise drops the packet (`:294-315`), so one missing fragment loses the whole message; there is no retransmission at this level. The final fragment does not advance `incomingSequence` (`:356`).
- `SV_SendClientMessages` (`server/sv_snapshot.cpp:806`) sends one pending fragment per rate interval and skips building a new snapshot for that client while fragments remain (`:820-827`). `SV_SendMessageToClient` (`:652`) also flushes pending fragments before sending a new message.
- Rate: `SV_RateMsec` (`:623`) returns `(min(size, 1500) + 48) * 1000 / rate` (`HEADER_RATE_BYTES` = 48, `:622`). In `SV_SendMessageToClient` the message is "rate delayed" and the next snapshot is pushed back when `rateMsec >= snapshotMsec` (`:682-692`). LAN and loopback clients are never delayed (`:676-679`; `sv_client.cpp` gives LAN clients `rate` 99999 around `:1455-1465`).
- Client defaults: `rate` 4000 and `snaps` 20 (`client/cl_main.cpp:2636-2637`). The server clamps `rate` to 1000..90000 and `snaps` to 1..30 (`sv_client.cpp:1455-1500`); `sv_maxRate` defaults to 0 (unlimited) (`sv_init.cpp:829`).
- `MAX_MSGLEN` is 49152 (`qcommon/qcommon.h:150`); `MAX_RELIABLE_COMMANDS` is 128 (`:106`).

## 2.8 Client game module: how received positions are used

Read in detail: `cgame/cg_predict.c`, `cgame/cg_ents.c` (the functions below) and the start of `CG_ScanForCrosshairEntity`.

- Prediction (`cg_predict.c`, `CG_PredictPlayerState` at `:963`): the local player's predicted state starts from `cg.snap->ps` (or `cg.nextSnap->ps`), replays every unacknowledged `usercmd` with the same movement code as the server, and compares with the previous prediction at the same command time. A difference of more than 0.1 unit (`:1251`) is stored as a prediction error that decays over several frames (`:1255-1272`). The start state of the simulation is therefore the received player state: origin, velocity and view angles must be identical to the server's values.
- Solid entities in prediction: `CG_BuildSolidList` (`:46`) and `CG_ClipMoveToEntities` (`:207`). The position of movers comes from `BG_EvaluateTrajectory(&cent->currentState.pos, cg.physicsTime, ...)` (`:251`); the position of other bounding-box entities (including other players) comes from `cent->lerpOrigin` (`:276`). So received entity positions are part of the movement simulation.
- `CG_AdjustPositionForMover` (`cg_ents.c:3008`) moves a rider with its mover using the mover's `pos` and `apos` trajectories; it is called by the prediction code.
- `CG_CalcEntityLerpPositions` (`cg_ents.c:3078`) computes `lerpOrigin` and `lerpAngles`: interpolation between two snapshots for players (`TR_INTERPOLATE`), otherwise `BG_EvaluateTrajectory` of `trBase + trDelta * time`. An error in `trDelta` grows with the extrapolation time.
- `CG_ScanForCrosshairEntity` (`cg_draw.c:6072`) traces from the muzzle against these same positions to find the entity under the crosshair (name, health display). It is display only.
- `lerpOrigin` appears about 340 times in 19 cgame files (122 in `cg_players.c`, 110 in `cg_ents.c`, 32 in `cg_draw.c`, 16 in `cg_weapons.c`); `currentState.pos` or `.origin` about 90 times. These were counted, not read, apart from the files listed at the top of this section.

## 2.9 Licensing facts

- CleanJK / this tree: a grep over `CleanJK/Game/code` found 221 `.cpp`/`.h` files containing "GNU General Public License version 2 as" and about a dozen containing "either version 2 ... or (at your option) any later version". The grep is not exhaustive. Most files are therefore GPL v2 only, which cannot be combined with Apache-2.0 or GPL v3 code.
- Third-party code read (licence file in each repository): ryg_rans CC0; FSE library BSD-2-Clause; qbc MIT; nibrans CC0; fsc Apache-2.0; Turbo-Range-Coder GPL v3; Draco Apache-2.0 (header of `LICENSE` read from the repository). OpenZL: no licence file was found at the path tried.
- zstd: licence not read (the Python standard library module was used only to measure sizes).
