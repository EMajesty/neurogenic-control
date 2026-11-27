# HW-003 validation

Baseline: HW-002 commit `3db026774fbef6c2b9802a99cfeec42611a24c06`; manufactured connectivity baseline remains the SHA recorded in `HARDWARE_REVISION.md`.

## Executed

- Generated manifest header freshness: PASS. TLC RGB mapping is now generated from the manifest.
- Native ASan/UBSan build and CTest: PASS, 2/2 tests (`host_tests`, `diagnostic_utility`).
- RP2040 Release cross-build in the locked Nix shell: PASS. GNU size: text 107,688 bytes, data 0, BSS 4,432, total 112,120. Versus HW-002: +61,232 text and +36 BSS; the increase includes PIO/DMA support, illumination state/serialization and expanded diagnostics.
- Outputs: `/tmp/ng-hw003-pico2/neurogenic_controller.elf` (1,001,844 bytes), `.uf2` (215,552 bytes), `.bin` (107,692 bytes), and `neurogenic_controller.elf.map` (650,504 bytes; validation workspace; normal documented build uses `build/pico`). UF2 SHA-256: `c44892d77d2d0eb191c7696df97d4ee360f268824775ce8141513039d860e312`.
- Pico SDK 2.1.1 `pioasm` needed `-include cstdint` with the pinned Nixpkgs GCC 16; the flake now supplies that deterministic compatibility flag.

Host coverage includes GRB serialization, 58-pixel frame length, pending-frame replacement, TLC OUT23-to-OUT0 ordering and 288-bit length, generated encoder mappings, gamma/current boundaries, startup all-off, input scanner regression, bus contention and bounded diagnostic validation. The RP2040 target compiled the same portable serializers.

## Not run

Physical LED output, actual cable order, arcade LED wiring, LED-rail behavior under USB-only power, PIO waveform timing, TLC clock/setup/hold/XLAT timing, scan jitter during updates, current, supply transient and temperature validation are **NOT RUN**. The boards were not available. No electrical validation is claimed.

## HW-004 recommendation

Measure every physical RGB channel and chain position; calibrate per driver/channel; verify minimum visible output; implement encoder hue/magnitude, bipolar magnitude and operational-state rendering; add smooth bounded transitions; tune the current model using measured installed parts, supply droop and temperature; retain the diagnostic calibration workflow and safety ceiling.
