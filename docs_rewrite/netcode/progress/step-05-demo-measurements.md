# Step 5: measurements on the demos

Status: **done**, 5 November 2026. Plan: [PLAN.md](../PLAN.md), step 5.

## What was measured

The step 1 codec (whole-message codec `codec2.py` with the prediction modes and the rANS initial-state trick, integer position prediction `pi`) was trained on the 60 training demos and measured on all 80 selected demos of step 4 ([step-04-demo-corpus.md](step-04-demo-corpus.md)). Sizes are real bytes of the rANS output, compared with the engine size of the same message (`nbytes`, the message without `svc_EOF`). The first message of each demo (the gamestate rebuilt by the recording client) is left out.

- Per-field modes (absolute or delta) and prediction modes are chosen on the training demos, as `run_pred.py` does on the capture: 85 of 196 fields are delta-coded, 24 of the 64 prediction candidates are kept (positions `pi`, time fields `t`, several angles and velocities `a2`).
- Three table sets were compared: one global set; one set per game type; one global set with the base age (0 = no base, 1, 2, 3, 4 and more) added to every context after the delta.
- `sim_loss.py` was run on the demos: the base age is set by the simulation, so all ages are measured on the same content.

## Tools

- `run_step5.py` (new): the measurement above, in parallel over the demos (about 15 minutes with 30 processes). Prints the per-field choices, the round-trip failures and, per table set, sizes by set, game type, snapshot interval, game type and interval, base age, held-out map and server build.
- `sim_loss_demos.py` (new): the loss and latency grid of `sim_loss.py` on the 20 held-out demos, tables trained on the 60 training demos under the three loss / age mixes of `sim_loss.py` (about 20 minutes).
- `bench2.py`: `choose_modes` is split into `mode_stats` (per demo), `merge_mode_stats` and `pick_modes`, so that the choice can be made over many demos; the result on the capture is unchanged.
- `run_full.py`: `train()` (training on the capture, used by `run_demos.py`) and a `rec` argument to `decode_check` (per-message sizes).
- Checks after these changes: `run_full.py` 139.1, `run_pred.py pi --fuzz` 107.3 and fuzz 0 failures, as before.

## Results with one global table set

Round trip: 0 failures on 80 demos for each of the three table sets.

Snapshot-only messages (no server command):

| set | messages | mean | p95 | max | engine mean | ratio to engine |
|---|---|---|---|---|---|---|
| held-out | 69,448 | 69.0 | 162 | 556 | 155.1 | 0.445 |
| training | 203,639 | 73.4 | 175 | 1,220 | 162.9 | 0.451 |

All messages except the gamestate: ratio 0.483 on the held-out set, 0.494 on the training set.

By game type:

| game type | held-out mean / p95 / max | held-out ratio | training mean / p95 / max | training ratio |
|---|---|---|---|---|
| CTF | 93.5 / 214 / 556 | 0.486 | 121.5 / 255 / 1,220 | 0.485 |
| duel | 37.6 / 58 / 201 | 0.460 | 31.7 / 49 / 248 | 0.433 |
| siege | 57.2 / 111 / 375 | 0.464 | 65.3 / 136 / 955 | 0.458 |
| team FFA | 63.7 / 103 / 452 | 0.404 | 58.8 / 103 / 564 | 0.414 |

Held-out maps:

| map | messages | mean | p95 | max | ratio to engine |
|---|---|---|---|---|---|
| `mp/ctf_gerona` | 4,972 | 28.9 | 54 | 193 | 0.433 |
| `mp/ctf_paulking` | 4,459 | 112.9 | 228 | 556 | 0.488 |
| `mp/ctf_salliche` | 2,133 | 155.5 | 272 | 458 | 0.510 |
| `mp/ctf_talay` | 8,591 | 105.4 | 213 | 500 | 0.487 |
| `mp/duel4` | 5,345 | 37.6 | 58 | 201 | 0.460 |
| `mp/ffa1` | 4,884 | 75.0 | 126 | 272 | 0.410 |
| `mp/ffa_bonus3` | 15,251 | 62.6 | 95 | 446 | 0.403 |
| `mp/ffa_kepler` | 10,083 | 59.7 | 102 | 452 | 0.400 |
| `siege_narshaddaa` | 13,730 | 57.2 | 111 | 375 | 0.464 |

**Held-out against training.** Absolute sizes depend on the content of each map, so the gap is measured on the ratio to the engine. Per game type, the held-out ratio exceeds the training ratio by at most 6 % (duel, 0.460 against 0.433); over all held-out snapshots it is lower (0.445 against 0.451). The 15 % limit of the plan is respected: the training data does not need to be widened.

**Against the study.** On the study capture (one map, bots) the same codec gives 107.3 bytes against 337.6 for the engine, a ratio of 0.318. The demos give 0.445: their snapshots are half the size (155 bytes against 338), so the fixed part of each message (header, acknowledgement, entity list framing) weighs more. The study's targets (PLAN section 6) apply to its content only; on the demos the codec divides the engine size by about 2.2 for snapshots and 2.1 for all messages.

## Table set: decision

