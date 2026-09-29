# GP7-02: Source, Build, and Package Audit

## Objective

Independently prove that the tested package is reproducible from the reviewed
source and contains no hidden helper, debug-only bypass, or missing dependency.

## Exclusive ownership

- `tests/splitscreen/gameplay/audit/build/**`
- Final read-only report `source-build-audit.md`

No production, test, or evidence changes are allowed.

## Audit

- Review the complete split-screen diff, ownership-lane changes, build scripts,
  package manifest, licenses, and configuration migration.
- Rebuild from a clean tree and compare binary/module/asset hashes or documented
  reproducible-build metadata.
- Search acceptance configs for prohibited direct profile/connect shortcuts and
  test-only behavior enabled in production.
- Verify one-player/upstream compatibility, warning policy, sanitizer reports,
  performance budgets, cleanup, and package-only cold runs.

## Acceptance

- The reviewed source reproducibly yields the certified package.
- No untracked dependency, helper bypass, debug escape, secret, or stale module
  can influence the result.
- Diff, build, test, and package audits contain no unresolved finding.

## Status

PLANNED.
