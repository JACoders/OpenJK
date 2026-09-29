# Split-screen QA harness

This directory contains repeatable smoke and stress configs for local split-screen
multiplayer. The runner copies these configs into the active OpenJK homepath and
launches the native arm64 app with the game VMs disabled so the in-tree code is
tested.

## Run

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

The runners install the authored menu files from `assets/splitscreen/base` into
the selected test home. A clean test home therefore does not depend on files
left behind by an earlier development session. To install only the assets:

```sh
tests/splitscreen/install_assets.sh /path/to/test-home
```

Useful overrides:

```sh
OPENJK_BIN=./build-arm64-native/openjk.arm64.app/Contents/MacOS/openjk.arm64 \
OPENJK_BUILD_DIR=./build-arm64-native \
OPENJK_BASEPATH="/Users/joshabb/Library/Application Support/Steam/steamapps/common/Jedi Academy/SWJKJA.app/Contents" \
OPENJK_HOMEPATH=./runtime-home \
tests/splitscreen/run_splitscreen_qa.sh local_2p_ffa splitnet_localhost
```

Output is written under `runtime-home/base/qa-logs`, with screenshots and
condumps using names without spaces.

The default suite covers:

- 2, 3, and 4 player local split-screen FFA.
- 4 player team and CTF, plus duel.
- Death/respawn flow for split players.
- Per-player profile changes for name, model, saber, saber color, and force
  powers.
- Per-player join/spectate/rejoin state.
- Per-viewport stock player configuration, console, virtual keyboard, and
  control binding screens.
- Deterministic keyboard/mouse and controller routing with no input bleed.
- Controller button remapping, force powers, weapon selection, attack, alt
  attack, jump/use buttons, and a local combat command stream.
- Localhost split-network connection setup for players 2 through 4.

## Public Internet acceptance

The routed public runners drive the visible stock menus, per-player character
and Force-profile screens, Favorites browser, and Join action before asserting
independent network slots, live player state, and device-isolated gameplay:

```sh
tests/splitscreen/run_routed_public_acceptance.sh 2
tests/splitscreen/run_routed_public_acceptance.sh 3
tests/splitscreen/run_routed_public_acceptance.sh 4
```

All three player-count runners have passed against public Internet endpoints.
The four-player run depends on a pre-cgame empty sequenced acknowledgement:
that packet lets vanilla servers deliver secondary gamestates while still
preserving pure-checksum-before-usermove ordering. The client also rejects
placeholder snapshots that reuse another local player's server slot, so a
same-IP policy rejection cannot be misreported as success. Dated screenshots,
logs, server slots, and occupancy details are in
`proof/public-internet-2026-07-22/README.md`.

## External input simulator

The in-game QA configs are useful for deterministic assertions, but keyboard and
mouse can also be driven from outside OpenJK on macOS:

```sh
tests/splitscreen/build_external_input_sim.sh
tests/splitscreen/external/macos_input_sim request-permission
tests/splitscreen/external/macos_input_sim wait 1000 key w down wait 250 key w up mouse 40 0 click left
```

This uses Quartz/CGEvent and requires macOS Accessibility permission for the
terminal or parent process launching the simulator.

Controller simulation uses an external-process SDL bridge. OpenJK creates up to
four ordinary SDL virtual joysticks and listens only on localhost; the simulator
drives those devices from a separate process. Input then travels through the
same SDL polling, joystick-slot assignment, menu routing, bindings, and usercmd
generation as a physical controller. This avoids Apple's restricted virtual-HID
entitlement while still testing the actual controller path inside the engine.

Launch OpenJK with the bridge enabled, then build and drive it:

```sh
OPENJK_VIRTUAL_GAMEPADS=3 OPENJK_VIRTUAL_GAMEPAD_PORT=29180 openjk.arm64 ...
tests/splitscreen/build_external_input_sim.sh
tests/splitscreen/probe_external_gamepad_sim.sh
tests/splitscreen/external/macos_input_sim gamepad 1 axis 1 24000 wait 250 gamepad 1 axis 1 0
tests/splitscreen/external/macos_input_sim gamepad 2 button 0 tap
```

The older `hid-gamepad-demo` command remains available for machines with an
Apple-authorized `com.apple.developer.hid.virtual.device` signing identity, but
it is not required for automated QA.

The external menu run drives the stock player, saber, Force, top-menu, Controls,
join/spectate, virtual-keyboard, CVAR, cheat, and per-player console flows. The
external character run traverses the stock portrait grid horizontally and
vertically, verifies isolation from the other profiles, closes and reopens the
screen by controller, and confirms the final model in gameplay.

The Phase 5 runners under `phase5` exercise two-, three-, and four-player kill,
respawn, simultaneous attack, Force, Duel, Power Duel, Siege, and every stock
multiplayer game type. `run_vanilla_network_qa.sh` puts four clients from one
split-screen process and a fifth untouched upstream client on an untouched
upstream dedicated server. `run_performance_qa.sh` records 600 measured frames
for each player count and checks frame-time, memory, and viewport budgets.

The controller shortcuts used by split-screen players are:

- Start: open that player's stock in-game top menu.
- Back + Start: toggle that player's Quake console.
- X: open that player's virtual keyboard from player setup.
- Left shoulder: open the stock saber setup screen.
- Right shoulder: open the stock Force setup screen.
- B or Back: cancel or return to the previous split-screen menu.

`stock_host_handoff` verifies that completing split-screen player setup opens the
stock Create Server workflow and records a pending host operation instead of
launching a hard-coded map.
