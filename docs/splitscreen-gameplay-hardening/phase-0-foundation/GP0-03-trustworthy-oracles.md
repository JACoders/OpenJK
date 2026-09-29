# GP0-03: Trustworthy State, Media, and Resource Oracles

## Objective

Prevent false passes caused by blank/fade screenshots, duplicated panes, stale
snapshots, unverified sound-off runs, or missing resource data.

## Exclusive ownership

- `tests/splitscreen/gameplay/oracles/**`
- `tests/splitscreen/gameplay/fixtures/**`

No production source changes are allowed.

## Work

- Add assertions for unique slots/qports/snapshots, lifecycle, score, team,
  objective state, userinfo, reliable commands, and clean disconnect.
- Extend image checks for pane uniqueness, blank/fade frames, wrong-menu
  substitution, viewport boundaries, HUD ownership, and cursor confinement.
- Define deterministic per-player input and audio event traces.
- Capture RSS, file descriptors, sockets, threads, frame-time distributions,
  renderer/VM restarts, and crash/sanitizer markers.
- Make every oracle consume a versioned manifest rather than infer success from
  filename existence.

## Acceptance

- Known-good 2/3/4-player proof fixtures pass.
- Deliberately duplicated, blank, stale, cross-pane, wrong-client, silent,
  leaked-socket, and truncated-log fixtures each fail for the expected reason.
- Oracle output identifies the player, phase, expected state, and actual state.

## Evidence

Archive self-test reports and the minimal invalid fixtures.

## Status

COMPLETE (2026-07-23). The version-1 manifest oracle and archived self-test report live in
`tests/splitscreen/gameplay/oracles/`; compact known-good and deliberately
invalid fixtures live in `tests/splitscreen/gameplay/fixtures/`. The 2026-07-23
self-test passed all three positive player-count cases, all eight required
negative mutations, and a tampered-hash case. The runtime path reads and hashes
real on-disk state, image-analysis, audio, resource, and optional raw evidence;
synthetic materialization is restricted to self-test fixtures.
