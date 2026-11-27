# Manufactured-board acceptance

This workflow is standalone: it needs the RP2040 diagnostic firmware, a Linux
serial terminal or `tools/neurogenic_diag.py`, a current-limited supply and the
operator's instruments. It does not need the NEUROGENIC application. No stage
is accepted by this repository; manufactured hardware remains **NOT RUN** until
an operator records evidence.

## Quick start

```sh
nix develop
cmake -S firmware -B build/pico -G Ninja -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD=pico
cmake --build build/pico
picotool info build/pico/neurogenic_controller.uf2
# Hold BOOTSEL, connect the Pico, copy the UF2 to RPI-RP2, then:
python3 tools/neurogenic_diag.py info --device /dev/ttyACM0
python3 tools/neurogenic_diag.py bringup --report acceptance.json
python3 tools/neurogenic_diag.py report --report acceptance.json --output acceptance.md
```

The serial path is always explicit. Resume by invoking `bringup` with the same
report. It advances only the first `NOT RUN` stage and will record a refused
safety prerequisite as `BLOCKED`. A plain terminal can send `help`, `info`,
`inventory`, `module N`, `raw`, `buttons`, `encoders`, and `stats`.

## Ordered stages and stop conditions

Power down before changing a cable. Never hot-plug an unvalidated module.
Every stage stops immediately for unexpected current, current-limit operation,
incorrect polarity, rail collapse, heating, smell, smoke, repeated USB reset,
unstable scan statistics, or any observation that conflicts with the expected
power path.

1. Inspect the brain board and connectors against the manufactured design.
2. With no external LED power, check shorts and intended rails.
3. Boot by USB only; `info` must enumerate a diagnostic-v1 identity. Do not
   infer anything about the unpowered AHCT125/LED rail.
4. Apply the intended external supply through the documented 12 V input using
   a current limit. Do not directly inject 5 V until the power path has been
   reviewed for backfeeding. Record 12 V, 5 V, 3.3 V and idle current.
5. With power removed between changes, test one 1x8, one 2x5, navigation,
   encoder and pad board. Use `module N`, raw bits, decoded transitions and
   operator-confirmed physical identities. A constant lane is `UNKNOWN`, not
   proof of presence or absence.
6. Assemble the powered-off complete harness. Confirm every bit and propose,
   but do not automatically install, a harness file.
7. After input acceptance, enable illumination explicitly on a current-limited
   supply. Walk 58 addressable pixels, then 24 TLC outputs and eight mapped
   encoder RGB destinations. A transmitted frame is not optical readback.
8. Follow the calibration guide, recording identity gains until measured
   proposals exist. Every pattern remains under the common 900 mA model.
9. Run a bounded stress session and record before/after scan, quadrature,
   contention, overflow and driver counters. Simulation success is not target
   timing evidence.
10. Review every `NOT RUN`, `UNKNOWN`, `BLOCKED` and `FAIL` before setting a
    final result. Missing evidence never becomes `PASS`.

## Guided control checks

For each button, identify the manifest-derived port/bit, press and release it,
and record missing, duplicate or simultaneous unexpected transitions. Repeat
with two held controls and release them in both orders to expose coupling.
For every encoder record initial A/B, slow clockwise and counterclockwise raw
transitions, reversal, faster rotation and invalid jumps. Test contextual push
switches and rotate while held where the mechanism allows. DATA calibration
uses a counted number of physical detents; contextual detentless encoders use a
known rotation extent. Exported inversion/count values are proposals only.

## Standalone profiles

`neurogenic_diag.py harness --profile PROFILE --output proposed.json` supports
`brain-only`, `one-1x8`, `one-2x5`, `navigation`, `encoder`, `pad`, and
`complete-reference`. Chain lengths are derived from `hardware/manifest.json`.
These files require operator confirmation, schema validation, regeneration and
a deliberate rebuild; runtime diagnostics never write flash configuration.

Encoder and calibration proposals are similarly explicit:

```sh
python3 tools/neurogenic_diag.py encoders --encoder DATA --cw-raw 40 \
  --ccw-raw -40 --known-steps 10 --output proposed-encoder.json
python3 tools/neurogenic_diag.py calibrate \
  --calibration-source hardware/illumination_calibration.json \
  --output proposed-calibration.json
```

The counts are operator-collected evidence, not defaults; exported files carry
source fingerprints and a review-required status.

## Safety limits

Illumination starts disabled. `led enable` is an operator assertion that the
external rail is valid. A 30-second command/heartbeat session watchdog blanks
TLC and queues addressable all-off; bounded stress mode keeps its own session
alive until completion and then queues all-off. Firmware has no 5 V sense.
Reset or USB loss cannot guarantee clearing latched SK6812 pixels, and software
average-current limiting does not bound PWM peaks, inrush or temperature.
Remove power during a fault.
