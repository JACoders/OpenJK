# GP0-02: Isolated End-to-End Runner

## Objective

Provide one deterministic runner contract for clean-home, multi-process, local,
controlled-WAN, and public split-screen journeys.

## Exclusive ownership

- `tests/splitscreen/gameplay/harness/**`
- `tests/splitscreen/gameplay/run_e2e.sh`

No production source changes are allowed.

## Work

- Allocate unique homepaths, client/server ports, qports, logs, screenshot
  directories, and process groups per case.
- Capture binary/module hashes and the exact command line before launch.
- Support 1/2/3/4 players, native/QVM selection, controlled dedicated servers,
  remote fifth clients, and validate-only replay.
- Guarantee bounded timeouts, graceful teardown, and escalation to diagnostics
  without killing unrelated processes.
- Produce a manifest even when setup, launch, assertion, or teardown fails.

## Acceptance

- Four runners can execute concurrently without file, process, or port overlap.
- A failed case preserves all logs and exits nonzero.
- A clean case leaves no child process, socket listener, or temporary server
  client behind.
- Validate-only mode proves artifacts without contacting a server.

## Evidence

Include concurrency, timeout, crash, and teardown self-tests.

## Status

IMPLEMENTED and self-tested on 2026-07-23.

- Four concurrent 1/2/3/4-player cases received distinct homepaths and 16
  distinct locked ports, with every lock removed after exit.
- Success, nonzero client failure, bounded timeout, process-group teardown,
  listener teardown, manifest-format rejection, failed-status rejection, hash
  replay, and validate-only replay passed
  `tests/splitscreen/gameplay/harness/self_test.sh`.
- The final self-test completed in approximately 9.5 seconds with
  `GP0-02 self-test: PASS`. Its preserved evidence root was
  `/private/tmp/openjk-e2e-selftest.AsBhGN`.
- Socket rebinding is attempted when the host permits it. This managed sandbox
  rejects all socket binds with `EPERM`, so the sandbox run additionally proves
  teardown through the recorded process PID and removal of its atomic port
  lock.

Phase 5 exposed and closed a false-green path: a client process can exit zero
after an in-game `Split*Assert*: FAIL`. The runner now marks the manifest
failed when the authoritative client log contains a split assertion failure,
`Sys_Error`, an assertion abort, or an ASan/UBSan/LSan marker. The self-test
includes a zero-exit worker that emits `SplitInputAssertCmd: FAIL` and proves
that both the process result and manifest are rejected. The current self-test
passes in the socket-restricted sandbox at
`/private/tmp/openjk-e2e-selftest.KJ1T45`.
