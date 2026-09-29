# P0-02: Add Viewport Telemetry

## Objective

Expose what each viewport actually renders without changing render behavior.

## Primary ownership

New diagnostic-only code in `codemp/cgame` plus read-only console output. Do not
edit client networking, input, UI menus, or test image analysis.

## Work

1. Emit viewport index, rectangle, local player, client number, snapshot number,
   player-state origin/view angles, camera origin/angles/FOV, team, health, model,
   and HUD identity source.
2. Emit scene entity count, areamask identity, refdef flags, and frame checksum.
3. Add rate-limited status and one-frame dump commands.
4. Make telemetry deterministic and disabled by default.

## Acceptance criteria

- One dump explains why two captured viewports are identical or divergent.
- Telemetry has no measurable effect when disabled.
- Values can be asserted by shell-based QA without parsing screenshots.
- No sensitive server credentials or chat content are logged.
