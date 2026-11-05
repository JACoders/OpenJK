# Netcode rework study: replacing the Huffman coder of Jedi Academy multiplayer

Status: analysis finished, nothing implemented in the engine yet. Measurements were taken on 3 and 4 November 2026 against the original 1.01 dedicated server. The folder lives in this repository, a fork of OpenJK and the proof-of-concept base of the rewrite: the whole tool chain was re-run here with identical results, the code readings were re-checked in OpenJK ([09-openjk.md](09-openjk.md)), and prediction variants were measured (03, section 3.12).

This folder records an investigation into replacing the network coding of Jedi Academy multiplayer (the Huffman-based `MSG_*` layer and the surrounding snapshot protocol) in a standalone game derived from CleanJK (itself derived from OpenJK, GPL v2). The owner is willing to break protocol compatibility and to change structures, protocol and third-party mods.

The work was done by an AI assistant (Claude) working with the owner. Every number in these documents was produced by a script kept in `tools/`; where a statement is a reading of the source code it carries a `file:line` reference; where it is an estimate or an assumption it is marked as such. Nothing was taken from memory of the original game's behaviour without checking it in the code.

## Owner's goals (stated 4 November 2026)

These constrain every recommendation in this folder:

1. **No loss of precision.** Values may be encoded differently (indices, prediction with a residual, another format) and may even be more precise than today, but never less precise than what the client receives from the original code. In practice this means the decoded values are identical to today's: float32 positions have a resolution of about 0.00006 unit near 1,000 units, finer than any fixed grid worth sending, so rounding to a grid (on the client or on the server) would lose precision and is excluded.
2. **No compatibility constraint.** Protocol, structures, mods and the client's prediction and interpolation code can all be rewritten; the existing code may contain bugs or poor calculations.
3. **As few network settings as possible.** Values such as `rate`, `snaps`, `cl_maxpackets`, `cl_packetdup` or the base selection should be computed or measured at run time (bandwidth, RTT, loss), not set by hand.
4. **Reusable.** The netcode should be generic enough to be reused by other JKA forks: driven by the field tables (`entityStateFields`, `playerStateFields`) and by the common trajectory fields, not by rules of one game module.

## What the investigation found, in ten lines

