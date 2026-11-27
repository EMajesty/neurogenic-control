# Input scanning

The RP2040 backend starts with `SCAN_CLK` low, `/PL` high, TLC BLANK high,
latch/data/RGB low, and pull-ups on all nine returns. Illumination stays off.
Because the externally powered 74AHCT125 is permanently enabled, GPIO22 is
held low; USB-only operation cannot guarantee buffer behavior with its 5 V rail
absent. Verify rails before attaching illumination wiring.

At each default 4 kHz deadline the scanner pulses `/PL` low for at least 1 µs,
releases it, reads all lanes using one `gpio_get_all()` for initial Q7, then
clocks and samples the remaining bits. Configured 8/16/24-bit lanes are masked
and clock returns low. Period is bounded to 125–500 µs (8–2 kHz). Runtime stats
distinguish actual min/max interval, missed deadlines, contention and maximum
transaction duration from this configured target.

The portable decoder provides independent 5 ms debounce. Ordinary switches are
active-low; PEL12T pushes are active-high through 15 kΩ/10 kΩ dividers. Startup
baselines held controls without synthetic presses. Quadrature state retains
partial cycles, records invalid two-bit jumps, supports inversion and configurable
counts per increment. Saturating 32-bit counters and 64-bit positions avoid
undefined signed overflow; musical acceleration is absent.

Events use a fixed 64-entry ring with physical identity, transition/delta, scan
index and sequence. Overflow is sticky and requires authoritative snapshot
reconciliation. The shared-clock owner rejects overlap; future TLC output must
shift all 288 bits, never latch partial data, release clock low, and request a
fresh snapshot. Its planned 4 MHz rate is not implemented or validated here.
