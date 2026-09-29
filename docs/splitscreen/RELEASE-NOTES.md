# Split-Screen Release Notes

## Implemented

- Two-, three-, and four-player Jedi Academy MP in one process.
- Independent network clients, snapshots, usercmds, cameras, scenes, HUDs,
  stock profile/saber/Force menus, top menus, controls, CVARs, consoles, and
  virtual keyboards.
- Keyboard/mouse plus controller assignment with exclusive device ownership.
- Stock local-server creation and stock internet/LAN browser handoffs.
- Vanilla OpenJK server compatibility and ordinary remote players, including a
  verified four-local-plus-one-remote session.
- All stock MP game types, death/respawn, spectate/rejoin, team changes, Force
  use, attacks, controller rebinding, and reconnect/error isolation.
- Native Apple Silicon package with static SDL2 and no Rosetta/Homebrew runtime
  dependency.
- Stock single-player and ordinary one-player multiplayer regression coverage.

## Limits

- All local players share one server destination and cannot join separate games.
- Window, renderer, audio device, filesystem, and server-browser selection are
  process-wide.
- A server can limit multiple clients from one IP or require a mod, download,
  password, authentication, or additional slots.
- Signing and Apple notarization require release-owner credentials and are
  optional pipeline stages, not embedded secrets.
