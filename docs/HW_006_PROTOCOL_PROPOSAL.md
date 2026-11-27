# HW-006 negotiated protocol proposal

This proposal is grounded in the protocol 1.3/controller documents at upstream
NEUROGENIC revision `ae66cee8b526cd999c2e957ecf1302080a92aaff`. It is not a
wire change and HW-005 does not modify the upstream repository.

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
