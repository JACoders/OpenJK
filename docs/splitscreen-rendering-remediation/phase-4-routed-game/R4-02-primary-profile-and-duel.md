# R4-02: Primary Profile Application and Duel Completion

## Ownership

Primary-client profile validation in `codemp/game/g_cmds.c` and the live
profile/gameplay assertions used by the routed acceptance scenario.

## Work

Allow the Player 1 UI Apply action to reach the primary client with the same
validation used for secondary clients, then assert the exact model and Force
strings after the local server starts.

## Acceptance

- Player 1 persists `kyle/default` and dark preset
  `7-2-012320333000030321` (Lightning rank 3).
- Player 2 persists `desann/default` and light preset
  `7-1-332300000330003230`.
- A Controller 1 attack damages Player 1.
- Player 1's keyboard Lightning kills Player 2 and awards score 1.

## Status

Implemented and certified on July 22, 2026.
