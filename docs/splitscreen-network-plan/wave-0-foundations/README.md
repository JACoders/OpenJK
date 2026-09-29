# Wave 0: Foundations

Start from the current `splitscreen-local-mp` branch. These tickets are mutually
independent and may run concurrently. They establish contracts and tooling only;
they must not alter the user-visible hosting or joining flow.

## Tickets

- [W0-01: Version split-screen UI assets](W0-01-version-ui-assets.md)
- [W0-02: Define party lifecycle and state contract](W0-02-party-state-contract.md)
- [W0-03: Isolate app-level QA environments](W0-03-isolated-qa-harness.md)
- [W0-04: Record vanilla protocol compatibility baseline](W0-04-vanilla-baseline.md)

## Exit gate

All assets required by the feature are reproducible from a clean clone, the
party state contract is documented and observable, QA runs cannot collide, and
the current vanilla-client wire behavior has a captured baseline.
