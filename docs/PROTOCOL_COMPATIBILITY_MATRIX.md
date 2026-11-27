# Protocol compatibility matrix

This matrix is a proposal based on normative upstream revision
`5f94323acff8a78059c05a4e1e508a6a7e0ced44`. Protocol 1.3 remains the only
implemented production contract. “Extended” means a future host/controller
that has upstream-approved minor-1.4 behavior.

| Host | Controller | Negotiated result |
|---|---|---|
| 1.3 | baseline | Exact 1.3: 67 controls, six-byte inventory, 13-byte snapshot |
| 1.3 | extended | Controller selects 1.3; pushes and encoder RGB are suppressed |
| extended | baseline | Host accepts 1.3 and exposes no optional facilities |
| extended | extended, push only | Validate push inventory; accept push events and suffix bitmap only |
| extended | extended, RGB only | Validate RGB-destination inventory; baseline input snapshot remains exact |
| extended | extended, both | Validate both records independently and enable both namespaces |
| extended | unknown capability bits | Mask unknown bits; never infer a namespace or behavior |
| extended | advertised option with wrong/missing inventory | Reject that negotiated session as incompatible; no optional traffic |
| any | unsupported major | Reject before inventory/session creation |
| any | malformed reply/capabilities/inventory | Reject negotiation; clear connection-local state |

The controller must select a mutually supported minor rather than replying 1.4
to a 1.3 HELLO. Current upstream 1.3 rejects a selected minor above its maximum,
so this downgrade is essential. A failed 1.4 optional-inventory validation is
not silently reinterpreted in the same session; an explicit fresh 1.3
negotiation may establish baseline mode.

Capabilities are independent. Push support never implies encoder illumination,
and encoder illumination never expands the input inventory. Unknown bits are
preserved only for diagnostics and are otherwise ignored. Baseline identities,
fingerprint, bitmap interpretation, sequence rules and generation rules remain
unchanged in every row.

## Failure matrix

| Condition | Required result |
|---|---|
| Bad magic, length, major or CRC | Decoder rejects and counts the frame |
| Unsupported message type | Ignore/count; no state transition |
| Invalid control or destination | Reject message; do not alias another identity |
| Duplicate input sequence | Drop duplicate |
| Sequence gap | Cancel ownership, create a new generation, request snapshot |
| Stale generation | Drop input and retain reconciliation requirement |
| Input overflow | Expose overflow and request authoritative snapshot |
| Feedback overflow | Retain newest desired state and converge on refresh |
| USB loss or heartbeat timeout | Cancel session, animation and queued feedback |
| New session | Clear old RX/TX/render work before full-state publication |

