# P7-02: Certify ARM64 Packaging

## Objective

Produce and certify release-candidate packages from the frozen commit.

## Primary ownership

`artifacts/certification/packages`, isolated clean build roots, signing output,
and package report only. Do not modify source, docs, or other reports.

## Work

1. Build unsigned payloads twice from clean roots and compare hashes.
2. Verify ARM64 architecture, dependencies, assets, licenses, and installation.
3. Sign/notarize the reproducible payload when credentials are available.
4. Smoke-test installed output from a path without developer files.

## Acceptance criteria

- Clean unsigned payloads are byte-for-byte reproducible.
- Every executable/library is ARM64 and resolves approved dependencies.
- Installed stock, split-screen, simulator, and uninstall paths pass.
- The report lists exact commit, environment, and package hashes.
