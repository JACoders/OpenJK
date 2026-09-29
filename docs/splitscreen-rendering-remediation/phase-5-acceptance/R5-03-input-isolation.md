# R5-03: Keyboard/Mouse Isolation

## Ownership

`tests/splitscreen/cfg/input_isolation_2p.cfg` and
`tests/splitscreen/rendering/certification/input-isolation-2p/`.

## Acceptance

- Controller 1 changes only Player 2 in UI and gameplay.
- Keyboard/mouse changes only Player 1 in UI and gameplay.
- Extreme mouse motion clamps the entire 40x40 cursor to Player 1's pane at
  `(600, 200)` and leaves Player 2 unchanged.

## Status

PASS: `/private/tmp/openjk-input-isolation-current-final`.
