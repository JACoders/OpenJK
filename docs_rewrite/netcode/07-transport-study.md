# 7. Transport study (exploratory)

**Status: exploratory. Do not act on this document before the new codec exists (steps 1 to 8 of [PLAN.md](PLAN.md)).** It records what was read in the engine and about candidate libraries, derives requirements from the measurements of [03](03-results.md), and proposes how to decide. Nothing here was prototyped or measured on a network. Statements about libraries come from reading their repositories on 4 November 2026 (README, licence, standard, headers, a line count); they were not built or run.

Why wait for the codec:

- The codec is independent of the transport and carries the largest measured gains (337.6 to 139.6 bytes per snapshot). The transport can be changed later without touching it.
- Several transport requirements depend on what the codec produces: message sizes decide whether fragmentation matters, the acknowledgement record decides the base age, and the table-update procedure decides what must be reliable.
- A library choice should be made with the real codec payloads, on a prototype, with measured overhead and CPU. Doing it now would repeat the codec study's main mistake (modelling instead of measuring) for a component that is more expensive to replace.

## 7.1 How the current transport works (read in the code)

### Connection

This section describes the 1.01 code. OpenJK (this repository) replaces the `rand()` challenge by a stateless HMAC-MD5 challenge and rate-limits connectionless packets per address; see [08-openjk.md](08-openjk.md) section 8.3.


- Out-of-band (OOB) packets carry the handshake: `getchallenge`, `challengeResponse`, `connect`, plus `getinfo` and `getstatus` for server browsers and master servers. The `connect` string is compressed with the adaptive Huffman coder (`Huff_Compress`).
- `SV_GetChallenge` (`server/sv_client.cpp:31`) creates a challenge `((rand() << 16) ^ rand()) ^ svs.time` per client address, in a table of `MAX_CHALLENGES` = 1024 entries (`server/server.h:190`), and answers at once for LAN clients. `rand()` is not a cryptographic generator. `AUTHORIZE_TIMEOUT` = 5000 ms (`server.h:192`) refers to the original CD-key authorisation, which no longer exists.
- `SV_DirectConnect` (`:221`) checks the protocol version, reads the challenge and the `qport` from the user-info string.
- The client sends at most one packet per second until it has a gamestate (`client/cl_input.cpp:1550-1559`); the server sends at most one message per second to a client that is not active (`server/sv_snapshot.cpp:694-706`).
- Timeouts: `cl_timeout` and `sv_timeout` default to 200 (seconds, `client/cl_main.cpp:2568`, `server/sv_init.cpp:853`), `sv_zombietime` 2.
- Sockets: IPv4, IPX and loopback only (`win32/win_net.cpp` has no IPv6 address family; the address types in `qcommon/qcommon.h:114` are IP, IPX, loopback). IPX is obsolete.

### Packets in play

- Server to client: 4-byte sequence (high bit = fragment), then the message (02, section 2.7). Client to server: 4-byte sequence, 2-byte `qport`, then the message.
- Client message (`CL_WritePacket`, `cl_input.cpp:1608`): `serverId` (4), acknowledged server message sequence (4), last server command sequence (4), any unacknowledged client commands (type byte, 4-byte number, string), then `clc_move` or `clc_moveNoDelta`, a command count, and the delta-coded `usercmd`s. `usercmd`s are XOR-keyed with `checksumFeed ^ serverMessageSequence ^ hash(last server command)` (`:1686-1696`).
- Send rate: `cl_maxpackets` default 30, clamped to 15..100 (`cl_main.cpp:2588`, `cl_input.cpp:1572-1582`); LAN and loopback clients send every frame (`:1562-1569`). `cl_packetdup` (0 to 5) repeats earlier `usercmd`s in each packet so that a lost packet does not lose input (`:1655-1665`).
- Acknowledgement: one number in each direction, the latest message received. Nothing records which earlier messages were lost.
- Reliable commands in both directions: repeated in every packet until acknowledged, window of 128 (`MAX_RELIABLE_COMMANDS`).
- Fragmentation and rate: see 02, section 2.7. Downloads travel on the same channel (the client sends at least 50 ms apart while downloading, `cl_input.cpp:1544-1547`).

