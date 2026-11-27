# Illumination calibration

`hardware/illumination_calibration.json` is the versioned prototype configuration. `tools/generate_calibration_header.py` validates it and deterministically generates firmware defaults. There are no runtime flash writes, wear, atomicity or recovery concerns in HW-004. Diagnostic changes are volatile; `led calibration export` prints values for later review and deliberate JSON update.

Defaults are explicitly **uncalibrated** identity gains: Q8 256 for RGB at each driver, encoder and pixel; quadratic gamma; global ceiling 128/255; minimum assigned 31/255; transition 40 ms. Gains accept 0..512 (0..2×), but final PWM always passes through the 900 mA limiter. Malformed schema, gain, ceiling, minimum, duration or gamma values fail validation.

The conservative uncharacterized model is 20 mA per SK6812 color channel, 1 mA SK6812 quiescent, and 10.5 mA per TLC5947 channel with the fitted 4.7 kOhm IREF. It is an average-duty estimate, not a bound on instantaneous peaks, inrush, droop or temperature.

## Eventual physical procedure

1. With power off, connect the verified harness. Use a current-limited external 5 V supply and confirm rail polarity/voltage.
2. Enable illumination conservatively and verify one addressable LED, then one encoder RGB output.
3. Walk every pixel and TLC channel; record chain order and RGB swaps without rewriting verified mappings from a single observation.
4. Determine minimum visible output in controlled ambient light.
5. Compare R/G/B primaries and representative functional colors across both driver families.
6. Adjust bounded driver gains, then only if needed per-encoder/per-pixel gains. Export results.
7. Measure current for representative, mixed and limited-white patterns; measure voltage at the regulator, brain and farthest module.
8. Measure regulator temperature after a controlled soak.
9. Scope SK timing/reset, TLC transaction duration/XLAT, and input scan jitter during concurrent animation.
10. Record part sources, instruments, supply limit, ambient conditions and final proposed JSON values for review.

Stop immediately for unexpected current, heating, voltage collapse, unstable USB/scanning, odor, smoke or incorrect channel behavior. Remove power before changing harness connections; do not hot-plug unvalidated modules. Software limiting does not replace electrical validation.

Useful commands include `led render encoder`, `led render bipolar`, `led render minimum`, `led render transition`, `led render gamma`, `led calibration driver`, `led calibration encoder`, `led calibration ramp`, `led calibration compare`, `led calibration reset`, and `led calibration export`. All still require explicit `led enable` for physical output.
