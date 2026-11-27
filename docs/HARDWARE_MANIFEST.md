# Hardware manifest

[`hardware/manifest.json`](../hardware/manifest.json) is the typed, stable
machine-readable board-support description. Names are always board-qualified
when reference designators repeat. Register `bits[0]` is the first bit observed
at Q7 after `/PL` rises; the array follows serial transfer order, not schematic
placement. `serial_out` identifies the MCU-facing end of every chain.

The brain uses GPIO14 `SCAN_CLK`, GPIO15 `SCAN_LOAD`, GPIO6–13 for module
returns 8–1, GPIO20 for the encoder return, GPIO21 TLC data, GPIO18 latch,
GPIO19 blank, and GPIO22 through enabled `brain.U1` (74AHCT125) and 220 Ω to the
first addressable RGB input. GPIO26–28 are optional analog returns. All module
connectors carry GND, 3.3 V logic, clock, load, independent serial return, RGB
in/out, another GND and 5 V illumination. The encoder connector substitutes
TLC data/latch/blank for RGB in/optional/out.

The manifest records each 74HC165 cascade and pin-derived bit order, all
encoder A/B/push nets, DATA A/B/push, all Choc switch nets, external arcade
nets, addressable chains, and the 24 TLC outputs. TLC `SOUT` is unconnected;
the driver consumes exactly 24 × 12 = 288 bits and latches only afterward.
`SCAN_CLK` is shared between all 165s and TLC `SCLK`.

Run `python3 tools/verify_manifest.py`. It parses balanced KiCad PCB S-expressions
and checks resolved footprint pad nets, rather than performing textual
substitution. A future machine with KiCad 9 CLI should additionally run:

```sh
mkdir -p build/netlists
for b in brain buttons-1x8 buttons-2x5 encoders nav pads; do
  kicad-cli sch export netlist --output "build/netlists/$b.net" "hardware/$b/$b.kicad_sch"
done
python3 tools/verify_manifest.py
```

For a fresh schematic-to-PCB component-pin comparison:

```sh
nix develop .#hardware --command scripts/verify_schematics.sh
```

The current verifier validates committed resolved PCB netlists. Fresh schematic
export is explicitly not verified in the present environment. Harness instance
assignment is also `UNVERIFIED_HARNESS_ASSIGNMENT`; the repeated board designs
cannot prove which physical cable is connected to a brain port.

`tools/generate_manifest_header.py` schema-validates this JSON and generates
the C++ header deterministically; CMake fails stale output. Harness generation
is separate so board design never implies a panel instance. Encoder pushes are
active-high after the 15 kΩ/10 kΩ divider, correcting the initial HW-001 prose
after tracing switch/divider connectivity.
