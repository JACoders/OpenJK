# P7-04: Certify Compatibility and Provenance

## Objective

Independently certify source provenance, licenses, vanilla compatibility, and
regression evidence for the frozen candidate.

## Primary ownership

`artifacts/certification/audit`, isolated vanilla servers/clients, and audit
report only. Do not modify source, packages, docs, or other reports.

## Work

1. Verify commit, submodules, dependencies, licenses, symbols, and build identity.
2. Run vanilla server/client compatibility and protocol-observation tests.
3. Audit Phase 5 evidence for all acceptance requirements and visual sign-offs.
4. Run cold-start, single-player, one-player MP, and teardown smoke gates.

## Acceptance criteria

- Source and dependency provenance is complete with no blocking license issue.
- Vanilla peers require no split-screen-specific protocol or server patch.
- Every global definition-of-done item links to valid passing evidence.
- The report identifies the exact commit and executable hash.
