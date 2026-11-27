# NEUROGENIC reference controller

Manufactured hardware authority and RP2040 firmware for the NEUROGENIC reference
surface. Start with the [hardware revision](docs/HARDWARE_REVISION.md),
[manifest](docs/HARDWARE_MANIFEST.md), [scanner](docs/INPUT_SCANNING.md),
[harness](docs/HARNESS_CONFIGURATION.md), and [diagnostics](docs/DIAGNOSTICS.md).

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
```

Illumination is disabled, the harness is an unapproved example, and physical
validation has not occurred.
