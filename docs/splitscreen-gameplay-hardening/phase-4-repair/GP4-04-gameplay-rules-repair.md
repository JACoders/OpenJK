# GP4-04: Authoritative Gameplay-Rule Repairs

## Objective

Fix defects in scoring, objectives, teams, roles, death/respawn, rounds,
intermission, restart, and mode-specific server behavior.

## Exclusive ownership

- `codemp/game/**`
- `tests/splitscreen/gameplay/regressions/rules/**`

This ticket does not edit client, UI, cgame, renderer, or audio source.

## Work

- Add minimal authoritative server-state regressions for assigned defects.
- Repair only split-triggered rule failures; preserve upstream one-client rules.
- Cover FFA, pickup modes, Duel/Power Duel, Team FFA, CTF/CTY, and Siege ledger
  items including low-population/queue behavior.

## Acceptance

- Every assigned reproduction fails before and passes after the patch.
- Scores, objectives, roles, teams, queues, death, respawn, intermission, and
  reset behavior match the same server's independent-client control.
- One-player and remote-client behavior does not regress.

## Evidence

Attach authoritative state dumps, event timelines, scoreboard captures, and
control comparisons.

## Status

IMPLEMENTED (SOURCE-LEVEL); BUILD/RUNTIME CONFIRMATION PENDING.

## Authoritative repair

`ClientConnect`'s same-IP admission count included every slot whose session IP
matched, including disconnected stale slots and the target reconnect slot. In
the four-player controlled recovery case, P4 disconnected and then its retained
session IP was counted alongside healthy P1-P3, producing `Too many connections
from the same IP`.

The count now includes only other clients whose persistent connection state is
not `CON_DISCONNECTED`. This preserves the existing `g_maxConnPerIP` boundary
and admission behavior for genuinely connected independent clients while
allowing an abandoned same-IP party slot to rejoin.

Focused source/behavior regressions are under
`tests/splitscreen/gameplay/regressions/rules/`. Six non-build tests pass,
covering stale-slot exclusion, connected-peer enforcement, the exact source
guard, and the evidence-based reclassification decisions below.

## Reclassified Phase 2 findings

- Team FFA enemy combat used an incorrect input-button expectation and never
  established an authoritative kill. Its respawn/intermission failures lack
  the required death/score prerequisite; no scoring or respawn rule was
  changed.
- Team FFA's immediate switch back hit the stock team-change cooldown. This is
  expected one-client behavior, not a split-only team rule.
- CTF/CTY P3/P4 team requests reached the game as commands such as
  `3 team red`, rather than `team red`. The server retained unique client
  numbers but correctly did not execute an unknown command as `SetTeam`.
  Routing belongs to GP4-03.
- Siege use used the wrong command-button expectation. Its death/class probes
  likewise did not prove an ordered death or authoritative class/inventory
  transition. No Siege objective, wave, or class rule was changed.
- Four-player Duel proved only a population of two active clients and two
  spectators. Connection order is not a defined tournament-queue oracle, and
  the voluntary spectator command was not backed by an ordered transition.
  No Duel selection rule was changed.
- The completed 2-player CTF/CTY objective, score, intermission, and restart
  assertions provide positive rule controls; no objective-rule repair was
  indicated.

The pre-existing `g_cmds.c` profile/assert-userinfo changes were preserved
unchanged. No rebuild was performed for this ticket as requested.
