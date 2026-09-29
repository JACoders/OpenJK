# W4-02: macOS ARM64 Packaging and Platform QA

## Objective

Produce a clean Apple Silicon application bundle containing the engine changes,
native modules, authored menus, and required testable defaults.

## Work

1. Verify every binary and dynamic library is ARM64 and resolves correctly.
2. Package versioned split-screen assets without generated player data.
3. Test fresh install, upgrade from prior config, quarantine/signing behavior,
   windowed/fullscreen sessions, Retina scaling, and multiple controllers.
4. Verify no x86_64 build or Rosetta dependency enters release instructions.
5. Produce checksums and reproducible build metadata.

## Acceptance criteria

- The app launches on a clean Apple Silicon test account.
- Stock single-player and multiplayer remain functional.
- Split hosting and internet joining work from packaged assets.
- Bundle inspection reports only expected architectures and dependencies.

## Deliverables

- Release bundle, checksums, build manifest, and platform test report.
