# GP6-04: Compatibility and Cold-Package Certification

## Objective

Prove that a clean packaged build can repeat the certified journeys against
supported stock, pure, QVM/native, download, public, and selected mod servers.

## Exclusive ownership

- `tests/splitscreen/gameplay/stress/package_compat/**`
- Evidence under `tests/splitscreen/gameplay/results/stress/package_compat/**`

No production changes are allowed.

## Matrix

- Install to a clean location with no developer build outputs on the search path.
- Launch fresh and persisted homes; verify required UI/cgame modules and assets
  come from the package manifest.
- Replay representative 2/3/4-player local, controlled pure, public, map
  download/rotation, native/QVM, and selected mod journeys.
- Upgrade an older split-screen config, then uninstall/remove the test package
  without deleting user data.

## Acceptance

- All loaded binary/module/asset hashes belong to the package.
- No developer-home dependency, missing asset, unsafe config migration, or
  protocol/mod-specific crash appears.
- Cold package results match Phase 5 behavior and cleanly preserve user data.

## Status

PLANNED.
