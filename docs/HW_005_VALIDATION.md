# HW-005 validation

HW-005 adds diagnostic interface version 1, content-addressed build identities,
bounded raw/inventory/timing/stress commands, a 30-second illumination session
watchdog, bounded TLC admission, deterministic acceptance/report helpers and a
guided Linux CLI. Firmware version is 0.4.0; production protocol remains 1.3.

Validation performed on 2026-11-27 without physical boards:

- generated manifest, harness and calibration headers: freshness checked;
- native Release build and three CTest targets: 3/3 passed; the two Python
  targets contain 11 deterministic unit tests;
- simulated workflow coverage: identity/version rejection, timeout,
  disconnection, safe termination, missing release, unexpected transition,
  report status/export, harness and encoder proposals: passed;
- fresh KiCad 10.0.8 exports for all six schematics: component-pin nets agreed
  with resolved PCB pad nets and the manifest verifier passed;
- ASan/UBSan native build and CTest: 3/3 passed;
- `nix flake check --no-build`: passed for x86_64-linux; aarch64-linux was
  reported incompatible with the current evaluation system and skipped;
- RP2040 Pico SDK 2.1.1 Release cross-build: passed; ELF, UF2, BIN, HEX and map
  generated under `build/hw005-pico`. Pre-commit size was text 124,220, data 0,
  BSS 5,304 (129,524 total), UF2 248,832 bytes. The clean-commit artifact hash
  is intentionally reported in the completion report because source identity
  changes when the task commit is created;
- physical execution, electrical measurements, optical checks, scan jitter,
  serial timing, current, droop and temperature: **NOT RUN**.

The final exact command results, artifact size/SHA-256 and memory usage are
reported after the clean final build. Build directories are ignored and not
committed. Fresh KiCad connectivity verification is not repeated because HW-005
does not change hardware sources; the manifest verifier remains authoritative.
