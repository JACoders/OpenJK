# P8-01: Seal Release Artifacts

## Objective

Combine the independently certified outputs into one immutable release set.

## Primary ownership

Final release directory, manifest, checksums, signatures, and publication record.
Do not modify source, packages, reports, acceptance criteria, or documentation.

## Work

1. Confirm all four Phase 7 reports name the same commit and executable hash.
2. Select only the package hashes approved by package certification.
3. Generate the final manifest, checksums, signatures, and evidence index.
4. Verify the sealed directory from a clean machine before publication.

## Acceptance criteria

- Every final artifact is immutable, hashed, and linked to an approving report.
- No file was rebuilt, edited, or substituted after certification.
- A clean-machine verification reproduces all manifest checks.
- The publication record names the commit, packages, reports, and release tag.
