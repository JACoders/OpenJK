# R3-02 deterministic intermission oracle

Execution date: 2026-07-22 (Pacific/Honolulu)

## Delivered harness

- `intermission_4p.cfg` joins four active clients, captures a gameplay baseline,
  requests deterministic staging and a fraglimit-one scoring event, then captures
  the candidate intermission frame.
- `run.sh` uses the frozen x86_64 client/modules, an isolated `/private/tmp`
  homepath, UDP port 29532, and a hard 90-second watchdog.
- `assert_intermission.py` compares baseline and candidate images independently
  inside all four viewport centers. It requires both a substantial changed-pixel
  area and bright neutral scoreboard text in every viewport.

## Fresh rebuilt-module result

The deterministic trigger succeeded with the rebuilt x86_64 game module:

- `SplitNetStagePair: PASS attacker=2 victim=1 separation=72.0`
- P2 killed P1 and the log emitted `hit the kill limit`.
- `SplitNetStatAssert` passed with P2 score 1.

The original capture occurred about one second after the kill limit and exposed
snapshot convergence skew: P1 had consumed its intermission snapshot while the
three secondary sockets still rendered their preceding gameplay snapshots.
This was a harness timing defect, not a missing cgame intermission path.

The corrected case makes the kill deterministic (`giveother 0 health 1`), waits
180 frames for all four sockets, then explicitly asserts `PM_INTERMISSION` for
P1-P4 before capture. All four lifecycle assertions and the image oracle pass:

| Player | Changed ratio | Bright scoreboard-text ratio | Result |
| ---: | ---: | ---: | --- |
| P1 | 0.9224 | 0.1000 | pass |
| P2 | 0.9285 | 0.0923 | pass |
| P3 | 0.9311 | 0.1066 | pass |
| P4 | 0.9355 | 0.1080 | pass |

The final PNG was manually inspected. All four viewports show the complete
four-row scoreboard, and each cgame highlights its own player row. No overlay
crosses a horizontal or vertical seam. R3-02/R3-05 certification is **PASS**.
