# GP1-02: Mouse Confinement and Focus Ownership

## Objective

Prove that keyboard/mouse navigation and clicking cannot bleed into another
player's split or change another player's focus.

## Exclusive ownership

- `tests/splitscreen/gameplay/mouse_focus/**`
- Evidence under `tests/splitscreen/gameplay/results/mouse_focus/**`

## Scenarios

- Exercise every edge and corner in 2/3/4-player layouts, including repeated
  large deltas, diagonal motion, clicks on boundaries, and click-drag.
- Change horizontal/vertical layouts, window size, resolution, fullscreen,
  renderer restart, and player count while the pointer is at a boundary.
- Open setup, Force, saber, controls, pause, scoreboard, chat, console, and
  virtual keyboard surfaces while other players continue input.
- Attempt to click every adjacent-pane control from the mouse-owned pane.
- If reassignment is supported, repeat for the non-P1 owner; otherwise prove
  that invalid keyboard/mouse reassignment is rejected visibly.

## Acceptance

- Cursor coordinates and hit tests remain inside the owning pane.
- No cursor appears or moves in a non-owning pane.
- No adjacent control, hover, focus, bind capture, or gameplay command fires.
- Resize/restart/layout transitions cannot strand or duplicate pointer state.

## Defect handling

Record immutable reproductions only; route fixes to GP4-01 or GP4-02.

## Status

FINAL-HASH COVERED MATRIX PASSED; 15 CELLS REMAIN BLOCKED.

The repaired 2/3/4-player matrix at
`tests/splitscreen/gameplay/results/certification/final/mouse-focus-737f/`
records:

- 76 `COVERED_PASS`
- 0 `DISCOVERED_FAIL`
- 15 `BLOCKED`

All three Phase 0 manifests record `status: passed` against client
`737fcd8c3cfdbc78f220e7d2b63dac3ba44b49e7f8b71d82b7ccee4029277c13`.
The covered cells include every edge/corner, repeated extreme deltas,
boundary drag, horizontal/vertical layouts, setup/Force/controls/OSK/console/
scoreboard/chat surfaces, resize, renderer restart, live count change, and the
explicit reassignment policy. Adjacent profiles remained unchanged and no
covered assertion failed.

The remaining blocked cells are not passes:

- Per-pane mouse-only pause has no entry point and remains GP1-04-owned.
- Fullscreen display-mode mutation was not authorized.
- Visible invalid keyboard/mouse reassignment rejection still needs a
  fresh-menu journey.
- Exhaustive clicking of every adjacent control is not represented by the
  boundary hit-test set.
- The 2p/3p Saber surface lacks a stable mouse-only entry route.
- The 4p Force coordinate clamps onto Saber and needs a dedicated route.

Evidence:

- `tests/splitscreen/gameplay/results/certification/final/mouse-focus-737f/matrix.json`
- `tests/splitscreen/gameplay/results/certification/final/mouse-focus-737f/2p/20260723T232154Z-21247-23311/manifest.tsv`
- `tests/splitscreen/gameplay/results/certification/final/mouse-focus-737f/3p/20260723T232228Z-22169-25234/manifest.tsv`
- `tests/splitscreen/gameplay/results/certification/final/mouse-focus-737f/4p/20260723T232303Z-23134-6988/manifest.tsv`
