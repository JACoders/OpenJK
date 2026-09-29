# Phase 8: Seal the Release

## Entry gate

All Phase 7 reports independently approve the same frozen commit and build.

## Ticket

- [P8-01: Seal release artifacts](P8-01-seal-release.md)

This phase intentionally has one ticket. Final package selection, report
aggregation, hashing, and signing form one sequential chain and cannot be split
honestly among parallel agents.

## Exit gate

The immutable release manifest, packages, reports, checksums, and signatures all
refer to one approved commit and are ready for publication.
