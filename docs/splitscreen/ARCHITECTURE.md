# Architecture

## Process Model

Split screen uses one OpenJK process, one renderer, one filesystem, and one audio
device. It does not launch or embed multiple game instances. Player 1 uses the
ordinary client globals; Players 2 through 4 use explicit `splitScreenClient_t`
network contexts with independent challenge, netchan, reliable commands,
snapshots, usercmds, connection state, and timeout/reconnect state.

The engine loads four ARM64 cgame modules. Before a viewport frame or cgame
callback, the VM layer selects that player's cgame context and shared-memory
block. The renderer composes independent 3D scenes and 2D HUD/menu passes into
stable two-, three-, or four-player rectangles.

## Input And UI

SDL discovers physical game controllers and maps each device slot to exactly one
party row. Keyboard/mouse and controller events enter player-aware queues before
bindings, UI handling, or usercmd generation. Menu catchers, cursor state,
bindings, virtual keyboards, and consoles carry an owner player.

The UI reuses stock menu definitions and handlers. The split-screen layer sets a
viewport transform and swaps per-player profile/CVAR state around each stock
menu paint and action. Engine-global CVARs are deliberately not duplicated.

## Networking

Every local player performs a normal challenge/connect sequence and occupies a
normal server client slot. Outgoing packets and connectionless replies are
matched to the owning client context. The server sees ordinary Jedi Academy
clients, so an unmodified OpenJK server and ordinary remote clients remain
compatible.

Local hosting is a stock Create Server handoff followed by localhost attachment.
Internet joining is a stock browser handoff followed by one connection per local
player to the selected address. All viewports share that destination.

## QA Boundaries

The QA system includes in-engine assertions plus an out-of-process macOS input
simulator. Virtual controller packets create SDL virtual joysticks, then traverse
the same SDL slot, menu, binding, and usercmd paths as physical devices. Quartz
events exercise the system keyboard/mouse path. Game screenshots are generated
with `screenshot_png`, checked by machine oracles, assembled into labeled contact
sheets, and visually inspected.