### What the "encryption" is

- Server to client (`server/sv_net_chan.cpp`, `SV_Netchan_Encode`): every byte after the first few is XORed with a **one-byte key** (`byte key`) that starts as `client->challenge ^ netchan.outgoingSequence` and is modified at each byte by the characters of the last client command string. Client to server: the `usercmd` key above (an int, from public or previously exchanged values).
- The challenge is sent in the clear during the handshake, and the sequence number is in the clear in every packet. There is no secret. There is no integrity check. The scheme only obfuscates.

## 7.2 Security: is it the same as with Huffman?

Short answer: **Huffman never gave any security, and the new codec gives none either. Nothing gets worse from the codec by itself, but it adds a new parser that must be written defensively.** Compression is not encryption: it makes data smaller, not secret, and does not detect modification.

| threat | today (Huffman + XOR) | new codec alone | with an authenticated, encrypted transport |
|---|---|---|---|
| Eavesdropping (reading packets on the path) | possible; the XOR is trivially broken | possible | prevented |
| Tampering (changing packets on the path) | possible, no integrity check | possible | detected and dropped |
| Spoofing / injection by someone off the path | an attacker needs the sequence number and, client to server, the `qport` (judgement from `Netchan_Process`); a one-byte XOR key does not stop it | the same | prevented (packets must carry a valid authentication tag) |
| Replay of old packets | the netchan drops `sequence <= incomingSequence`, which stops simple replays | the same | explicit replay window |
| Connection-flood and amplification | `getstatus` and `getinfo` answers are presumably larger than the requests (sizes not measured); challenge table of 1024 with `rand()`; no cryptographic cookie | the same | depends on the library (netcode has connect tokens and amplification protection per its README) |
| Malicious server attacking a client through malformed data | the existing readers have some checks (for example `CL_ParseSnapshot` validity tests, the `readcount > cursize` tests in `CL_ParseServerMessage`); their completeness was not audited | **new attack surface**: a new decoder with tables, gap-coded indices, string lengths, entity numbers | same as before for the parser |
| Malicious client attacking the server | the same for what the server parses (`usercmd`, client commands) | unchanged by the codec steps (server-to-client codec only); becomes a new surface if the `usercmd` stream is recoded | same |
| Cheating through packet inspection (for example reading entity data a client should not know) | the server decides which entities to send (`SV_AddEntitiesVisibleFromPoint`); the codec does not change that | the same | the same |

What this implies:

1. **Do not describe the codec as protection.** It is neither better nor worse than Huffman on confidentiality or integrity.
2. **Harden the decoder.** Bounds-check every index and length (entity number below `MAX_GENTITIES`, field index below the table length, string and download lengths), limit the number of operations per message, fail closed (drop the message) and fuzz it in CI. The plan has this in 04 section 4.8 and step 8 of the plan. The decoder never writes outside a fixed-size output structure, which is the usual class of exploitable bugs.
3. **A decompression-bomb risk does not apply** in the usual sense: every decoded symbol is bounded by the schema (a message produces at most one value per field per entity and at most `MAX_GENTITIES` entities).
4. **Real security is a transport decision.** If cheating by packet injection, spoofing or traffic reading matters to the project, an authenticated encrypted transport is needed (section 7.4). That has a cost in bytes (section 7.4, "What the numbers imply for the choice") and in connection design (connect tokens, section 7.4).
5. **Side channels from compression**: compressing attacker-influenced data together with secrets and then encrypting it can leak information (the CRIME / BREACH class). This does not apply to the design here, because the codec uses static tables with no cross-packet state and the secret (keys) is never inside the coded payload. Keep it that way.

## 7.3 Requirements derived from the study

Evidence is from [03](03-results.md) unless noted. "Must" means the codec's gains are lost or the design is unsafe without it; "should" is a measured benefit; "could" is a judgement.

