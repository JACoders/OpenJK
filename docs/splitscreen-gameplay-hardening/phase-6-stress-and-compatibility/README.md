# Phase 6: Stress and Compatibility Certification

## Entry gate

All Phase 5 journeys pass against one frozen build and complete artifact
manifest.

## Parallel tickets

- [GP6-01: Physical-device certification](GP6-01-physical-devices.md)
- [GP6-02: Networked lifecycle soak](GP6-02-network-soak.md)
- [GP6-03: Performance, memory, and sanitizer certification](GP6-03-performance-sanitizers.md)
- [GP6-04: Compatibility and cold-package certification](GP6-04-compatibility-package.md)

## Independence rule

Each ticket owns separate machines/processes where needed, homepaths, port
blocks, logs, captures, and reports. No ticket changes production code,
thresholds, or another ticket's environment.

## Exit gate

All stress budgets pass on the same Phase 5 hashes. Any crash, leak, corruption,
stuck input, compatibility failure, or unexplained performance regression
reopens Phase 4 and invalidates Phases 5–6.
