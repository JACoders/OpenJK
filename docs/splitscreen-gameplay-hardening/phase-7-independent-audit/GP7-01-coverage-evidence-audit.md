# GP7-01: Coverage and Evidence Audit

## Objective

Independently prove that every required player-count/mode/lifecycle/device/
environment cell has valid, visually credible evidence from the frozen build.

## Exclusive ownership

- `tests/splitscreen/gameplay/audit/coverage/**`
- Final read-only report `coverage-audit.md`

No production, test, or evidence changes are allowed.

## Audit

- Regenerate the coverage ledger solely from archived manifests and logs.
- Sample every lifecycle class and visually inspect all required transition,
  modal, objective, error, intermission, and terminal-state contact sheets.
- Verify timestamps, hashes, slot/player identities, device ownership, endpoint
  class, and artifact provenance.
- Search for blank/fade, duplicate/stale panes, assertion failures, warnings that
  invalidate the scenario, missing files, and smoke mislabeled as end-to-end.

## Acceptance

- The regenerated ledger is complete and matches the committed ledger.
- Every claimed pass is independently traceable to authoritative state and
  visual proof.
- No reproducible defect or unreviewed anomaly remains.

## Status

PLANNED.
