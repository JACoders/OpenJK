# R2-02 frozen-build lifecycle results

Execution date: 2026-07-22 (Pacific/Honolulu)

Binary under test:
`build-x86_64/openjk.x86_64.app/Contents/MacOS/openjk.x86_64`, with the
matching frozen `cgame{x86_64,2x86_64,3x86_64,4x86_64}`, UI, and game modules.
Runs used `/private/tmp/openjk-r2-02-lifecycle` and UDP port 29322.

## Observed matrix

| Transition | Viewports exercised | Result | Evidence |
| --- | --- | --- | --- |
| initial join and spawn | P1-P4 | pass | all four report `ALIVE` in each run |
| death | P1-P4 | pass | `respawn_flow.stdout.txt`: all four report `DEAD` |
| respawn | P1-P4 | pass | each returns to `ALIVE` from a controller/keyboard attack edge |
| held attack across death | P2-P4 | pass | remains `DEAD` until release followed by a new attack edge |
| spectate | P2, P3, P4 independently | pass | each reports `SPECTATOR` while the other players retain their expected states |
| rejoin from spectate | P2, P3, P4 independently | pass | each returns to `ALIVE` |
| team change | P1/P3 red, P2/P4 blue | pass | team assertions pass before and after restart |
| map restart | P1-P4 | pass | all four remain `ALIVE` with the same teams after `map_restart 0` |
| intermission | none | **not exercised** | no deterministic intermission command/oracle exists in the frozen harness |

The four captured final-state images pass the existing four-viewport content
oracle (non-blank viewports and pairwise visual differences).  That oracle does
not identify semantic overlay ownership, so it is not evidence that every
overlay pixel is correct.  No claim of full R2-02 certification is made because
intermission was not driven.

## Reproduction

```sh
tests/splitscreen/rendering/certification/lifecycle/run_frozen_matrix.sh
python3 -m unittest discover \
  -s tests/splitscreen/rendering/certification/lifecycle \
  -p 'test_*.py' -v
```

The runner exits nonzero if any lifecycle assertion fails. Controller input is
explicitly enabled so simulated secondary-player button edges enter usercmd
generation.
