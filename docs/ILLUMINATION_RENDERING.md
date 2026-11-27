# Illumination rendering

HW-004 consulted NEUROGENIC revision `ae66cee8b526cd999c2e957ecf1302080a92aaff`, especially `VISUAL_LANGUAGE.md`, `REFERENCE_CONTROLLER_V2.md`, `REFERENCE_CONTROLLER_V2_FIRMWARE.md`, and `CONTROLLER_PROTOCOL.md`.

The implemented bounded pipeline is:

`explicit host/diagnostic state -> functional RGB + operational state -> normalized perceptual magnitude -> temporal/interpolated brightness -> fixed-point device calibration -> gamma -> 8/12-bit electrical PWM -> combined current limiter -> physical publication`.

Functional RGB, normalized values, assignment availability and operational meaning remain host-owned. Firmware owns physical mapping, bounded interpolation/animation, calibration, gamma, brightness ceiling, current limiting and scheduling. Palette constants reproduce the upstream provisional values only as diagnostic presets; physical indices have no musical meaning.

## Magnitude and state

Magnitude is an already-normalized unsigned 16-bit value. Unipolar maps 0 to the configured assigned minimum and 65535 to maximum. Bipolar uses 32768 as center and is symmetric toward either endpoint. Boolean uses minimum/maximum; enumerations use constant middle emphasis. The host must normalize logarithmic or other nonlinear musical ranges.

The provisional encoder minimum is 31/255 (12.2%). Unassigned, unavailable and explicit off are zero. Minimum brightness is still subject to global brightness and current limiting.

Operational precedence is explicit: recording red, queued amber, selected white, active green, locked purple, muted functional hue at reduced brightness, otherwise the supplied functional hue. This renderer does not infer state. Explicit off/unavailable wins; operational override then hue, magnitude, temporal envelope, calibration, gamma and safety limiting follow.

## Time and scheduling

Steady, blink, triangle pulse, bounded fade, one-shot acknowledgement and off use injected monotonic 32-bit milliseconds with unsigned wrap-safe subtraction. Updates replace targets; no intermediate queue exists. Ordinary brightness changes interpolate over 40 ms. Explicit all-off clears all state immediately. Firmware evaluates at 100 Hz and physical drivers retain their bounded coalescing, asynchronous SK DMA and scan-prioritized TLC admission.

Gamma is fixed-point: linear or the default quadratic `x²` curve. Exact zero and endpoints are preserved and mappings are monotonic. Identical curves do not imply matched optical output between driver families.
