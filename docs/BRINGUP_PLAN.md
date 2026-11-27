# Ordered-board input bring-up

Power down before every harness change. Keep illumination disconnected or
disabled. At every stage run `version`, `hardware`, `ports`, `raw`, and `stats`;
stop for unexpected current, heat, smell, rail voltage, USB resets, shorts,
reversed connectors or unsafe BLANK/RGB states.

1. **Brain only:** inspect polarity and continuity. USB-only boot should
   enumerate CDC and show pull-up patterns. Measure 3.3 V; the external 5 V AHCT
   buffer is not guaranteed valid while unpowered.
2. **External power:** current-limit 12 V. Verify 5 V/3.3 V, regulator
   temperature, BLANK high, LED data/latch low. Stop on current limit.
3. **One 1×8:** attach powered-off; verify its eight raw bits individually.
4. **One 2×5:** verify its 16-bit cascade and unused high bits.
5. **Navigation:** verify six switches plus DATA A/B/push; rotate slowly both
   ways and record the physical detent/count convention.
6. **Encoder board:** verify 24 bits, each A/B pair, active-high pushes and
   divider voltages. TLC stays blank.
7. **Pad board:** verify J2 and all 16 positions; leave LEDs disconnected.
8. **Complete harness:** after powered-off tracing, replace the provisional
   config. Exercise simultaneous presses, holds, releases, reversals, USB
   reconnect, 10 Hz stream and overflow reconciliation.

Record supply limits, currents, remote rail voltages, scan stats and actual MOD
assignment. Diagnostics are observation, not electrical validation or reliable
presence detection. Never hot-plug these unvalidated modules.

## HW-003 illumination stages

Keep LEDs disabled through input validation. With power off, connect the complete intended RGB chain. Apply a current-limited external 5 V supply; measure the brain and farthest-module rails; then issue `led enable` and `led status`. Walk pixels, TLC channels and encoder colors at default 24/255 brightness, recording observed order/swaps without changing verified mappings. Briefly test RGB and current-limited white while monitoring rail voltage, supply current, regulator temperature and input `stats`. Stop for excess current, heating, rail collapse, USB instability, odor or smoke. End with `led off`, wait, `led disable`, then remove power. This does not authorize hot-plugging.

After chain validation, follow `ILLUMINATION_CALIBRATION.md`: establish minimum visibility, compare primaries and driver families, measure representative current/droop/temperature, scope TLC duration and scan jitter, export proposed values, and review before committing calibration JSON. Identity defaults are not calibration evidence.

HW-005 turns these stages into the resumable `neurogenic_diag.py bringup`
workflow described in `HARDWARE_ACCEPTANCE.md`. It records explicit
PASS/FAIL/NOT RUN/BLOCKED/UNKNOWN results and never treats missing data as
acceptance. Save its JSON and generated Markdown together for review.
