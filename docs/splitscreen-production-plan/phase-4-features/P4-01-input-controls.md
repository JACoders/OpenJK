# P4-01: Route Devices and Controls

## Objective

Deliver exclusive physical and simulated device ownership for gameplay, stock
menus, and complete stock action binding.

## Primary ownership

SDL/platform producers plus dedicated `cl_split_input.*` and
`ui_split_controls.*` provider modules. Do not edit `ui_main.c`,
`cl_local_input.*`, profiles, consoles, host/browser, or rendering.

## Work

1. Assign keyboard/mouse and gamepads mutually exclusively to local slots.
2. Route axes, buttons, mouse motion, keys, menu navigation, and hot-plug events.
3. Mirror every stock control action and default, including weapons, saber,
   movement, communication, and every Force power.
4. Feed external simulators through the physical-device routing boundary.

## Acceptance criteria

- Four simultaneous devices change only their assigned players and viewports.
- Keyboard/mouse never broadcasts, and one device cannot occupy two slots.
- Default bindings are usable; remaps persist independently and can be reset.
- Physical and simulated gamepads produce identical routing telemetry.
