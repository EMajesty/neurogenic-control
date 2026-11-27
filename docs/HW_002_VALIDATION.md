# HW-002 validation

Validated 2026-11-27 without physical hardware:

- Manifest and both generated headers passed freshness/schema checks.
- Native and ASan/UBSan builds each passed 2/2 CTest tests.
- Deterministic tests cover initial Q7, mixed 8/16/24-bit lanes, masking,
  contention, active-low/high debounce, simultaneous transitions, startup
  baseline, quadrature reversal/invalid jumps, queue overflow and parser limits.
- `nix flake check --no-build` evaluated x86_64-linux shells; aarch64-linux was
  declared but not evaluated on this host.
- Pico SDK 2.1.1 commit `bddd20f928ce76142793bef434d4f75f4af6e433`
  and Arm GNU 15.3.1 produced ELF, BIN, HEX, map and an actual UF2.
- Final validated size: text 46,456 bytes, data 0, BSS 4,396, total 50,852
  bytes. UF2 SHA-256 is `fe3dd188b7b08ec7331442fe6dd6e680e95a41f3a5a7d6729c84c8a632d9ca2b`.

Artifacts are ignored under `build/pico/neurogenic_controller.{elf,bin,hex,uf2}`
and `.elf.map`; final UF2 size is 93,184 bytes. KiCad CLI 10.0.8 freshly exported
all six schematic netlists, and component-pin connectivity agreed with resolved
PCB nets. Reproduce with `nix develop .#hardware --command scripts/verify_schematics.sh`.
Physical USB, scan timing, polarity, bounce,
detents, presence, electrical measurements and manufactured boards are all
**NOT RUN**.

HW-003: SK6812 PIO, TLC grayscale serialization, startup blanking, generated
RGB chains, shared-clock recovery, current limiting and diagnostic channel/pixel
walking—without changing input identity or protocol 1.3.
