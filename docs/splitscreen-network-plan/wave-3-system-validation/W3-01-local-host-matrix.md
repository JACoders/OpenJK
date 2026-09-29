# W3-01: Local Hosting Matrix

## Objective

Prove the complete stock Create Server to local/remote gameplay workflow.

## Matrix

- Local players: 2, 3, 4.
- Game types: FFA, Holocron, Jedi Master, Duel, Power Duel, Team, Siege, CTF,
  and CTY where maps permit.
- Bots: none, partial fill, capacity boundary.
- Remote clients: zero, one vanilla client, multiple up to capacity.
- Transitions: death/respawn, team/spectate/join, restart, next map, disconnect.

## Acceptance criteria

- Stock settings match serverinfo and gameplay.
- Every viewport retains its profile, HUD, input, menu ownership, and score.
- A fifth remote player can join a four-local-player listen server.
- No crash, hang, ghost client, slot leak, or input bleed occurs.

## Deliverables

- Machine-readable matrix report with exact pass/fail cells.
- Logs and screenshots for every failure and representative successes.
- Focused regression test for every fixed defect.
