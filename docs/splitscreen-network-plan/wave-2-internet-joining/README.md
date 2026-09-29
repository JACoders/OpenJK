# Wave 2: Stock Internet Joining

Begin only after Wave 1 is integrated. These tickets are independent: UI entry,
connection fan-out, credential/error handling, and per-client identity have
separate ownership.

## Tickets

- [W2-01: Hand off to stock server browser](W2-01-stock-browser-handoff.md)
- [W2-02: Orchestrate vanilla multi-client joins](W2-02-multi-client-connect.md)
- [W2-03: Handle passwords, rejection, and recovery](W2-03-auth-error-recovery.md)
- [W2-04: Synchronize per-player userinfo](W2-04-userinfo-sync.md)

## Exit gate

Two to four local players can select one stock browser entry and connect to the
same unmodified OpenJK server with correct identities and recoverable errors.
