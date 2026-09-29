# W2-03: Handle Passwords, Rejection, and Recovery

## Objective

Make authentication and server rejection understandable and recoverable for a
party without confusing Player 1's stock browser flow.

## Ownership

- Credential propagation, error aggregation, retry, and cancellation logic.
- Do not edit packet routing, browser layout, or per-player profile data.

## Work

1. Apply the stock password entry to every party connection securely.
2. Handle bad password, full server, banned IP, protocol mismatch, pure/download
   failure, timeout, and per-IP connection-limit rejection.
3. Define policy for partial admission: present status, retry failed players, or
   leave as a party; never silently continue with missing players.
4. Clear credentials and stale errors on cancellation/disconnect.
5. Keep logs useful without printing passwords.

## Acceptance criteria

- Every failure identifies affected local slots and a practical recovery action.
- Retrying does not duplicate already connected clients.
- Leaving as a party returns cleanly to the stock browser.
- Password material is absent from console, condump, and QA artifacts.

## Verification

- Automated fixtures for every listed rejection.
- Partial-capacity test where only some local slots are admitted.
- Retry with corrected password and retry after a slot becomes available.
