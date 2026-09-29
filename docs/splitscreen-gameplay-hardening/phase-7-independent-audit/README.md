# Phase 7: Independent Audit

## Entry gate

Phases 5–6 pass on one frozen release-candidate manifest.

## Parallel tickets

- [GP7-01: Coverage and evidence audit](GP7-01-coverage-evidence-audit.md)
- [GP7-02: Source, build, and package audit](GP7-02-source-build-audit.md)

## Independence rule

Auditors are read-only. They do not reuse the implementing agent's conclusion,
change artifacts, rerun with looser thresholds, or repair findings.

## Exit gate

Both independent reports say PASS with no unresolved contradiction. Any finding
reopens the appropriate earlier phase and invalidates the frozen manifest.
