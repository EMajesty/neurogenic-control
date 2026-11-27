# Protocol 1.3 baseline audit

Normative source consulted: `EMajesty/neurogenic` `main` at
`5f94323acff8a78059c05a4e1e508a6a7e0ced44` (2026-11-27). The audit read the
wire header, protocol/session implementations, Reference V2 descriptor,
simulator, tests, and requested documentation at that revision. The main
repository remains the normative contract owner; this repository did not
modify it.

## Locked envelope and inventory

Protocol is major 1, minor 3. Every integer is little-endian. A frame is
`magic:u16=0x474e, major:u8, type:u8, payload_length:u16, sequence:u32,
payload, crc:u16`. CRC is CRC-16/CCITT-FALSE: polynomial `0x1021`, initial
`0xffff`, no reflection, no final XOR, over every byte preceding the CRC.
Payload is at most 256 bytes and the complete frame at most 268 bytes. The
stream decoder has bounded storage, retains partial frames, skips bytes until
magic, and rejects the wrong major, oversized length, or bad CRC.

The mandatory V2 inventory is exactly 67 controls: 58 buttons and nine relative
encoders. Its literal locked fingerprint is `0x3256434e`. No algorithm that
derives that value from manifest entries exists in the audited source; it is a
canonical constant (little-endian bytes spell `NCV2`), so an extension must not
claim to reproduce an undocumented baseline hash algorithm.

Stable IDs are unchanged:

| Range | Logical inventory |
|---|---|
| `0x0100..0x010f` | `pad.1..pad.16` |
| `0x0110..0x0117` | `page.1..page.8` |
| `0x0118..0x011f` | `function.1..function.8` |
| `0x0120..0x0127` | relative `parameter.1..parameter.8` |
| `0x0128` | relative `data` |
| `0x0130..0x0135` | back, up, enter, left, down, right |
| `0x0140..0x0149` | ten edit buttons |
| `0x0150..0x0159` | ten transport buttons |

The canonical manifest order—not numeric wire-ID order alone—is pads, pages,
functions, eight parameter encoders, DATA, navigation, edit, transport.

## Messages and payloads

Message IDs are HELLO 1, HELLO_REPLY 2, CAPABILITIES 3, BUTTON 4, ENCODER 5,
ANALOG 6, LED 7, CONTEXT 8, STATE_SYNC 9, HEARTBEAT 10, ERROR 11, RESET 12,
INVENTORY 13, INPUT_STATE 14, LED_RENDER 15, DIAGNOSTIC 16.

- HELLO is host `{major:u8, highest_minor:u8}`.
- HELLO_REPLY is `{major, selected_minor, id_length, firmware_length, bytes}`.
- CAPABILITIES is an empty request and a four-byte mask response. Existing
  bits 0..10 are buttons, relative encoders, analog, RGB, context feedback,
  heartbeat, legacy inventory, V2 inventory, input state, LED render, and
  diagnostics.
- INVENTORY response is exactly `{count:u16=67, fingerprint:u32=0x3256434e}`.
- STATE_SYNC 1.3 is `{host_input_generation:u32}`.
- INPUT_STATE is generation plus nine bitmap bytes: 13 payload bytes and 25
  framed bytes. Bits index all 67 canonical manifest entries; encoder slots are
  unused. Current validators require the exact length, so those unused bits are
  not extension space.
- BUTTON is generation, wire ID, pressed: 7 payload/19 framed bytes.
- ENCODER is generation, wire ID, signed delta: 8 payload/20 framed bytes.
- LED_RENDER is wire ID, `0x00RRGGBB`, brightness, mode, period: 10 payload/22
  framed bytes. Modes are off, steady, dim, blink, pulse.
- HEARTBEAT normally carries opaque `monotonic_ms:u64`; the host echoes it.
- RESET restarts negotiation.

The frame sequence is sender-owned and covers all messages. For synchronized
input, the host additionally requires the current generation. A duplicate
input sequence is rejected. A gap increments diagnostics, reconnects the
session (cancelling gesture ownership), issues a new generation snapshot
request, and rejects the triggering event. Old-generation input is rejected.

## Negotiation and reconciliation

The host starts by clearing output/session state, incrementing a nonzero input
generation, and sending HELLO. It rejects a different major, a minor newer than
its maximum, a configured identity mismatch, malformed lengths, missing V2
inventory/input-state capabilities, or incorrect inventory. After inventory
validation it creates the Reference V2 session, sends STATE_SYNC, and accepts no
musical input until an exact-generation INPUT_STATE arrives.

Snapshot bits only quarantine held buttons. A snapshot never synthesizes a
press. A quarantined control's first release clears quarantine without musical
dispatch. Session reconnect cancels held modifiers/actions and pending semantic
ownership. Disconnect clears host output and session state; RESET performs a
new negotiation.

## Feedback and failure behavior

The host owns musical semantics, context, normalization and operational color.
LED_RENDER already supports arbitrary RGB, perceptual brightness, explicit off,
dim, blink, pulse and firmware-default or explicit period. It has no host
generation field. Ordered USB plus connection-local queues provide the epoch:
both endpoints must discard queued RX/TX and renderer targets when negotiation
restarts. Heartbeat loss is the bounded process-crash detector.

Host output/session queues are bounded. Backpressure leaves the feedback
revision incomplete and retries until the latest authoritative projection
converges; reconnect forces a full refresh. Unknown control IDs do not dispatch.
Malformed payloads fail the message. CRC/length/major failures are decoder
errors. Unsupported message-type handling is currently permissive at the host
receive switch boundary (no semantic action); firmware should count and ignore
it rather than invent behavior.

The baseline simulator uses production encode/decode, host negotiation,
generation, snapshot, semantic session and feedback paths. Upstream tests cover
the golden button frame, CRC rejection, negotiation, lost-release recovery,
LED_RENDER publication, reconnect generation, stale input, heartbeat and
bounded-feedback convergence. That is software evidence, not physical USB
interoperability.

