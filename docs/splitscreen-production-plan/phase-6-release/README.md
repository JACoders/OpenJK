# Phase 6: Release Hardening

## Entry gate

The same frozen Phase 5 build passes all matrices. Every release-blocking defect
is closed and rerun evidence is attached.

## Parallel tickets

- [P6-01: Meet performance budgets](P6-01-performance.md)
- [P6-02: Finalize the ARM64 packaging pipeline](P6-02-arm64-packaging.md)
- [P6-03: Finalize operator and player docs](P6-03-documentation.md)
- [P6-04: Build release-audit automation](P6-04-release-audit.md)

## Exit gate

All hardening branches merge without changing frozen contracts. A fresh
candidate is built and the complete Phase 5 matrix passes again. That exact
commit and build then become immutable inputs to Phase 7.
