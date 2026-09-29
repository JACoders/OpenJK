# GP8-01: Seal the Split-Screen Release Candidate

## Objective

Issue the final release decision only when the complete end-to-end gameplay
hardening contract is satisfied.

## Exclusive ownership

- `tests/splitscreen/gameplay/release/**`
- `docs/splitscreen-gameplay-hardening/generated/final-report.md`

No production or certification artifact changes are allowed.

## Seal checks

- Confirm all phase entry/exit gates and both independent audits.
- Confirm zero open reproducible split-screen defects at every severity.
- Confirm every fixed defect has a failing-before/passing-after regression and
  appears in the replayed frozen-build results.
- Confirm 1/2/3/4-player, every stock mode, UI/device, local/public/pure,
  lifecycle/recovery, audio, soak, performance, sanitizer, and package coverage.
- Publish exact source, binary, module, asset, package, ledger, and report hashes.
- Publish one command that validates the immutable evidence without launching or
  contacting an external server.

## Acceptance

The ticket may conclude only `SEALED` or `NOT SEALED`. Missing evidence, a
waived reproduction, hash mismatch, unexplained warning, or open defect forces
`NOT SEALED` and reopens the responsible earlier phase.

## Status

PLANNED.
