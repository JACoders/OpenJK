# W0-01: Version Split-Screen UI Assets

## Objective

Move the split-screen menu definitions currently living under generated
`runtime-home` state into a versioned, distributable source location.

## Ownership

- UI asset source and asset installation/package rules.
- Do not change `codemp/ui/ui_main.c` or networking code.

## Work

1. Inventory `jampmenus.txt`, `jampingame.txt`, and split-screen `.menu` files.
2. Separate authored files from screenshots, logs, configs, and generated data.
3. Add authored assets under a repository path suitable for development and
   release packaging.
4. Add a deterministic install/copy target for the ARM64 app test home.
5. Ensure custom menu lists extend the stock list without dropping stock menus.
6. Add ignore rules that exclude runtime output but not authored menu files.

## Acceptance criteria

- A clean clone plus the documented build command produces all split-screen
  menus without copying files from an old `runtime-home`.
- Stock menus still load from the game assets.
- `git status` remains clean after an install and smoke run.
- No screenshots, logs, QKEY data, or player configs are committed.

## Verification

- Delete or rename the test home, reinstall assets, and launch the ARM64 build.
- Open the multiplayer, split-screen setup, and per-player profile menus.
- Capture and inspect one screenshot of each menu family.
