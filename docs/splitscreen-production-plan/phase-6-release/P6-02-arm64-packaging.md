# P6-02: Finalize the ARM64 Packaging Pipeline

## Objective

Make the Apple Silicon packaging pipeline reproducible and complete without
declaring its test output to be the final release artifact.

## Primary ownership

CMake/package scripts, asset manifests, signing/notarization configuration, and
artifact checksums. Do not edit gameplay, tests, or documentation.

## Work

1. Build clean ARM64 binaries from a fresh checkout.
2. Package menus, configs, simulator tools, licenses, and migration defaults.
3. Verify architecture, dynamic libraries, asset lookup, and writable homepaths.
4. Make signing/notarization optional pipeline stages with deterministic inputs.

## Acceptance criteria

- `file`/Mach-O inspection reports ARM64 for every shipped executable/library.
- Repeated pipeline runs reproduce matching unsigned payload hashes.
- Cold start finds every required UI/test asset without developer paths.
- Installation leaves stock Jedi Academy data untouched.
