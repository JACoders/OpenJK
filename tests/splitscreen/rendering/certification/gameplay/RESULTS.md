# Split-screen gameplay acceptance results

Run `./run.sh` from this directory. The runner launches an isolated two-player
FFA match and fails if any engine assertion fails or the required input/lifecycle
markers are absent.

## Certified scenario

- Player 1: keyboard/mouse, `Kyle`, `kyle/default`, dark-side powers.
- Player 2: `controller1`, `Light_Sith`, `reborn/default`, light-side powers.
- P2's controller attack is observed in P2's generated usercmd and reduces
  Kyle's health below 100.
- P1's keyboard `L` event is observed in P1's generated usercmd with Force
  Lightning active.
- P2's light-side Absorb is allowed to deplete before the held Lightning event
  supplies the final damage.
- Final assertions require Kyle alive, P2 dead, and Kyle's score at least one.

The checked artifacts in `artifacts/` are from the passing 2026-07-22 run.
`acceptance_kyle_lightning.png` captures the live Lightning interaction and
`acceptance_kyle_lightning_kill.png` captures both local-player kill views.