| id | requirement | priority | evidence |
|---|---|---|---|
| T1 | Precise, fast acknowledgement of recent packets (an ack bitfield of at least the last 32 packets), sent in every client packet | should | latency, not loss, drives message size: +21 % at 100 ms RTT, +112 % at 400 ms (3.9); with one acknowledged number the server cannot tell which of several recent snapshots arrived |
| T2 | Unreliable sequenced channel for snapshots that tolerates loss and drops late packets | must | the delta scheme already assumes it |
| T3 | Reliable ordered channel for the gamestate, server commands, client commands and downloads, with its own fast retransmission | should | the engine resends a lost gamestate only after about 1,000 ms (3.10); a lost fragment loses the whole message |
| T4 | Message size above the MTU handled by the transport (fragmentation with reassembly), or avoided by the codec | should | 2 of 8,836 messages exceed 1,300 bytes for ordinary play; the gamestate (4,024 bytes) and crowd snapshots do |
| T5 | Low fixed overhead per packet | should | the solo payload is 7 to 9 bytes; the netchan header is 4 bytes plus 28 bytes of IP and UDP |
| T6 | Bandwidth control that adapts, with no setting to tune (owner's goal 3) | must | at the default `rate` 4000 the engine delays 93 % of snapshots (3.10) |
| T7 | Authenticated and encrypted packets | could / decision for the owner | the current scheme has none (7.2) |
| T8 | Connection handshake with stateless, unpredictable cookies and amplification limits | should | challenge from `rand()`, `getstatus` amplification (7.1, 7.2) |
| T9 | IPv6 | should | no IPv6 in `win_net.cpp` (7.1) |
| T10 | 32 to 64 or more simultaneous clients on a dedicated server (Windows and Linux) | must | the owner's player count is not fixed (`MAX_CLIENTS` is 32 in OpenJK); the engine has a Linux dedicated server |
| T11 | Licence compatible with GPL v2 (BSD-2, BSD-3, MIT, zlib, CC0), no Apache-2.0 / GPL v3 dependency in the linked code, or an explicit decision on the licence of the whole | must | most CleanJK files are GPL v2 only (02, section 2.9) |
| T12 | The server-browser and master-server queries (`getinfo`, `getstatus`, heartbeats) keep working or are replaced | must | the server list is how players find games |
| T13 | Statistics: RTT, jitter, loss per connection, to drive T6 | must | step 10 of the plan computes the settings from these measurements; today only the ping in the snapshots exists |
| T14 | A way to test with loss, latency and jitter (a simulator) | should | the study's loss results are models |

## 7.4 Candidate approaches

### What was read about each library

All data below is from the repository contents on 4 November 2026; no library was compiled or run for this study. Line counts are `wc -l` of the main source files.

| | netcode + reliable (+ yojimbo) | GameNetworkingSockets (Valve) | ENet | in-place improvement of the netchan |
|---|---|---|---|---|
| Licence | BSD-3-Clause (all three: Más Bandwidth LLC) | BSD-2-Clause (Valve) | MIT | the engine's (GPL v2) |
| Size | netcode about 11,000 lines, reliable about 4,600, yojimbo about 11,000 (C and C++) | about 70,000 lines in `src/` plus protobuf-generated code | about 6,500 lines (C) | small (a few hundred lines for acks and reliability, more for crypto) |
| Dependencies | libsodium (a copy is in the netcode repository) | OpenSSL 1.1.1+ or libsodium, **and** Google protobuf (and Abseil with protobuf 22+), C++17 compiler for builds with recent protobuf | none | none (libsodium if encryption is added) |
| Maximum clients | netcode 256 (`NETCODE_MAX_CLIENTS`), yojimbo "100 players or less" (its README) | not stated in what was read | peer id up to 0xFFF (4,095) | the engine's |
| Encryption and authentication | yes: AEAD (ChaCha20-Poly1305), connect tokens issued by a backend, replay protection; the README claims protection against malicious packets, replays and amplification | yes: AES-GCM-256 per packet, Curve25519 key exchange and certificates | none | optional, to be added |
| IPv6 | yes (IPv4 and IPv6) | yes | no AF_INET6 found in its socket code | to be added |
| Reliability | reliable: acknowledgement with a 32-bit ack mask (header 4 to 9 bytes), duplicate detection, RTT / jitter / loss estimates; yojimbo adds reliable-ordered and unreliable messages and blocks | reliability layer based on ack vectors (DCCP / QUIC style), reliable and unreliable messages, lanes with priorities | reliable and unreliable channels | to be added |
| Fragmentation and reassembly | yes (reliable: `fragment_above` 1024 by default, up to 16 fragments of 1024 bytes in the defaults seen) | yes, messages larger than the MTU, with retransmission for reliable ones | yes | exists; to be improved |
| Maximum packet / MTU | netcode payload 1 to 1,200 bytes | MTU-aware | default MTU 1,392 | 1,300-byte fragments |
| Per-packet overhead (read from the standards) | netcode: prefix byte (1) + sequence (1 to 8 bytes) + 16-byte authentication tag = 18 to 25 bytes; reliable: 4 to 9 bytes more | encrypted per-packet overhead not read | small | 4 bytes today |
| What it does not do | game-state serialisation, compression (yojimbo has a bit packer) | "higher level serialization of entities, delta encoding", "compression" (its README) | encryption, IPv6 (as found) | |
| Connection model | clients need a **connect token** from a backend (REST) in the documented design; public open servers need a token service | connection-oriented API; P2P and relay features | simple connect | the existing OOB handshake |
| Last commit seen | 29 October 2026 (netcode, yojimbo), 27 October 2026 (reliable) | 2 October 2026 | 23 June 2026 | n/a |
| Concerns to check | connect-token backend for public servers; libsodium build on all targets; thread model (single-threaded); crediting requirement in the README (four libraries) | heavy dependency chain; build on Windows and Linux; the OpenSSL 3 licence (Apache-2.0) in combination with GPL v2 code needs a legal reading | no encryption, no IPv6 | design and test effort |

Not covered: QUIC implementations (msquic, quiche, ngtcp2), SteamNetworkingSockets as shipped inside Steam, raw sockets with a custom protocol on top of libsodium. They were not read.

### Options

**A. Improve the netchan in place.** Keep the OOB handshake and the sequence-numbered packets, and add: an ack bitfield; a reliable channel with its own retransmission for the gamestate, commands and downloads; fragmentation limited to what remains; stateless cookies for the challenge; IPv6 sockets; optionally an AEAD wrapper (libsodium or another small library) replacing the XOR. Pros: smallest change, keeps server browsers and tools working, overhead stays at 4 bytes unless encryption is added. Cons: engineering effort is ours, and the security design must be done correctly (nonces, replay windows, key exchange).

**B. netcode + reliable (or yojimbo).** Pros: authenticated encryption, replay protection, IPv6, acks and fragmentation already written and fuzz-tested by their authors (the repositories contain fuzz targets and a security policy). Cons: the connect-token model needs a backend for public servers; per-packet overhead of about 22 to 34 bytes before IP and UDP (18 to 25 for netcode plus 4 to 9 for reliable) against 4 today; the OOB queries for server browsers must still be kept; game-specific channel configuration must match on both sides.

**C. GameNetworkingSockets.** Pros: mature reliability and encryption, lanes, simulation tools. Cons: large dependency chain (protobuf, OpenSSL or libsodium), 70,000 lines to integrate, a message-oriented API that replaces the whole connection model; the licence interaction of OpenSSL needs a legal reading.

**D. ENet.** Pros: small, MIT, reliable and unreliable channels. Cons: no encryption, no IPv6 as found, and its acknowledgement model is not designed around "the server needs to know exactly which snapshots arrived".

**E. A thin new layer on libsodium** (the codec payload, a packet header with sequence and ack bitfield, AEAD, a stateless cookie handshake). This is A with encryption designed in. It avoids the connect-token backend but moves the security design onto the project.

### What the numbers imply for the choice

Per-packet overhead matters most when the payload is small. With the study's payloads (140 bytes mean for the crowded test, 9 bytes alone) and the overheads read above (calculation, not measurement; the fixed 28 bytes of IP and UDP are added to both):

| transport | overhead on top of IP and UDP | 140-byte payload on the wire | 9-byte payload on the wire |
|---|---|---|---|
| current netchan | 4 | 172 | 41 |
| in-place with an ack bitfield (assume 4 + 8) | 12 | 180 | 49 |
| netcode + reliable | 22 to 34 | 190 to 202 | 59 to 71 |

So an authenticated transport adds roughly 10 to 17 % to a normal snapshot and 44 to 73 % to an idle one. Compared with the codec's savings (about 58 %) this is affordable for normal traffic; it matters for idle clients, spectators and low-rate links.

## 7.5 Proposed way to decide (to start once the codec exists)

1. **Fix the owner's constraints** (questions in 7.7).
2. **Prototype the in-place improvements in the simulator** (ack bitfield, faster gamestate resend) using `sim_loss.py`-style simulation with the real codec; measure the effect on bytes and gamestate time.
3. **Build a small test harness**, outside the game, that sends the codec's recorded messages through (a) the netchan model, (b) netcode + reliable, and optionally (c) GameNetworkingSockets, with emulated latency, jitter and loss. Measure: overhead per packet, CPU per client at 64 clients, behaviour under 5 / 10 / 20 % loss, behaviour with large messages (gamestate), reconnect time.
4. **Review the licences and builds** on the target platforms (Windows and Linux dedicated server), including libsodium and OpenSSL if used, with a legal reading where the licence is not plainly compatible.
5. **Decision gate.** Choose by this order: licence and maintainability first, then behaviour under loss and latency, then overhead, then integration cost. Record the decision with the measurements.
6. Only then integrate (steps 9 to 11 of the plan).

Design notes whichever option is chosen:

- The server should compute `rate` and the snapshot interval from the real message size and the connection's measured bandwidth, not from the fixed formula, and the default should be reconsidered (the engine's 4,000 bytes per second delays 93 % of its own snapshots).
- Reliable data (gamestate, commands, downloads) should not block snapshots; the engine currently suppresses new snapshots while fragments of an earlier message are pending (`sv_snapshot.cpp:820-827`).
- The OOB queries for server browsers (`getinfo`, `getstatus`) keep a plain path; they should be rate-limited per address and their answers kept no larger than the request where possible.
- `usercmd` coding (about 30 packets per second per client by default) is a separate study; it is not covered by the codec results.

