# Test And Release Guide

## Functional Suites

From the repository root:

```sh
tests/splitscreen/run_splitscreen_qa.sh
tests/splitscreen/run_external_gamepad_qa.sh
tests/splitscreen/run_external_menu_qa.sh
tests/splitscreen/run_external_character_qa.sh
tests/splitscreen/run_external_combat_qa.sh
tests/splitscreen/run_external_controls_qa.sh
tests/splitscreen/run_external_lifecycle_qa.sh
tests/splitscreen/run_external_team_profile_qa.sh
tests/splitscreen/run_external_topmenu_qa.sh
tests/splitscreen/run_stock_regression_qa.sh
tests/splitscreen/run_vanilla_network_qa.sh
tests/splitscreen/run_performance_qa.sh
```

Phase 5 runners under `tests/splitscreen/phase5` cover two-, three-, and
four-player combat; Duel and Power Duel; every stock game type; Siege; chained
kills; simultaneous input; Force use; death; respawn; spectate; and rejoin.

Set `OPENJK_BUILD_DIR` to test a different build tree and use isolated homepaths
under `/tmp`. Screenshots must have names without spaces. Run
`tests/splitscreen/make_contact_sheet.py` to label a visual review set.

## ARM64 Package

```sh
scripts/release/build_macos_arm64.sh
scripts/release/audit_macos_arm64.sh dist/OpenJK-SplitScreen-arm64
```

The release build downloads a pinned SDL2 source archive, verifies its SHA-256,
builds it statically for ARM64/macOS 11, builds OpenJK Release binaries, stages
modules and menus, rejects developer-machine dylib paths, normalizes timestamps,
and writes payload/archive checksums. Set `OPENJK_SIGN_IDENTITY` for an optional
signing pass; notarization credentials are intentionally external.

Packaging normally rejects dirty source. `OPENJK_ALLOW_DIRTY=1` exists only for
development candidates and records `dirty=true` in `BUILD-MANIFEST.txt`.

For a frozen package and matching build directory, run the complete gate with:

```sh
scripts/release/run_release_gate.sh \
  dist/OpenJK-SplitScreen-arm64 \
  build-arm64-package
```

The gate records the commit and executable/package hashes, verifies that staged
modules match the build products, and runs stock regression, all split-screen
and external-input suites, Phase 5 gameplay, vanilla network compatibility, and
the performance matrix in isolated homepaths.
