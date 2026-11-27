# HW-004 validation

Baseline: HW-003 `76f23dcb11110ce7aac6b293f414de1594931eeb`. Upstream visual-language revision: `ae66cee8b526cd999c2e957ecf1302080a92aaff`. Protocol remains 1.3; no new capability or application repository change was made.

Portable acceptance covers palette preservation, unipolar endpoints, bipolar center/symmetry, boolean/enumerated behavior, assigned minimum, unassigned/unavailable/off, operational overrides, fixed-point calibration validation, gamma monotonicity/endpoints, interpolation/replacement, temporal wrap, all-off, combined conservative limiting, zero budget, generated mappings, SK ownership, TLC 288-bit/one-latch ordering, scanner/debounce/quadrature regressions and bounded parser inputs.

Executed results: manifest verifier PASS for six PCB netlists; three generated headers fresh; Nix flake check PASS on x86_64-linux (aarch64-linux evaluation skipped by the local system); sanitizer-enabled native build PASS; CTest 2/2 PASS (`host_tests`, `diagnostic_utility`); Pico SDK 2.1.1 RP2040 Release cross-build PASS. GNU size is text 117,196, data 0, BSS 4,432, total 121,628 bytes. Artifacts are `/tmp/ng-hw004-pico/neurogenic_controller.{elf,uf2,bin}` and `neurogenic_controller.elf.map`. UF2 size is 234,496 bytes and SHA-256 is `0f26ef06ae7703eca987824d44e1161969019a5d33d01df217779e44f6fd72a2`.

Physical optical calibration, current, voltage droop, regulator temperature, SK/TLC waveforms, TLC transaction duration and input scan jitter remain **NOT RUN**. The hardware was unavailable; defaults are not measured calibration.

HW-005 should add guided board-by-board bring-up, interactive harness/chain confirmation, direction/detent calibration, combined input/illumination stress, structured report export, build identity and a repeatable manufactured-board checklist without changing protocol semantics.
