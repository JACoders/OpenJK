# Step 1: prediction modes and the rANS initial-state trick in the Python codec

Status: **done**, 5 November 2026. Plan: [PLAN.md](../PLAN.md), step 1. Detailed tables: [../03-results.md](../03-results.md) sections 3.12 (ideal bytes) and 3.13 (real codec).

## Goal and check

Confirm with the real encoder and decoder the gains that `bench5.py` measured in ideal bytes, without any loss of precision. Check: 0 round-trip failures on the real messages, on the synthetic corpus and under simulated loss; mean real bytes per snapshot at most 110 on the test part of the 5-minute capture.

Both parts of the check are met.

## What was done

- `bench5.py` (new): ideal-byte measurement of four variants on top of the study's schema: T (time fields against the message's `serverTime`), P1 / P2 (positions predicted from the old velocity / from the mean of the old and new velocity), A2 (angle index predicted to second order). Result: 136.9 to 106.1 ideal bytes with T, P2 and A2 together.
- `codec.py` (changed; every new behaviour is off by default, so the earlier scripts give the study's numbers, re-checked):
  - `X0`: initial state of the rANS encoder. `1` enables the initial-state trick; the decoder stops reading at the end of the buffer.
  - `PMODES`: map from a prefixed field name (`e:` entity, `p:` player state) to a prediction mode: `t`, `pf` (float arithmetic), `pi` (integer arithmetic) or `a2`.
  - `CTX`: prediction context filled by the callers (message `serverTime`, base `serverTime`, state being built, base state, base's own base).
  - `enable_prediction()`: moves `pos.trType` and `pos.trDelta[0..2]` before `pos.trBase`, and `velocity` before `origin`, so that the decoder knows every input of a prediction before the value it predicts.
- `codec2.py` (changed): passes the context; each header record keeps the snapshot it was coded against (`Ref`), so that the next message can find the base's own base on both sides; `enable_prediction()` for the player-state tables.
- `run_pred.py` (new): per-field choice of the modes on the training part, round trip of all real messages, `--fuzz` (seeds 1 to 5), `--loss` (the loss and latency grid of `sim_loss.py`); `pf` or `pi` selects the arithmetic.

## Results

| configuration | snapshot bytes, 5min (p95, max) | bots | solo |
|---|---|---|---|
| engine today | 337.6 | 255.2 | 17.1 |
| study codec | 139.1 (240, 744) | 106.5 | 9.3 |
| + rANS initial-state trick | 137.3 (238, 753) | 105.1 | 8.1 |
| + prediction, float arithmetic | 106.8 (191, 650) | 82.3 | 8.0 |
| **+ prediction, integer arithmetic (kept)** | **107.3** (191, 650) | **82.7** | **8.0** |

- 0 round-trip failures: 8,842 real messages (every configuration), 18,150 synthetic messages (both arithmetics), 20 runs of the loss grid.
- With loss and latency (float arithmetic): 152.2 to 118.3 bytes at RTT 50 ms and 0 % loss; 322.4 to 291.5 at 400 ms. The saving is about 31 to 34 bytes whatever the base age.
- Server commands + snapshot: 210.9 to 178.4. Gamestate: 4,024 to 4,014. `svc_mapchange`: 9 to 6.
- Kept by the per-field choice (11 of 64 candidates): `pos.trTime` (T); `pos.trBase[0..2]` and `origin[0..2]` (position prediction); `apos.trBase[0..1]` and `viewangles[0..1]` (A2). `commandTime` against `serverTime` was not kept.

## Findings

1. **Precision is unchanged, bit for bit.** The prediction is only the codec's guess; the exact difference between the value and the guess is sent, so the client rebuilds the same float32 the engine sends today. The game still computes in float as before.
2. **Integer arithmetic for the guess.** Client and server must compute the same guess; float arithmetic can differ between compilers (FMA, x87), integer arithmetic cannot. Cost: 0.5 byte per snapshot. The exact definition is in 03 section 3.13; the C++ code must implement exactly that.
3. **Two symmetry rules found by the fuzz test** (first run: a desynchronisation on vehicle states):
   - the vehicle state of an older snapshot exists only if that snapshot's player state had a vehicle (the engine's own rule for the base);
   - "the base state exists" is decided on its values (missing and zero fields are the same state), not on which keys a structure holds. The server keeps explicit zeros, the client does not.
   General rule for the C++ implementation: every input of a prediction must be taken from what the client also holds, in the same canonical form.
4. **Second-order angles need the base's own base.** Both sides keep, with each snapshot, the snapshot it was decoded against.
5. **The trick of the rANS initial state** recovers 1.5 bytes per snapshot (3 bytes on a 9-byte message).
6. **Game module dependency** (settled on 5 November 2026: the target is the base game module, players as `TR_LINEAR_STOP`, 08 section 8.3). The captures come from the 1.01 base module, where players are `TR_LINEAR_STOP` with `trTime` = `commandTime`; the position prediction uses that `trTime`. A game module that sends players as `TR_INTERPOLATE` (without `trTime`) is handled too: the codec uses the `serverTime` difference of the message and its base. That rule is implemented and covered by the synthetic corpus, but no real recording exercises it.
7. **Latency.** The codec does not change the round-trip time. It reduces rate delays and fragmentation (which matter at low `rate`, not at this branch's 25000). The latency gains are expected from steps 9 (acknowledgements) and 10 (interpolation delay computed instead of `cl_timeNudge`).

## Limits

- One map (`mp/ffa3`), one human, bots, 20 snapshots per second. Step 4 provides the data to re-measure; step 5 re-runs this.
- The fuzz test used uniform tables only for the prediction run (the earlier study also ran trained tables); the real messages and the loss grid used trained tables.
- Speed of the prediction was not measured (Python); it adds a few integer operations per predicted field.

## Reproduce

From `docs_rewrite/netcode/tools/python`:

```
python bench5.py                      # ideal bytes, variants base T P1 P2 A2 T+P2+A2   [110 s]
python run_pred.py pi --fuzz          # real codec, integer arithmetic, fuzz seeds 1 to 5 [90 s]
python run_pred.py pf --fuzz --loss   # float arithmetic, plus the loss grid           [4 min]
python run_full.py                    # study codec unchanged: snapshot mean 139.1       [15 s]
```

## What the next steps need from this one

- Step 5 must re-run `run_pred.py` on the demos of step 4 and check the `TR_INTERPOLATE` rule on demos whose game module sends players that way.
- Step 6 (C++) must implement: the field order of `enable_prediction()`, the integer prediction of 03 section 3.13, the A2 rule with the base's own base, the two symmetry rules above, and the rANS initial state of 1; and produce byte-identical output to `run_pred.py pi` with the same tables.
