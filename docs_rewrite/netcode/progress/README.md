# Netcode rewrite: progress

This folder records where the work stands, so that a new session (human or model) can resume without re-reading everything. One document per finished step of [PLAN.md](../PLAN.md); this page is the summary and is updated at the end of every session.

Last update: 5 November 2026 (step 2 done; next is the demo reader, step 4, when the first demo is supplied).

## Where we are

| step of the plan | status | result | document |
|---|---|---|---|
| before the plan: study moved into this repository (OpenJK), re-checked | done | every number of the study reproduced in this repository; OpenJK is wire-compatible with 1.01 except the pilot player state | [../08-openjk.md](../08-openjk.md), [../06-reproduction.md](../06-reproduction.md) section 6.7 |
| 1. prediction modes and rANS trick in the Python codec | **done** | 139.1 to **107.3** real bytes per snapshot (engine 337.6), 0 round-trip failures | [step-01-prediction-codec.md](step-01-prediction-codec.md) |
| 2. field descriptors from one source | **done** | `data/fields.json` (schema 2) generated from `msg.cpp`, offsets and C types by the compiler; tools read it; 139.1 and 107.3 unchanged; runtime overrides removed | [step-02-field-descriptors.md](step-02-field-descriptors.md) |
| 3. state recorder in the server | later | not needed before step 5 since demos are used | |
| 4. demo corpus | **next**, with the owner | `.dm_26` reader to write; the owner supplies demos | |
| 5 to 13 | to do | | |

## Next actions

1. Step 4 of the plan: write the `.dm_26` reader in `jkadec.py` (strip the final `svc_EOF`; do not measure the rebuilt gamestate) and test it on the first demo supplied. `data/demos/` does not exist yet.
2. Owner: put demos in `data/demos/train/` and `data/demos/holdout/` (the held-out ones chosen before any measurement): protocol 1.01 (`.dm_26`), several humans, many maps, FFA, CTF, team FFA, duel, siege, race, several mods; note for each the map, mode, mod, player count, and `sv_fps` if known.
3. Then step 5: re-run the measurements on the demos.

## Decisions taken (with the date)

| date | decision | where it is written |
|---|---|---|
| 4 Nov 2026 | Measurements stay referenced to the original 1.01 server (the captures) | [../08-openjk.md](../08-openjk.md) |
| 4 Nov 2026 | Owner's goals: no loss of precision (values identical to today's or more precise, never less); no compatibility constraint; as few network settings as possible, computed at run time; netcode reusable by other JKA forks | [../README.md](../README.md), "Owner's goals" |
| 4 Nov 2026 | Branch defaults: `sv_fps` 20 (was 40), `sv_clientRate` 25000 (was 50000); `snaps` stays 40, `rate` 25000, `cl_maxpackets` 63; rate and snaps policies and the pilot fix are kept | `codemp/server/sv_init.cpp:960`, `:983`; [../08-openjk.md](../08-openjk.md) section 8.2 |
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
| 5 Nov 2026 | Step 2 before step 4, since no demo is available yet; field offsets computed by a small CMake program (`tools/fieldinfo`) built by the owner, not by parsing headers | [step-02-field-descriptors.md](step-02-field-descriptors.md) |

## Open questions for the owner


- Which game module the new game will use: the 1.01 base module (players sent as `TR_LINEAR_STOP`, like the captures) or one that sends players as `TR_INTERPOLATE` (some mods). The codec handles both; the recordings of step 4 should cover the one that will ship.
- Five transport questions of [../07-transport-study.md](../07-transport-study.md) section 7.7 (encryption, player count, IPv6, server browser, Apache-2.0 / OpenSSL licence position; platforms is answered). Not needed before step 9.

## Known issues found on the way (not fixed)

- The `user*` mod fields are networked 1 bit wide, floats included: only the lowest bit reaches the client unless a mod widens them with the overrides files ([../02-engine-code-findings.md](../02-engine-code-findings.md) section 2.2).

- With `sv_ratePolicy 2` and `sv_maxRate 0`, the client rate becomes 0 and `SV_RateMsec` divides by zero (`codemp/server/sv_client.cpp:1090`, `codemp/server/sv_snapshot.cpp:679`). Read only, not tested.

## State of the repository

- Nothing of this work is committed unless the owner did it. Check `git status` at the start of a session.
- Branch changes in the engine so far: the two defaults above in `codemp/server/sv_init.cpp`. Everything else is under `docs_rewrite/netcode/`.
- Python caches (`*.pkl`) are rebuilt by the tools and ignored by git. Delete `bdata.pkl` and `fullmsgs.pkl` after regenerating `data/fields.json`.
- `tools/fieldinfo/build/` is ignored by git; the owner builds `fieldinfo.exe` there.

## How to resume

1. Read this page, then the document of the last finished step, then the next step in [PLAN.md](../PLAN.md).
2. Check that the tools still give the recorded numbers: from `docs_rewrite/netcode/tools/python`, `python run_full.py` (study codec: snapshot mean 139.1, 0 failures) and `python run_pred.py pi --fuzz` (step 1: 107.3, 0 failures). About 2 minutes in all.
3. At the end of the session, write the step document, update the tables above, and record new decisions with their date.