1. Replacing only Huffman with a better entropy coder saves 4.6 % of the bytes. It is not worth doing alone.
2. The gain is in the model: one context per field, no `lc` byte, delta coding chosen per field, `eType` first, angles as 16-bit indices. That gives 337.6 bytes per snapshot down to 139.6 bytes (-58.6 %) with every value the client sees unchanged.
3. Angles sent by the engine are already multiples of 360/65536 (99.9 % of them), so coding them as 16-bit indices is lossless. Positions are not on any grid and must stay exact, because the client prediction code and the collision code use them.
4. The entropy coder family hardly matters: a static trained rANS (ryg_rans, CC0) or tANS (FSE, BSD-2) are within 1 % of the ideal; adaptive coders (qbc, nibrans) and general-purpose compressors (zstd, deflate, lzma) do not beat the engine's own trained Huffman table on 300-byte packets.
5. CPU is not a constraint: the current Huffman code uses about 0.9 % of a core for 64 clients at 20 snapshots per second; the new codec uses about 0.1 to 0.3 %.
6. Latency of acknowledgements matters far more than packet loss: the delta base is the newest snapshot the server knows the client has, so message size grows with round-trip time (+21 % at 100 ms, +112 % at 400 ms) but only +5 % at 20 % loss.
7. With the default client `rate` of 4000 bytes per second the engine delays 93 % of snapshots (8.2 per second); the new codec delays 39 % (14.6 per second).
8. The gamestate drops from 6 to 4 fragments (7048 to 4024 bytes on the 5-minute capture) and fewer fragments means fewer lost gamestates.
9. The codec round-trips exactly on 8,842 real messages and on 27,225 synthetic messages (vehicles, pilot player states, downloads, extreme values, any base age). A worst-case input (every field of every entity random) costs 1.34 times the engine's size, so a raw-format escape path is required.
10. Nothing here is measured with several human players, on other maps or other game modes. That is the largest open point.
11. Prediction adds a further 22 % in ideal bytes, still lossless: time fields against `serverTime`, positions predicted from the velocity, angles to second order (136.9 to 106.1 bytes; 03, section 3.12). Not yet in the real codec.
12. OpenJK sends the same bitstream as 1.01 (same tables, same Huffman table; the captures decode with OpenJK's tables), except the pilot player state (all 140 fields instead of 58). Its defaults differ (`sv_fps` 40, `snaps` 40, client rate forced to 50000), which changes the base age for a given RTT; this branch sets `sv_fps` 20 and the 1.01 conditions are restored with cvars ([09](09-openjk.md)).

## Documents

| file | content | read it when |
|---|---|---|
| [01-methodology.md](01-methodology.md) | How the study was done, step by step: capture, decoding, validation, benchmark design, simulation design, what each number means | you want to judge or reproduce the method |
| [02-engine-code-findings.md](02-engine-code-findings.md) | What was read in the engine and the client game code, with `file:line` references | you need the facts behind the design |
| [03-results.md](03-results.md) | All measured results in tables, with the script that produced each one | you need the numbers |
| [04-recommendations.md](04-recommendations.md) | Recommended algorithms, schema, precision policy, transport policy, and the reasons | you are deciding what to build |
| [05-implementation-plan.md](05-implementation-plan.md) | Ordered steps (1 to 13) from the Python codec to the engine, transport and settings, each with a deliverable, a check and its dependencies; tests and risks | you are going to implement it |
| [06-verification-guide.md](06-verification-guide.md) | Claims with confidence levels, known weaknesses, mistakes made on the way, and ideas that were not explored | you are going to re-check the work or look for better solutions |
| [07-reproduction.md](07-reproduction.md) | Tools, file formats, commands, data checksums, environment | you want to re-run something |
| [08-transport-study.md](08-transport-study.md) | **Exploratory.** How the current transport and its "encryption" work, security implications, transport requirements derived from the measurements, candidate libraries as read (not run), and how to decide. Wait for the codec before acting on it | you are going to touch the transport |
| [09-openjk.md](09-openjk.md) | What differs in OpenJK (wire format, defaults, challenge, demos), a bug found, and where every function of 02 and 08 is in this tree | you work in this repository |
| `tools/` | The Python and C code used (see 07) | |
| `data/` | The three real captures, a copy of `msg.cpp` (2.6 MB, 0.5 MB, 0.1 MB), the C benchmark input files (13 MB and two small crowd scenarios) | you want to re-run anything |
| `bin/` | `jkabench.exe` and `jkabench_libs.exe`, the Windows x64 builds used for the CPU measurements | you want to re-measure speed without compiling |

## Suggested reading order

- Owner or lead: this file, then 04, then 05; then 08 when the transport comes up (it is exploratory).
- Engineer implementing: 02, 09, 04, 05, then 03 as reference.
- A model or reviewer asked to re-verify and improve the work: 06 first, then 01, 02, 03, 07, and finally 04 and 05 to see what is proposed.

## Conventions used in these documents

- "Engine" means the original 1.01 server code (`E:\Git\jka\jediacademy\codemp`), which produced the captures. "OpenJK" or "this repository" means the proof-of-concept base (`codemp/`), a fork of OpenJK; its differences are in 09. "CleanJK" means the working base of the new game (`E:\Git\jka\CleanJK\Game\code`), which has the same network structure.
- "Snapshot" is the server-to-client message that carries one player state and the visible entities. "Message" is one server-to-client packet payload; it can hold server commands, a snapshot, a gamestate and other operations.
- "Base" is the snapshot a delta is computed against. "Base age" is the distance in snapshots between the message and its base.
- "Ideal bytes" are the code length a static entropy coder approaches (the sum of `-log2 p`); "real bytes" are produced by an actual encoder.
- Sizes are payload bytes only, before the netchan header (4 bytes for server to client), UDP and IP (28 bytes) and any encryption tag.
- Percentages of CPU are the share of one core used by encoding at 20 snapshots per second for N clients.