| table set | held-out snapshot mean | held-out ratio, all messages | contexts |
|---|---|---|---|
| global | 69.0 | 0.483 | 1,126 |
| one per game type | 67.4 (−2.3 %) | 0.475 | 4 sets of 453 to 1,052 |
| global, base age as context | 67.5 (−2.2 %) | 0.476 | 4,328 |

Per game type, most of the gain is in team FFA (−5.5 %), and one held-out map loses (`mp/ctf_gerona`, 28.9 to 33.0 bytes, +14 %). The age context helps mostly the messages without a base (11 held-out messages, 272.7 to 237.5 bytes). The two variants combined were not tested.

**Decided with the owner (5 November 2026): one global table set, without the base age as a context.** The gains of about 2 % are smaller than the differences between maps, and each variant multiplies the size of the tables shipped with the binary (04 section 4.5).

## Snapshot interval (`sv_fps`)

The codec makes no assumption about the snapshot interval: time fields are coded against the snapshot's `serverTime`, whatever the gap. The interval changes only the content (more movement between two snapshots). The goal (owner, 5 November 2026) is one table set that works at any `sv_fps`.

| game type, interval | set | messages | mean | ratio to engine |
|---|---|---|---|---|
| CTF 33 ms | held-out / training | 20,155 / 47,937 | 93.5 / 115.1 | 0.486 / 0.482 |
| CTF 66 ms | training | 1,194 | 348.7 | 0.531 |
| CTF 99 ms | training | 213 | 284.1 | 0.465 |
| duel 50 ms | held-out / training | 5,345 / 10,733 | 37.6 / 28.1 | 0.460 / 0.418 |
| duel 66 ms | training | 5,378 | 38.8 | 0.458 |
| siege 33 ms | held-out / training | 13,730 / 48,685 | 57.2 / 65.3 | 0.464 / 0.458 |
| team FFA 33 ms | training | 5,011 | 53.8 | 0.436 |
| team FFA 50 ms | held-out / training | 30,218 / 84,488 | 63.7 / 59.1 | 0.404 / 0.413 |

The one global table set decodes every interval of the corpus (33, 50, 66, 99 ms) with 0 failures, with ratios between 0.40 and 0.53. The interval this branch uses by default (50 ms, `sv_fps` 20) is well covered (130,000 snapshots). The effect of the interval alone cannot be isolated: no server of the corpus appears at two intervals, and 66 and 99 ms come from one or two demos. Measuring it needs recordings of the same game at several `sv_fps` on this branch (state recorder, step 3, or bot demos).

## Base age

Real ages in the held-out demos: age 1 for 55 % of the snapshots, 2 for 17 %, 3 for 9 %, 4 for 18 %, others rare (0, no base: 11 messages). The ratio to the engine grows with the age (0.413 at age 1, 0.492 at age 4), but the real ages are tied to the servers (most age-4 messages come from the CTF server at 33 ms).

`sim_loss_demos.py` sets the age on the same content (held-out demos, snapshot messages with or without server commands, real bytes, 0 round-trip failures in the 20 runs):

| loss | ack age 1 | age 2 | age 4 | age 8 |
|---|---|---|---|---|
| 0 % | 90.7 | 104.3 | 125.8 | 160.3 |
| 2 % | 91.0 | 104.5 | 126.0 | 160.5 |
| 5 % | 91.4 | 104.9 | 126.4 | 160.6 |
| 10 % | 92.2 | 105.6 | 126.9 | 160.9 |
| 20 % | 94.3 | 107.3 | 128.3 | 161.7 |

p95 245 to 383. Maximum 4,654 to 4,910 (the simulation counts every message with a snapshot, including those with server commands; these messages were not examined). Without loss, by game type (age 1 / 2 / 4 / 8): CTF 126.0 / 140.4 / 164.5 / 205.3, duel 29.6 / 36.7 / 46.2 / 60.6, siege 77.4 / 90.5 / 109.5 / 141.1, team FFA 64.8 / 79.0 / 101.1 / 133.1.

From age 1 to age 4 the size grows by 39 % (study capture with the study codec: +54 %, 152.2 to 235.0, 03 section 3.9); to age 8 by 77 % (study: +112 %). Loss adds at most 4 % at 20 %. As in the study, latency, not loss, drives the size: keeping the base age low (04 section 4.7) matters more than the table design.

## Dropped from the plan

- The ideal-byte benchmarks `bench2.py` and `bench5.py` were not re-run on the demos: the real codec, measured above, replaces them.
- The check of the `TR_INTERPOLATE` elapsed-time rule: the target game module is the base one, which sends players as `TR_LINEAR_STOP` (decision of 5 November 2026, [../08-openjk.md](../08-openjk.md) section 8.3).

## Check

| check | result |
|---|---|
| Mean / p95 / max on the held-out set per map and game type | documented above |
| Held-out sizes at most 15 % above training sizes | yes (ratio to engine, at most +6 % per game type) |
| Round trip | 0 failures: 80 demos × 3 table sets, and 20 held-out demos × 20 loss / age runs |
| Decisions of 04 section 4.5 | one global table set, no age context |

## What step 6 needs from this one

- The table design: one global set, trained on the 60 training demos, per-field modes and prediction modes as chosen by `run_step5.py`.
- The size regression test of PLAN section 4 uses the 20 held-out demos; reference: 69.0 bytes per snapshot-only message, ratio 0.483 to the engine on all messages.
