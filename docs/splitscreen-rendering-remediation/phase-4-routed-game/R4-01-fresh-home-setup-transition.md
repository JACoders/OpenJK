# R4-01: Fresh-Home Setup Transition

## Ownership

`assets/splitscreen/base/ui/jamp/splitscreen_players.menu`, transient split UI
cvar registration in `codemp/client/cl_input.cpp`, and the setup transition in
`codemp/ui/ui_main.c`.

## Work

Make Next atomically establish setup state, wait for its activating control to
be released, and enable split rendering outside the active menu-paint call.
Resume input only after a settled setup frame.

## Acceptance

- Main menu -> Play -> Split Screen -> 2 -> Next works from a clean home.
- The transition does not require `splitscreen_setup` or a test-authored
  `cl_splitScreen` assignment.
- The first settled frame contains two clipped, interactive player panes.
- Player 1 owns keyboard/mouse and Player 2 owns Controller 1.

## Status

Implemented and certified on July 22, 2026.
