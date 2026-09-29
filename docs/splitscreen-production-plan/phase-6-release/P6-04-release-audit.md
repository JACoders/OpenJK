# P6-04: Build Release-Audit Automation

## Objective

Build independent automation for provenance, regressions, compatibility, and
evidence checks. Actual certification runs in Phase 7.

## Primary ownership

Audit scripts, compliance/security checks, evidence schemas, and approval-record
templates. Do not edit gameplay, packaging implementation, or user docs.

## Work

1. Automate source, submodule, dependency, license, and symbol checks.
2. Define cold-start, single-player, one-player MP, 2/3/4 split-screen,
   local-host, vanilla-join, simulator, and teardown smoke gates.
3. Validate that every acceptance requirement can link to typed evidence.
4. Reject missing, stale, manually altered, or visually unreviewed fixtures.

## Acceptance criteria

- Audit scripts pass against known-good and intentionally broken fixtures.
- A waived rendering or input-isolation failure remains release-blocking.
- Screenshot records require both machine checks and human visual sign-off.
- Approval templates require commit, environment, report, and artifact identities.
