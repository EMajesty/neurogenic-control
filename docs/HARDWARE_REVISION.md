# Frozen hardware revision

HW-001 freezes the manufactured design baseline at superproject commit
`dfa14009539741f9b325898556ced2ca027bf737`. This is a hardware-design revision,
not firmware version `0.1.0` and not USB controller protocol `1.3`.

| Board | KiCad project | Fabrication artifact |
|---|---|---|
| brain | `hardware/brain/brain.kicad_{pro,sch,pcb}` | `hardware/brain/gerbers/gerbers.zip` |
| buttons 1×8 | `hardware/buttons-1x8/buttons-1x8.kicad_{pro,sch,pcb}` | `hardware/buttons-1x8/gerbers/gerbers.zip` |
| buttons 2×5 | `hardware/buttons-2x5/buttons-2x5.kicad_{pro,sch,pcb}` | `hardware/buttons-2x5/gerbers/gerbers.zip` |
| encoders | `hardware/encoders/encoders.kicad_{pro,sch,pcb}` | `hardware/encoders/gerbers/gerbers.zip` |
| navigation | `hardware/nav/nav.kicad_{pro,sch,pcb}` | `hardware/nav/gerbers/gerbers.zip` |
| pads | `hardware/pads/pads.kicad_{pro,sch,pcb}` | `hardware/pads/gerbers/gerbers.zip` |

Each directory contains copper, mask, paste, silkscreen, outline, PTH/NPTH drill
and `.gbrjob` files. The archives and `.kicad_pro` plot settings are the only
manufacturing configuration recorded here; vendor order parameters are not in
the repository and are therefore not claimed. Footprints are pinned by the
superproject to `keyswitches.pretty` `49083b8f0d1c8a68a1b5c4ac9bc81b3870e9aa68`
and `majestic-kicad` `213406a2a5eee1461b943cc5407b2bef8c7c6dec`.

The panel inventory is 58 buttons (42 non-pad and 16 external arcade pads),
nine rotary encoders, and eight optional contextual-encoder push switches.
The 16 pad switches connect separately through `pads.J2`; their separately
wired addressable LEDs are assumed to connect to `pads.J3` (+5 V, DIN, GND).
The PCB does not establish the external LED part, cable order, pixel order, or
switch/LED pairing. Inspect all four during bring-up.

Protocol 1.3 describes 67 controls: 58 buttons plus nine non-push encoders. It
does not describe the eight wired P1–P8 pushes or dedicated encoder RGB
destinations. They remain diagnostic-only/unreported in compatible baseline
mode. Never alias them to existing controls.

No manufactured assembly has been electrically validated. Verify cable keying,
the `MOD1`–`MOD8` harness assignment, arcade wiring, encoder direction, active
levels, RGB order, AHCT buffer direction, regulator polarity/output, rail
resistance and current, TLC current, USB behavior, and every connector before
normal operation. The committed design includes three optional analog returns;
their intended peripherals are not established.
