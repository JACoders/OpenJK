# OpenJK Local Split Screen

This build adds two-, three-, and four-player local multiplayer to Jedi Academy
MP in one OpenJK process. Each player is a real network client with an independent
viewport, HUD, stock profile menus, controls, userinfo, command stream, console,
and connection lifecycle.

## Requirements

- Apple Silicon Mac running macOS 11 or later.
- A legal Jedi Academy installation containing `base/assets0.pk3` and the other
  retail game data.
- One input device per active local player. Keyboard and mouse count as one
  device; each gamepad counts as one device.

## Install

The release includes a self-contained ARM64 SDL build and does not require
Homebrew or Rosetta.

1. Extract `OpenJK-SplitScreen-arm64.zip`.
2. In Terminal, run `./install.sh` from the extracted directory.
3. If Jedi Academy is not in the default Steam location, pass its app Contents
   directory: `./install.sh "/path/to/SWJKJA.app/Contents"`.
4. Launch `openjk.arm64.app` from the Jedi Academy app Contents directory.

The installer adds OpenJK applications to the game directory and installs this
build's modules and menu assets under
`~/Library/Application Support/OpenJK/base`. It does not edit or replace retail
Jedi Academy PK3 files. Single-player remains available through
`openjk_sp.arm64.app`.

## Start A Party

1. Open **Multiplayer**, select **Play**, then select **Split Screen**.
2. Choose 2, 3, or 4 players. For a two-player party, choose either
   **Top / Bottom** or **Left / Right** screen layout.
3. Choose **Local Match** to create a game or **Server Party** to join an
   existing server.
4. Assign keyboard and mouse or a controller to every player. Assignments are
   mutually exclusive; selecting a device already in use swaps ownership.
5. Select **Next**. Every viewport shows the stock Player Configuration screen.
6. Each player chooses a name, character, saber, colors, and Force powers with
   their assigned device, then applies or joins.
7. Local Match continues to the stock Create Server menus. Server Party
   continues to the stock server browser.

All local players join the same match. They cannot use one process to connect to
different servers.

See [PLAYER-GUIDE.md](PLAYER-GUIDE.md) for controls and per-player menus, and
[NETWORK-GUIDE.md](NETWORK-GUIDE.md) for hosting and vanilla servers.

## End-to-End Hardening

The phased, parallel gameplay and bug-closure plan is in
[Split-Screen End-to-End Gameplay Hardening](../splitscreen-gameplay-hardening/README.md).
It distinguishes spawn/input smoke tests from complete scoring, objective,
death/respawn, intermission, restart, network-recovery, and clean-exit coverage.
