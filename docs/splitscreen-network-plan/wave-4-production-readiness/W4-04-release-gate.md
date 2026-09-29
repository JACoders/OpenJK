# W4-04: Release Candidate Gate

## Objective

Make the final release decision from reproducible evidence rather than informal
claims of completeness.

## Required gates

1. Clean ARM64 build and package from a fresh checkout.
2. Existing OpenJK unit/smoke tests and complete split-screen suite pass.
3. Wave 3 matrices contain no unresolved critical or high-severity failures.
4. Four local players plus one vanilla remote client complete a hosted match.
5. Four local players complete a multi-map session on a vanilla server.
6. Controller and keyboard isolation traces pass for every player count.
7. Visual screenshots are inspected at supported layouts and resolutions.
8. Single-player and ordinary one-player multiplayer regression tests pass.
9. Security review finds no credential logging or unsafe userinfo construction.
10. Documentation and known limitations match observed behavior.

## Defect policy

- Critical/high defects block release.
- Medium defects require an owner, regression test plan, and documented impact.
- Low visual defects may defer only when they do not obscure controls or state.
- A flaky test is a failing test until its nondeterminism is understood.

## Deliverables

- Signed-off checklist with commit, bundle checksum, test manifests, compatibility
  table, screenshots, known issues, and rollback instructions.
- Release tag only after every mandatory gate is evidenced.
