# Phase 0: Coverage and Oracle Foundation

## Entry gate

The current tree builds, existing routed 2/3/4-player local and public proofs
remain readable, and their known limitations are recorded rather than treated
as passes.

## Parallel tickets

- [GP0-01: Executed coverage ledger](GP0-01-executed-coverage-ledger.md)
- [GP0-02: Isolated end-to-end runner](GP0-02-isolated-e2e-runner.md)
- [GP0-03: State, visual, audio, and resource oracles](GP0-03-trustworthy-oracles.md)

## Independence rule

The ledger, runner, and oracle packages have separate directories and schemas.
Agents may agree on stable manifest field names before work begins, but may not
edit another ticket's files.

## Exit gate

The ledger is generated from actual logs, every runner gets a clean homepath and
port range, and the oracles correctly pass known-good proofs while rejecting at
least one deliberately invalid fixture per oracle class.

## Result

PASS on 2026-07-23.

- The coverage ledger imports legacy proof conservatively, marks it
  `legacy-unverified`, and prevents it from qualifying a frozen-build seal.
- Four concurrent harness cases used four unique homepaths and sixteen unique
  locked ports, then released every process, listener, and lock.
- The oracle self-test passed three positive player counts and rejected
  duplicate, blank, stale, cross-pane, wrong-client, silent, leaked, truncated,
  and hash-tampered fixtures.
- An on-disk evidence bundle passed through the oracle inside the isolated
  runner, and the resulting hash-pinned manifest passed validate-only replay.
