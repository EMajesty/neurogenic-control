# HW-001 validation record

Software evidence is produced by `scripts/test.sh`: manifest/schema invariants,
resolved PCB pad connectivity, chain/bit/cascade order, identity uniqueness,
TLC mapping, quadrature transitions/errors, exclusive shared-bus sequencing,
the 288-clock frame and single latch, gamma boundaries, current budgeting and
conservative startup defaults.

Run on 2026-11-27 in the development workspace: all six resolved PCB netlists
passed the manifest verifier and the single aggregated CTest executable passed.
Clang 21.1.8 built the host suite. `kicad-cli`, `arm-none-eabi-gcc`, and
`PICO_SDK_PATH` were absent, so fresh schematic export and the Pico/UF2 build
were **NOT RUN**, not passed. The exact completion commands are in the README
and manifest guide.

KiCad CLI schematic export, Pico cross-build, UF2 production and all physical
tests are environment-dependent and must be reported from the actual run. A
successful host test is not electrical validation. The one intentionally open
mapping is physical cable instance to `brain.J1`…`J8`.

HW-002 should implement the board GPIO backend and simultaneous nine-return
scanner; fixed debounce/reconciliation buffers; DATA and P1–P8 quadrature
polarity calibration; raw CDC diagnostic snapshot/transition commands; module
presence heuristics; scan timing/error counters; and manufactured-board
bring-up evidence. It must retain protocol-compatible suppression of P pushes
and must not implement production LED drivers.
