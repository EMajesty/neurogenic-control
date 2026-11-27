# HW-006C firmware implementation handoff

Do not begin production protocol implementation until HW-006B commits the
normative upstream header and golden vectors. Then update the verified vendor
pin and retain the upstream MIT provenance.

## Firmware work

- Add a separate `production` firmware target using bounded binary framing;
  retain the current text-only `diagnostic` target. Never multiplex unsolicited
  text into production frames.
- Implement mutual minor selection and capability/inventory validation. Exact
  1.3 mode publishes only the 67-control baseline and exact baseline snapshot.
- Map physical P1-P8 pushes to approved optional IDs only after push
  negotiation. Suppress them otherwise. Keep the DATA shaft push mapped once
  to existing navigation Enter.
- Append optional push snapshot state only in the approved mode. Reuse the
  scanner's authoritative current state on boot, overflow, gaps and reconnect;
  never synthesize musical presses.
- Accept encoder LED_RENDER destinations only when encoder-RGB capability and
  inventory are negotiated. Map logical P1-P8 through the generated manifest,
  not TLC channel order.
- Make session replacement clear parser, TX, event, renderer, animation and
  pending driver state before a full refresh. Reject stale generation input;
  honor host generation ownership and bounded sequence handling.
- Use fixed buffers and latest-state/coalescing semantics. An input overflow
  reports the condition and requires reconciliation. Feedback overflow retains
  the newest authoritative target.
- Feed every production LED target through existing calibration, gamma, global
  brightness, combined 900 mA modeled limiter and safe driver scheduling.
  Negotiation never implies external 5 V presence or bypasses diagnostic
  illumination policy.

## Conformance gate

Run upstream normative golden vectors through the actual firmware codec and
run the firmware-produced vectors through upstream tests. Cover 1.3 fallback,
each capability independently, both capabilities, wrong inventories, unknown
bits, optional events, held-on-connect snapshots, overflow, gaps, reconnect,
stale feedback and maximum payloads. USB interoperability, physical timing and
illumination safety remain separate hardware acceptance gates.

