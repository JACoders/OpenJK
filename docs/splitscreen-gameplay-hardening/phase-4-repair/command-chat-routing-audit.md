# Phase 4 command/chat routing audit

Read-only review of the current client changes; no build or runtime result is
claimed.

## Blockers

### P1 — Secondary chat submission is gated by Player 1 lifecycle

`Message_Key` checks global `cls.state == CA_ACTIVE` before calling
`CL_AddReliableCommandForPlayer` (`cl_keys.cpp:1243-1250`). At that point the
primary context is loaded, even when `Key_GetConsolePlayer()` is P2–P4.
Consequently an active secondary cannot submit chat while P1 is reconnecting,
and a non-active secondary can enter the routing function merely because P1 is
active. The active-state decision must be made against the owning player's
connection context (and remain coupled to that same owner through enqueue).

### P1 — Per-player cgame console dispatch clobbers global command tokens

`CL_CGameConsoleCommandForPlayer` calls `Cmd_TokenizeString` before checking
whether the requested secondary is enabled/cgame-started
(`cl_cgame.cpp:699-709`) and never restores the prior command token context.
This function is called from `CL_SendCmd` for score/weapon/inventory/Force
edges, so it can overwrite the command currently being executed even on its
early-failure path. Use the engine's nested-command save/restore mechanism (or
an equivalent scoped token context), and tokenize only after player validation.

## Lifecycle/safety concerns

- `CL_CGameConsoleCommandForPlayer` stores multi-megabyte primary snapshots in
  function-static objects (`cl_cgame.cpp:691-692`). This avoids stack pressure
  but makes the context swap non-reentrant. A nested console dispatch would
  overwrite the outer saved primary state before restoration. A guarded/scoped
  context owner or heap-backed local snapshot is safer.
- The secondary cgame path otherwise restores the important mutable globals:
  it writes back the secondary `cl`, `clc`, and `cls.state`, then restores the
  primary copies, render-player cvar, and selected cgame
  (`cl_cgame.cpp:711-726`). The early secondary failure path reselects P1, but
  should also explicitly restore the render-player cvar for symmetry.
- `CL_AddReliableCommandForPlayer` correctly selects P1 directly and uses
  `CL_SplitNetAddReliableCommand` for P2–P4. That helper loads the secondary
  `cl`/`clc`, enqueues on its reliable sequence, stores it back, and restores
  P1. Its connected-state check prevents queueing to disabled/disconnected
  secondaries. The chat caller's lifecycle check remains the blocker above.
- Console edit fields and history are isolated per owner through
  `Key_SaveConsolePlayerState`/`Key_LoadConsolePlayerState`. Switching an open
  console changes ownership without closing it, which is coherent. Chat text
  itself remains one global `chatField`; ownership is represented indirectly
  by `consolePlayer`. Any path that calls stock `messagemode` rather than
  `Con_MessageModeForPlayer` will therefore inherit the last console owner.
  Tests should cover P2 chat after P3 console use and cancellation/reconnect.

## Required regression coverage

1. P2 chat succeeds while P1 is not active, and P2 chat is rejected while only
   P2 is not active.
2. A failed P3 cgame command dispatch does not alter the surrounding command's
   `Cmd_Argc`/`Cmd_Argv` values.
3. Rapid P2/P3 score and weapon edges restore P1 `cl`, `clc`, `cls.state`,
   selected cgame, and render-player ownership after every dispatch.
4. P2/P3/P4 chat reaches only the matching reliable sequence through
   disconnect/rejoin and varied slot churn.
