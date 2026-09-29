# R1-01: Renderer Viewport Isolation

## Ownership

`codemp/client/cl_cgame.cpp`, `codemp/cgame/cg_view.c`,
`codemp/rd-vanilla/*`, and scene regression tests.

## Work

Make viewport/scissor/clear explicit for every cgame submission, preserve the
physical frame across players, and restore full-screen state after the last one.

## Acceptance

- Each scene touches only its rectangle in 2/3/4-player layouts.
- Clear operations cannot erase or retain another player's pixels.
- Single-player output is unchanged.
