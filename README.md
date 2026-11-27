# NEUROGENIC reference controller

This repository contains the manufactured hardware design authority and the
RP2040 firmware foundation for the NEUROGENIC reference control surface. The
six immutable board projects live under `hardware/`; documentation starts at
[`docs/HARDWARE_REVISION.md`](docs/HARDWARE_REVISION.md), and firmware is under
`firmware/`. The application and normative controller protocol live in the
separate [NEUROGENIC repository](https://github.com/EMajesty/neurogenic).

## Host validation

Linux needs Python 3, CMake, a C++17 compiler and CTest:

```sh
scripts/test.sh
```

On NixOS: `nix-shell -p python3 cmake gcc --run scripts/test.sh`.

## Pico build and UF2

Use the explicitly versioned Pico SDK release `2.1.1` (or a checkout whose SHA
you record alongside build evidence):

```sh
git clone --branch 2.1.1 --depth 1 https://github.com/raspberrypi/pico-sdk.git
git -C pico-sdk submodule update --init
PICO_SDK_PATH="$PWD/pico-sdk" cmake -S firmware -B build/pico \
  -DNEUROGENIC_PICO=ON -DPICO_BOARD=pico
cmake --build build/pico
```

The Pico SDK emits `neurogenic_controller.uf2`. Hold BOOTSEL while connecting
the Pico and copy the UF2 to the `RPI-RP2` mass-storage volume. A CDC terminal
at any nominal baud can send `i` for identity or `h` for help; USB CDC ignores
the line baud. No desktop UI, controller hardware or audio system is needed to
build.

See the [manifest guide](docs/HARDWARE_MANIFEST.md), [architecture](docs/FIRMWARE_ARCHITECTURE.md),
[protocol status](docs/PROTOCOL_COMPATIBILITY.md), and [bring-up plan](docs/BRINGUP_PLAN.md).
