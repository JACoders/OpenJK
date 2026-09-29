# GP1-04: Pause, Chat, Console, and Scoreboard Ownership

## Objective

Verify the behavior and clipping of modal/gameplay overlays for every local
player while other players are active.

## Exclusive ownership

- `tests/splitscreen/gameplay/modal_ownership/**`
- Evidence under `tests/splitscreen/gameplay/results/modal_ownership/**`

## Scenarios

- Route pause, scoreboard press/release, global/team chat, console, virtual
  keyboard, controls, and join/spectate UI from each assigned device.
- Enter distinct text/commands per player and verify recipient, team, and command
  context on the server.
- Open a modal while other players move/attack, die/respawn, or hold inputs.
- Repeat across 2/3/4 players, both layouts, intermission, spectate, and
  connection-error screens.

## Acceptance

- The intended global-versus-per-pane pause policy is explicit and asserted.
- Text, commands, scoreboard, and menu focus belong to the initiating player.
- Other panes remain visually clipped and behave exactly as the policy defines.
- Closing a modal restores the correct input state with no stuck command.

## Defect handling

Record immutable reproductions only; route fixes to GP4-01, GP4-02, or GP4-05.

## Status

DISCOVERY COMPLETE; ACCEPTANCE NOT MET.

The hash-stable 2/3/4-player evidence is indexed by
`tests/splitscreen/gameplay/results/modal_ownership/matrix.json`.

- Covered/pass: Player 2 console ownership in 2/3/4-player layouts.
- Deterministic findings: duplicated top-menu rendering, missing routed
  scoreboard, and missing routed chat.
- Intermittent finding: SDL bridge packets can arrive without reaching routed
  controller state.
- Remaining scenario gaps are explicitly `BLOCKED_UNSUPPORTED`; none are
  counted as passes.

Findings are routed to GP4-01, GP4-02, and GP4-05. No production source was
changed by this discovery ticket.
