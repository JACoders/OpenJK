# GP1-03: Controller Hotplug, Remap, and Concurrent Input

## Objective

Prove stable ownership when controllers disconnect, reconnect, reorder, remap,
or hold input concurrently.

## Exclusive ownership

- `tests/splitscreen/gameplay/controller_lifecycle/**`
- Evidence under `tests/splitscreen/gameplay/results/controller_lifecycle/**`

## Scenarios

- Disconnect each assigned controller during player setup, gameplay, pause, and
  a held axis/button; verify only its player becomes inert or unassigned.
- Reconnect the same device and a different device in changed enumeration order.
- Remap attack, Force, jump/use, weapon, menu accept/back, and verify persistence.
- Hold keyboard, mouse, and all controllers simultaneously; open/close menus and
  release inputs in multiple orders.
- Exercise duplicate-device assignment, missing-device launch, `in_restart`,
  process relaunch, and recovery from an interrupted bind capture.

## Acceptance

- No automatic reassignment or input bleed occurs.
- Held inputs are neutralized on removal, focus change, and menu transition.
- Reconnection requires and honors an explicit stable ownership decision.
- All player command streams recover without stuck buttons or axes.

## Defect handling

Record immutable reproductions only; route fixes to GP4-02.

## Status

DISCOVERY COMPLETE; ACCEPTANCE NOT MET. Two externally driven SDL-bridge runs
produced 2 covered/passing cells, 4 deterministic failing cells, and 13
explicitly unsupported physical-hotplug cells. Immutable logs, hashes, and the
cell matrix are under
`tests/splitscreen/gameplay/results/controller_lifecycle/`. Production fixes
are deferred to GP4-02. The refreshed evidence was recorded against frozen
binary/module hashes and passes strict Phase 0 `openjk-e2e-v1` manifest
validation; that certification covers artifact integrity, not gameplay
acceptance.
