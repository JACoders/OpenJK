# W0-04: Record Vanilla Protocol Compatibility Baseline

## Objective

Capture the exact connection and userinfo behavior expected by an unmodified
OpenJK server before expanding the network workflow.

## Ownership

- Protocol notes, packet/log probes, and compatibility fixtures.
- Do not alter client connection code.

## Work

1. Run one stock client against a clean vanilla dedicated server.
2. Record connect, challenge, userinfo, gamestate, snapshot, reliable command,
   map change, disconnect, password failure, and full-server behavior.
3. Identify fields that must be unique per local client and fields that may be
   shared by clients behind one IP address.
4. Document server CVARs or mods known to restrict multiple clients per IP.
5. Produce sanitized fixtures or log patterns for later regression tests.

## Acceptance criteria

- The baseline names the tested OpenJK revisions and protocol version.
- No server modification is required by the documented happy path.
- Sensitive values and local filesystem details are absent from fixtures.
- Later tests can distinguish protocol incompatibility from UI failure.

## Verification

- Replay or pattern-check the sanitized baseline.
- Confirm a second ordinary client from another machine can join the baseline
  server and survive a map change.
