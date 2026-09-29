# W3-04: Lifecycle and Fault Stress

## Objective

Stress transitions and resource cleanup beyond ordinary happy-path matches.

## Scenarios

- Repeated host, leave, browse, join, disconnect loops.
- Rapid menu open/close during connect and map load.
- One local client timeout, kick, or malformed packet while others play.
- Server shutdown, restart, map crash, reconnect, and renderer restart.
- Controller removal, focus loss, sleep/wake, and network interface change.
- Long soak with combat, Force powers, deaths, chat, console, and map rotation.

## Acceptance criteria

- No crash, deadlock, use-after-free, stale catcher, stuck key, or leaked socket.
- Party state returns to a documented state after every interruption.
- One client's failure never corrupts another client's snapshot or commands.
- Memory and handle usage remain bounded during the soak.

## Deliverables

- Sanitizer and soak reports with peak resource counts.
- Deterministic fault-injection hooks suitable for CI.
- Minimal reproducer and regression test for every defect found.
