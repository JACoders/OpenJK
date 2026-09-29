# P1-05: Input and Settings Ownership Contract

## Objective

Define how physical devices, gameplay actions, menu events, control mappings,
consoles, CVARs, and text entry are owned by one local player.

## Primary ownership

New declarations and design notes in shared input/settings headers. No runtime
implementation and no edits to stock menu definitions.

## Contract contents

- Stable keyboard/mouse and SDL gamepad device identities.
- Mutually exclusive device-to-slot assignment and hot-plug behavior.
- Gameplay, menu, console, and virtual-keyboard event routing.
- Per-player binding, CVAR, console history, and modal-focus namespaces.
- Simulation injection below the physical-device boundary.

## Acceptance criteria

- Every event has exactly one local owner or an explicit shared-system owner.
- Keyboard/mouse cannot implicitly broadcast to all viewports.
- Four simulated gamepads use the same routing path as physical SDL devices.
- Stock control actions, including Force powers, are representable without a
  second mapping system.
