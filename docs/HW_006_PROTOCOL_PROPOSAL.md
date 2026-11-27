# HW-006 negotiated protocol proposal (superseded by HW-006A audit)

This HW-005 planning note is retained for history. The current source-grounded
proposal is [PROTOCOL_EXTENSION_SPEC.md](PROTOCOL_EXTENSION_SPEC.md), based on
upstream `5f94323acff8a78059c05a4e1e508a6a7e0ced44`; the audited baseline is in
[PROTOCOL_BASELINE_AUDIT.md](PROTOCOL_BASELINE_AUDIT.md). Neither is an
implemented wire change.

Keep the existing 67-control identity and inventory fingerprint as the
mandatory baseline. Negotiate optional capabilities before using any extension:

- a separate eight-entry contextual-push input inventory, with its own stable
  identities and fingerprint;
- a separate eight-entry encoder-RGB destination inventory (destinations are
  not input controls);
- normalized magnitude and explicit assigned/available state only when both
  sides advertise support;
- temporal modes constrained to the firmware's bounded set, while the host
  owns musical phase and generations;
- authoritative current-state snapshots plus monotonic host/session generation
  after connect, overflow or reconnect; and
- bounded, length-checked payloads with unknown capabilities ignored.

Existing RGB/brightness/temporal feedback can represent host-rendered output
for already identified destinations; it need not transport raw musical values.
Normalized magnitude and assignment availability are useful only if the host
chooses firmware-side interpolation/rendering. HW-006 should first audit whether
destination negotiation can reuse the existing feedback envelope, then add the
smallest capability/inventory messages jointly in host and firmware.

Implementation order: freeze identifiers and fingerprints; add host parsing and
fallback tests; add firmware negotiation and baseline-only fallback; add push
events and reconciliation; add destination feedback; finally enable richer
magnitude/state fields. A 1.3 host sees exactly the old 67 controls, receives no
push events, and need not drive encoder lighting. Disconnect discards stale
feedback generations; reconnect starts with explicit capabilities and a full
input-state reconciliation.
