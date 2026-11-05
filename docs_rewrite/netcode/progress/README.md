# Netcode rewrite: progress

This folder records where the work stands, so that a new session (human or model) can resume without re-reading everything. One document per finished step of [PLAN.md](../PLAN.md); this page is the summary and is updated at the end of every session.

Last update: 5 November 2026 (steps 2, 4 and 5 done; next is step 6, the C++ codec).

## Where we are

| step of the plan | status | result | document |
|---|---|---|---|
| before the plan: study moved into this repository (OpenJK), re-checked | done | every number of the study and of the demo round trip reproduced here; OpenJK is wire-compatible with 1.01 except the pilot player state | [../08-openjk.md](../08-openjk.md), [../06-reproduction.md](../06-reproduction.md) section 6.7 |
| 1. prediction modes and rANS trick in the Python codec | **done** | 139.1 to **107.3** real bytes per snapshot (engine 337.6), 0 round-trip failures | [step-01-prediction-codec.md](step-01-prediction-codec.md) |
| 2. field descriptors from one source | **done** | `data/fields.json` (schema 2) generated from `msg.cpp`, offsets and C types by the compiler; tools read it; 139.1 and 107.3 unchanged; runtime overrides removed | [step-02-field-descriptors.md](step-02-field-descriptors.md) |
| 3. state recorder in the server | later | not needed before step 5 since demos are used | |
| 4. demo corpus | **done** | 1,116 demos indexed (148 h); 60 train + 20 held-out demos (held out by map), 427,468 messages; round trip 0 failures; pilot rule removed from `codec2.py` | [step-04-demo-corpus.md](step-04-demo-corpus.md) |
| 5. measurements on the demos | **done** | held-out snapshots 69.0 bytes against 155.1 for the engine (ratio 0.445; all messages 0.483); held-out within 6 % of training; one global table set, no age context; 0 round-trip failures | [step-05-demo-measurements.md](step-05-demo-measurements.md) |
| 6 to 13 | to do | | |

## Next actions

1. Step 6: the codec library in C++ ([PLAN.md](../PLAN.md) step 6), with the table design of step 5 ([step-05-demo-measurements.md](step-05-demo-measurements.md), "What step 6 needs").
2. Corpus scope (owner, 5 November 2026): no race, no JA+ or JAPRO demos will come. Possible addition: one FFA bot demo on this branch (base game module), only for the FFA game type.

## Decisions taken (with the date)

