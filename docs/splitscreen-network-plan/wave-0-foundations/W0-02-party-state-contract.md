# W0-02: Define Party Lifecycle and State Contract

## Objective

Define one explicit state machine for a split-screen party before it enters a
local host or internet join flow.

## Ownership

- New party-state module or narrowly scoped client state declarations.
- Developer diagnostics and documentation.
- Do not edit menu assets or implement host/join transitions.

## Required states

`disabled`, `configuring`, `host_pending`, `join_pending`, `connecting`,
`active`, `map_transition`, `disconnecting`, and `failed`.

## Work

1. Define authoritative player count, device owner, profile, connection state,
   target address, and pending operation fields.
2. Define legal transitions and cancellation behavior.
3. Specify which state survives menu closure, `vid_restart`, map changes, and
   server reconnects.
4. Add read-only status output suitable for automated assertions.
5. Document ownership of legacy CVARs and migration rules.

## Acceptance criteria

- Illegal transitions fail safely and emit a useful diagnostic.
- Status output reports all four local slots without mutating state.
- No user-visible behavior changes in this ticket.
- Existing local split-screen QA still passes.

## Verification

- Unit or command-level tests cover every legal transition and representative
  illegal transitions.
- Run under ASan/UBSan where supported and verify clean shutdown.
