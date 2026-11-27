# Proposed negotiated controller extension

Status: **provisional, non-normative, not implemented**. This proposal is based
on upstream `5f94323acff8a78059c05a4e1e508a6a7e0ced44`. Numeric message IDs are
not allocated here. Production remains protocol 1.3 until the host repository
approves and commits a joint contract.

## Inventory reconciliation

The manufactured design has 58 ordinary switch inputs (including the DATA
shaft push used as the existing navigation Enter input), nine rotary encoders,
and eight additional P1-P8 pushes. Quadrature A/B lines are electrical samples,
not logical controls. The physical output inventory is 58 addressable RGB
destinations and eight encoder RGB destinations driven by 24 TLC channels.
Three TLC channels form one logical encoder destination. Musical assignments
remain host profile state and are not inventory.

Baseline logical inventory remains 58 buttons plus nine rotations. DATA push is
not added again: `nav.DATA.S1` is the physical source for the existing
`navigation.enter` relationship and must not produce two logical events.

## Smallest coherent extension

Propose negotiated minor 1.4, with firmware selecting the highest mutually
supported minor from HELLO. A controller presented to a 1.3 host selects 1.3,
sends the exact six-byte baseline inventory, exact 13-byte snapshot, and
suppresses every optional push and encoder-light extension.

Two independent provisional capability bits are required:

- `CONTEXT_ENCODER_PUSH` (suggested bit 11): optional input inventory and events.
- `CONTEXT_ENCODER_RGB` (suggested bit 12): optional feedback destinations.

Unknown bits are masked and ignored. Presence of a bit never validates its
inventory. Rich normalized-magnitude rendering needs no initial wire
capability: the existing host can compute final hue/brightness/mode, while
firmware applies calibration, gamma, global/electrical caps and scheduling.

## Optional identities

Proposed input controls are `parameter.1.push` through `parameter.8.push`, with
provisional IDs `0x0160..0x0167`. They are buttons related to, but independent
from, existing rotations `parameter.1..8`. They have press/release state, no
implicit action, and are unassigned by default. These numbers are proposed for
upstream review, not production constants.

Encoder RGB uses existing `parameter.1..8` wire IDs only inside the LED_RENDER
destination namespace. Input message type distinguishes rotation from feedback
destination, so new destination numbers add no information. Firmware maps each
logical destination to physical P1-P8 and then through the manifest's
nonsequential TLC channel map.

## Inventory and deterministic fingerprints

The six-byte baseline INVENTORY prefix and `0x3256434e` never change. In 1.4,
append one fixed eight-byte record for each negotiated optional namespace:
`namespace:u8, count:u8, entry_size:u8=0, reserved:u8=0, fingerprint:u32`.
Canonical record order is push then encoder RGB. A capability without its exact
record is incompatible; an unadvertised record is malformed.

Because upstream has no baseline fingerprint algorithm, optional inventories
provisionally define CRC-32/ISO-HDLC over canonical entries, each encoded as
UTF-8 identity, NUL, little-endian `u16` ID, and `u8` kind. This produces
provisional push fingerprint `0x1936cd35` and encoder-RGB fingerprint
`0x6b7c76c7`. Upstream may approve another explicit algorithm/value without
altering the baseline constant. Canonical bytes and vectors live in
`protocol/fixtures/proposed_extension_v1.json`.

## Events and authoritative state

Optional pushes reuse BUTTON exactly: generation, proposed ID, pressed. They
share sender sequence and host generation behavior with baseline buttons.
Firmware emits none unless push capability and inventory are both accepted.
Duplicates, gaps, old generations and unsupported IDs follow baseline failure
behavior.

INPUT_STATE retains generation plus the canonical nine baseline bytes. When and
only when push capability is negotiated, append one P1..P8 bitmap byte, making
14 payload/26 framed bytes. Do not consume unused encoder-position bits in the
baseline bitmap. Held optional pushes are quarantined exactly like baseline
buttons; snapshots reconcile state without generating musical presses.

## Encoder rendering ownership

LED_RENDER already covers required output. Host sends final functional or
operational RGB, final perceptual brightness, mode and period. For assigned
zero/unipolar minimum or bipolar center, host applies its chosen minimum visible
brightness once. Unassigned, unavailable and explicit off use mode off and zero
brightness. Selected/recording/queued overrides are host-computed. Firmware
does not apply an additional semantic minimum or operational recolor in
production mode; it performs interpolation, calibration, gamma and final
combined current limiting. This prevents double gamma, double minimum and
competing precedence.

Steady/dim/blink/pulse remain local monotonic effects. A new packet replaces the
previous generation for that destination. Musical beat phase remains host-owned
and would require explicit future synchronization, not an MCU musical clock.
Disconnect, RESET, heartbeat timeout or new negotiation cancels animation and
stale pending frames before accepting the new full refresh.

## Sizes and bandwidth

Frame overhead is 12 bytes. Baseline inventory is 18 framed bytes; both optional
records make it 34. Baseline snapshot is 25; push snapshot is 26. One optional
push event is 19. One LED_RENDER is 22, so eight encoder updates are 176 bytes.
A complete 58-addressable refresh is 1,276 bytes; adding all eight encoders is
1,452 bytes. Reconnect control traffic excluding variable HELLO_REPLY is 97
bytes baseline or 114 bytes with both inventories/push state; HELLO_REPLY is
`16 + device_id_length + firmware_length` framed bytes (50 plus firmware length
for the 34-byte V2 identity).

At a real 115200-baud 8N1 UART, 1,452 bytes is roughly 126 ms. USB CDC is a USB
byte stream and is not throttled by the host's nominal line-coding baud value.
Regardless of transport, 256-byte payloads, fixed queues, coalesced feedback and
latest-state convergence remain mandatory.

## Failure and safety rules

Bad magic/major/length/CRC is rejected and counted. Unsupported message types
and unknown capabilities are ignored/countable, never enabled. Bad identity,
mandatory inventory or optional advertised inventory fails compatibility.
Invalid control/destination IDs are rejected. Gaps trigger reconciliation;
duplicates and stale generations are dropped. Queue overflow exposes counters:
input overflow requests a snapshot, feedback overflow retains newest desired
state and requests a full refresh.

USB loss/host crash cancels session ownership on heartbeat timeout. All old
binary RX/TX and rendering queues are discarded before a new session. Protocol
success never proves the external 5 V rail exists. Every feedback command still
passes the 900 mA modeled policy, calibration and physical driver safeguards.

## USB mode separation

Keep the current text diagnostics as a permanent `diagnostic` firmware target
and add a later `production` target containing only binary protocol traffic.
Separate build profiles are the smallest unambiguous first implementation and
avoid composite-USB complexity. A future composite device may use separate CDC
interfaces with distinct descriptors, but unsolicited text must never share
the production binary interface. No full production endpoint is implemented in
HW-006A.

