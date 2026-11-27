# Protocol compatibility

The baseline is exactly NEUROGENIC controller protocol 1.3 at upstream commit
`40d5b94318f9623b74d0350a8bef749cefa7947b`: identity
`neurogenic.reference-controller-v2`, 67 controls and fingerprint
`0x3256434e`. The firmware skeleton exposes versioned identity over CDC but does
not yet claim full framing or negotiation conformance.

HW-002's bounded text CDC diagnostics are deliberately not the binary protocol
and must not share a production binary stream. No identity, inventory,
fingerprint or wire version changed.

Compatible mode reports the 58 established buttons and nine encoders only.
Eight PEL12T push switches and encoder illumination destinations are physical
capabilities without protocol-1.3 identities. They remain suppressed except in
raw standalone diagnostics. A future host-coordinated extension must negotiate
a distinct capability and inventory, stable push and illumination identities,
counts, fingerprint, feedback behavior and fallback. It must not reinterpret
an existing wire ID or unilaterally increment a protocol number here.

HW-004 implements local rendering/calibration facilities exercised only through bounded standalone diagnostics. It does not claim that protocol 1.3 transports the added operational/calibration representation and does not advertise a new capability. The host remains authoritative for RGB, normalized magnitude and operational meaning; future negotiation must coordinate any richer representation.

HW-005 diagnostic version 1 is CDC text, not protocol 1.3 framing. The proposed
joint extension at upstream revision `ae66cee8b526cd999c2e957ecf1302080a92aaff`
is in `HW_006_PROTOCOL_PROPOSAL.md`: preserve the 67-control fallback and keep
optional push inputs separate from encoder-light destinations.
