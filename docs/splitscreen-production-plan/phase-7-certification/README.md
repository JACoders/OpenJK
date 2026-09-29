# Phase 7: Frozen-Candidate Certification

## Entry gate

Phase 6 hardening is integrated, the complete Phase 5 matrix passes again, and
one commit/build is frozen. Phase 7 agents receive read-only source/build inputs
and write only to their assigned report or artifact directories.

## Parallel tickets

- [P7-01: Certify performance](P7-01-performance-certification.md)
- [P7-02: Certify ARM64 packaging](P7-02-package-certification.md)
- [P7-03: Certify documentation workflows](P7-03-documentation-certification.md)
- [P7-04: Certify compatibility and provenance](P7-04-compatibility-audit.md)

## Exit gate

All four independent reports approve the same commit and build. No source,
threshold, document, package-pipeline, or test change occurs during this phase.
