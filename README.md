# NEUROGENIC reference controller

Manufactured hardware authority and RP2040 firmware for the NEUROGENIC reference
surface. Start with the [hardware revision](docs/HARDWARE_REVISION.md),
[manifest](docs/HARDWARE_MANIFEST.md), [scanner](docs/INPUT_SCANNING.md),
[harness](docs/HARNESS_CONFIGURATION.md), and [diagnostics](docs/DIAGNOSTICS.md).
Manufactured-board work starts with the [acceptance quick start](docs/HARDWARE_ACCEPTANCE.md);
its structured output is defined by the [report format](docs/DIAGNOSTIC_REPORT_FORMAT.md).

## Reproducible development and tests

```sh
nix develop
scripts/test.sh
cmake -S . -B build/asan -DNEUROGENIC_SANITIZERS=ON
cmake --build build/asan
ctest --test-dir build/asan --output-on-failure
```

The lock pins nixpkgs and Pico SDK 2.1.1 with TinyUSB. The optional heavy KiCad
shell is `nix develop .#hardware`.

## Pico build and flash

```sh
nix develop
cmake -S firmware -B build/pico -G Ninja -DPICO_BOARD=pico
cmake --build build/pico
arm-none-eabi-size build/pico/neurogenic_controller.elf
picotool info build/pico/neurogenic_controller.uf2
```

Outputs are `build/pico/neurogenic_controller.elf`, `.uf2`, `.bin`, `.hex`, and
`.elf.map`. Hold BOOTSEL while connecting the original Pico and copy the UF2 to
`RPI-RP2`. Build directories are ignored.

```sh
python3 tools/neurogenic_diag.py --device /dev/ttyACM0 --command version
python3 tools/neurogenic_diag.py info --device /dev/ttyACM0
python3 tools/neurogenic_diag.py bringup --report acceptance.json
```

Illumination is disabled at boot, the harness is an unapproved example, and physical validation has not occurred. HW-003 adds PIO/DMA SK6812 and shared-clock TLC5947 drivers behind an explicit diagnostic enable and the combined 900 mA model. See [illumination drivers](docs/ILLUMINATION_DRIVERS.md), [power budget](docs/ILLUMINATION_POWER_BUDGET.md), and [HW-003 validation](docs/HW_003_VALIDATION.md).

```sh
nix develop
cmake -S firmware -B build/pico -G Ninja
cmake --build build/pico
sha256sum build/pico/neurogenic_controller.uf2
```

HW-004 adds the bounded semantic renderer and generated uncalibrated defaults. See [rendering](docs/ILLUMINATION_RENDERING.md), [calibration](docs/ILLUMINATION_CALIBRATION.md), and [validation](docs/HW_004_VALIDATION.md). Protocol remains 1.3 and physical calibration is not claimed.

HW-005 adds diagnostic-v1 identity/fingerprints, guided board acceptance,
standalone harness profiles, bounded stress/timing diagnostics and versioned
JSON/Markdown reports. Physical acceptance remains NOT RUN. See
[HW-005 validation](docs/HW_005_VALIDATION.md) and the source-grounded
[HW-006 proposal](docs/HW_006_PROTOCOL_PROPOSAL.md).

HW-006A pins and audits the normative upstream 1.3 contract and adds portable
golden-vector conformance tests without enabling a production endpoint:

```sh
python3 tools/verify_upstream_contract.py
python3 tools/generate_protocol_fixture_header.py --check
cmake -S . -B build/host -G Ninja
cmake --build build/host
ctest --test-dir build/host --output-on-failure
```

The provisional extension, compatibility matrix, and implementation handoffs
are in [the extension specification](docs/PROTOCOL_EXTENSION_SPEC.md),
[compatibility matrix](docs/PROTOCOL_COMPATIBILITY_MATRIX.md),
[HW-006B handoff](docs/HW_006B_HOST_HANDOFF.md), and
[HW-006C handoff](docs/HW_006C_FIRMWARE_HANDOFF.md). Extension identifiers are
not authoritative until committed by the main NEUROGENIC repository.
