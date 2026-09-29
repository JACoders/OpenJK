# P2-05: Implement Local-Input Dispatch

## Objective

Implement the neutral event and device dispatch foundation from P1-05 without
choosing bindings or player-facing assignment behavior yet.

## Primary ownership

New generic modules `codemp/client/cl_local_input.*` and their unit tests. Do not
edit SDL/platform ingestion, stock Controls UI, cgame, renderer, or networking.

## Work

1. Define stable device/event records and slot-targeted dispatch queues.
2. Enforce one explicit owner or shared-system owner for every queued event.
3. Add physical and simulator producer registration through one API.
4. Expose telemetry for source device, destination slot, context, and consumption.

## Acceptance criteria

- Parallel producers cannot deliver one event to multiple local slots.
- Reassignment flushes or transfers pending state according to the contract.
- Simulator events are indistinguishable after the producer boundary.
- The foundation has no Player 1 default and preserves stock one-player input.