| date | decision | where it is written |
|---|---|---|
| 4 Nov 2026 | Measurements stay referenced to the original 1.01 server (the captures) | [../08-openjk.md](../08-openjk.md) |
| 4 Nov 2026 | Owner's goals: no loss of precision (values identical to today's or more precise, never less); no compatibility constraint; as few network settings as possible, computed at run time; netcode reusable by other JKA forks | [../README.md](../README.md), "Owner's goals" |
| 4 Nov 2026 | Do not restore the 1.01 pilot rule (`size - 82`): it was a bug | [../08-openjk.md](../08-openjk.md) section 8.1 |
| 4 Nov 2026 | No rounding of positions, neither on the client nor on the server (float32 is finer than any useful grid) | [../04-recommendations.md](../04-recommendations.md) section 4.3 |
| 4 Nov 2026 | Settings to remove and compute at run time; new debug cvars and commands are welcome | [../04-recommendations.md](../04-recommendations.md) section 4.10 |
| 5 Nov 2026 | Position prediction in integer arithmetic (0.5 byte more than float, same bits on every compiler) | [step-01-prediction-codec.md](step-01-prediction-codec.md) |
| 5 Nov 2026 | The plan is a separate document, [PLAN.md](../PLAN.md); progress is recorded here; the study documents are renumbered 01 to 08 | this folder |
| 5 Nov 2026 | Client demos (`.dm_26`) replace new proxy recordings: same server message, checked in the client code; the state recorder (step 3) moves later | [PLAN.md](../PLAN.md) step 4 |
| 5 Nov 2026 | Target platforms: Windows and Linux, x86 and x64, client and dedicated server; not macOS or ARM for now. Linux is built and tested alongside Windows from step 6 | [../README.md](../README.md), "Owner's goals" item 5; [../07-transport-study.md](../07-transport-study.md) section 7.7 |
| 5 Nov 2026 | Every legacy choice may be redone if it improves the result, including field types and the time base (owner) | [../README.md](../README.md), "Owner's goals" item 2 |
| 5 Nov 2026 | Descriptors carry the C type of each field (schema version 2); the new codec sends booleans as "changed" only and codes `user*` fields by C type | [../04-recommendations.md](../04-recommendations.md) section 4.2; [step-02-field-descriptors.md](step-02-field-descriptors.md) |
| 5 Nov 2026 | `serverTime` stays the game clock; a real send timestamp is evaluated in step 10 | [../04-recommendations.md](../04-recommendations.md) section 4.9; [../02-engine-code-findings.md](../02-engine-code-findings.md) section 2.4 |
| 5 Nov 2026 | Runtime width overrides (`netf_overrides.txt`, `psf_overrides.txt`) removed from the new protocol; mod compatibility is never a criterion, not even a question to ask | [step-02-field-descriptors.md](step-02-field-descriptors.md); [../README.md](../README.md), "Owner's goals" item 2 |
| 5 Nov 2026 | Demo selection: exclude files with more than 5 % undecodable messages or under 3 minutes; held-out set by map (about 20 % of the maps per game type, seed 1); 60 training and 20 held-out demos, first 5,400 messages each | [step-04-demo-corpus.md](step-04-demo-corpus.md) |
| 5 Nov 2026 | `codec2.py` codes all 140 pilot fields (1.01 rule dropped): real servers send more than 58 | [step-04-demo-corpus.md](step-04-demo-corpus.md) |
| 5 Nov 2026 | Branch defaults: `sv_fps` 20 (upstream 40), `sv_clientRate` 25000 (upstream 50000); other defaults upstream (`sv_snapsPolicy` 1, `cl_maxpackets` 63); rate and snaps policies and the pilot fix are kept | `codemp/server/sv_init.cpp:960`, `:983`; [../08-openjk.md](../08-openjk.md) section 8.2 |
| 5 Nov 2026 | Target game module: the base one (OpenJK's), players sent as `TR_LINEAR_STOP` with `g_smoothClients` 1, never rounded; game code keeps the base game's precision; no `TR_INTERPOLATE` recording needed | [../08-openjk.md](../08-openjk.md) section 8.3 |
| 5 Nov 2026 | One global table set, without the base age as a context (per game type and age context each save about 2 %, less than the differences between maps); one table set must work at any `sv_fps` | [step-05-demo-measurements.md](step-05-demo-measurements.md); [../04-recommendations.md](../04-recommendations.md) section 4.5 |
| 5 Nov 2026 | Ideal-byte benchmarks (`bench2.py`, `bench5.py`) not re-run on the demos, real codec instead; `TR_INTERPOLATE` check dropped (base game module) | [step-05-demo-measurements.md](step-05-demo-measurements.md) |
| 5 Nov 2026 | Step 2 before step 4, since no demo is available yet; field offsets computed by a small CMake program (`tools/fieldinfo`) built by the owner, not by parsing headers | [step-02-field-descriptors.md](step-02-field-descriptors.md) |

## Open questions for the owner


- Five transport questions of [../07-transport-study.md](../07-transport-study.md) section 7.7 (encryption, player count, IPv6, server browser, Apache-2.0 / OpenSSL licence position; platforms is answered). Not needed before step 9.

## Known issues found on the way (not fixed)


- The `user*` mod fields are networked 1 bit wide, floats included: only the lowest bit reaches the client unless a mod widens them with the overrides files ([../02-engine-code-findings.md](../02-engine-code-findings.md) section 2.2).

- With `sv_ratePolicy 2` and `sv_maxRate 0`, the client rate becomes 0 and `SV_RateMsec` divides by zero (`codemp/server/sv_client.cpp:1090`, `codemp/server/sv_snapshot.cpp:679`). Read only, not tested.

## State of the repository

- Nothing of this work is committed unless the owner did it. Check `git status` at the start of a session.
- This repository is the OpenJK fork `E:\Git\jka\OpenJK_Netcode`, branch `feature/poc-netcode`.
- Branch changes outside `docs_rewrite/netcode/`: the two defaults in `codemp/server/sv_init.cpp` (`sv_fps` 20, `sv_clientRate` 25000) and `*.dm_26` in `.gitignore`. Nothing in the game module.
- Python caches (`*.pkl`) are rebuilt by the tools and ignored by git. Delete `bdata.pkl` and `fullmsgs.pkl` after regenerating `data/fields.json`.
- `tools/fieldinfo/build/` is ignored by git; the owner builds `fieldinfo.exe` there.

## How to resume

1. Read this page, then the document of the last finished step, then the next step in [PLAN.md](../PLAN.md).
2. Check that the tools still give the recorded numbers: from `docs_rewrite/netcode/tools/python`, `python run_full.py` (study codec: snapshot mean 139.1, 0 failures) and `python run_pred.py pi --fuzz` (step 1: 107.3, 0 failures). About 2 minutes in all.
3. At the end of the session, write the step document, update the tables above, and record new decisions with their date.
