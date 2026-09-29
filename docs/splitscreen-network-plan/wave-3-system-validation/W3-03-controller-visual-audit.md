# W3-03: Controller and Visual Workflow Audit

## Objective

Verify every host and join workflow is usable with assigned devices and rendered
correctly in 2-, 3-, and 4-player layouts.

## Work

1. Drive party setup, stock Create Server, stock browser, profile, saber, Force,
   controls, top menu, console, and virtual keyboards through real input paths.
2. Verify each physical/virtual gamepad maps to exactly one viewport.
3. Verify the single keyboard/mouse owner cannot affect another viewport.
4. Inspect text clipping, cursor/highlight ownership, HUD, overlays, modal prompts,
   keyboard centering, and viewport boundaries.
5. Test controller disconnect/reconnect and reassignment before connection.

## Acceptance criteria

- Every expected action is reachable without hidden developer commands.
- No input changes multiple players or produces navigation-only sounds.
- Stock menus are recognizably stock and correctly clipped per viewport.
- Every screenshot is visually inspected, not merely captured.

## Deliverables

- Screenshot set under a no-space path, grouped by player count and workflow.
- Input event/owner trace paired with each automated navigation scenario.
- Regression scenarios for all discovered bleed or layout defects.
