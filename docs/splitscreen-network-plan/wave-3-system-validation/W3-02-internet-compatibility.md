# W3-02: Vanilla Internet Compatibility Matrix

## Objective

Validate party joining against unmodified OpenJK servers and common real-world
server configurations.

## Matrix

- Local players: 2, 3, 4.
- Server: matching vanilla revision, supported older vanilla revision, passworded,
  pure, downloads enabled/disabled, near capacity, map rotation.
- Network: LAN, loopback, routed internet, latency/loss profiles.
- Outcomes: success, wrong password, full, protocol mismatch, timeout, kick.

## Acceptance criteria

- No server-side split-screen code is required.
- Every accepted player has independent scoreboard/userinfo state.
- Rejections and partial admissions follow the documented recovery policy.
- All clients remain synchronized through at least five map changes.

## Deliverables

- Compatibility table naming exact server/client revisions.
- Sanitized packet/log evidence and reproducible commands.
- Documented limitations for per-IP policies or incompatible mods.
