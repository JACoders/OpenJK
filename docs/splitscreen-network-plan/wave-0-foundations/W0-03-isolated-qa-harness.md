# W0-03: Isolate App-Level QA Environments

## Objective

Make each OpenJK QA invocation use unique ports, home paths, logs, screenshots,
and virtual-controller channels so independent agents cannot corrupt results.

## Ownership

- `tests/splitscreen` runners and simulator launch utilities.
- Do not change engine or UI behavior.

## Work

1. Allocate ports per run instead of hard-coding shared values.
2. Create a unique temporary home and artifact directory per scenario.
3. Ensure cleanup terminates child game and simulator processes.
4. Add a lock or explicit rejection for tests requiring foreground window focus.
5. Emit a machine-readable result manifest listing build, commands, logs,
   screenshots, assertions, and exit status.
6. Preserve failed-run artifacts under a path without spaces.

## Acceptance criteria

- Headless command-level tests may run concurrently without collisions.
- Foreground visual/input tests serialize automatically.
- A failed test cannot make a later test pass or fail through stale CVARs.
- Runners return nonzero for launch failure, timeout, assertion failure, or crash.

## Verification

- Launch two nonvisual scenarios concurrently and verify unique resources.
- Intentionally fail one scenario and confirm its manifest and artifacts remain.
- Run the same scenario twice and compare asserted state.