## 7.6 What was not read or not done

- `client/cl_main.cpp` connection state machine in detail, `server/sv_main.cpp` query rate limiting, master-server heartbeat code, `qcommon/net_ip` and `win32/win_net.cpp` beyond the address types, the Linux socket code.
- `usercmd` coding (`msg.cpp:685-860`) beyond noting the key.
- Voice, server-side demo recording and other users of the netchan.
- Any benchmark or build of a transport library. Licences were read from the repository files; library behaviour comes from their documentation.
- Whether the encrypted-transport overhead is acceptable for the owner's target: that is a product decision, not a measurement.

## 7.7 Questions for the owner (needed before step 1)

Answered so far (4 November 2026): compatibility with the old protocol and mods is not required; network settings should be computed, not configured. The questions below are still open.

1. Is authenticated encryption a requirement? If yes, is a backend that issues connect tokens acceptable, or must anyone be able to connect to a public server directly?
2. Target number of simultaneous players per server (the answer decides between libraries with client caps, 100 for yojimbo as documented, and larger designs).
3. Platforms for the client and the dedicated server (the engine has Windows and Linux dedicated servers). **Answered 5 November 2026:** Windows and Linux, x86 and x64, client and dedicated server; not macOS or ARM for now.
4. IPv6 required at release?
5. Are the existing server browser and master-server protocols to be kept?
6. Is the legal position on linking Apache-2.0 / OpenSSL 3 components with GPL v2 code acceptable, or must the transport avoid them?
