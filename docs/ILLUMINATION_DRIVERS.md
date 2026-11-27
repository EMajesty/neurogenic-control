# Illumination drivers

HW-003 implements one logical illumination authority and two internal physical drivers. Illumination remains unavailable until the operator sends `led enable` after confirming the external 5 V rail is present. USB power is not evidence that the AHCT125 or LEDs are powered.

## SK6812MINI-E

GPIO22 drives the brain-board 74AHCT125 and `RGB_DIN1`. The RP2040 PIO state machine runs at 8 MHz and emits a 1.25 us cell: 0.25 us high for zero and 0.875 us high for one. Data is GRB, MSB first. DMA supplies 58 complete 24-bit pixels without blocking the scanner. A fixed two-buffer queue owns active and pending frames; a new request replaces an unsent request, and all-off has priority. Completion waits for the FIFO to empty and then holds the line low for 100 us (the OPSCO Rev. 02 requirement is greater than 80 us).

The provisional harness defines physical chain order as enabled MOD1 through MOD8. Module type supplies pixel count (1x8: 8, 2x5: 10, nav: 6, pads: 16). Cable order and bypass continuity remain unverified. The reference count is 58.

## TLC5947

Portable serialization emits OUT23 grayscale bit 11 first and OUT0 bit 0 last: exactly 288 bits/36 bytes. The encoder RGB-to-OUT mapping is generated from `hardware/manifest.json`; it is deliberately nonsequential. The RP2040 backend acquires `SharedBus`, starts with SCAN_CLK low, shifts a complete frame, pulses XLAT once, returns clock/data/latch low, then releases the bus. The next scan therefore starts with parallel load. BLANK stays asserted at startup and while disabled; normal updates do not toggle BLANK, avoiding unnecessary flicker.

The initial bounded GPIO implementation uses direct GPIO writes with short cycle delays. TI permits much higher rates, but the actual serial rate and 4 MHz target require oscilloscope validation. Firmware reports maximum software transaction duration; it is not an electrical measurement.

## Safety and failure behavior

- Initialization holds BLANK, XLAT, TLC data, SCAN_CLK and RGB data in safe states.
- `led disable` asserts BLANK immediately and queues a bounded addressable all-off frame. An MCU reset alone does not clear previously latched SK6812 pixels.
- Invalid indexes, values, long commands and unrestricted raw frames are rejected.
- Initialization failure or no explicit enable leaves illumination off/unavailable.
- SK DMA and TLC/shared-clock errors are counted; physical fail-safe behavior still needs board validation.

Manufacturer bases: OPSCO `SK6812MINI-E` Rev. 02 and Texas Instruments `TLC5947` SBVS114B. Timing here is an implementation target, not a scope measurement.

HW-004 corrected the final-bit boundary: DMA completion and FIFO-empty are insufficient because the OSR may still be shifting. Service now clears and waits for the state machine's TXSTALL flag, proving it returned to the empty-FIFO pull after the final bit, before beginning the 100 us reset interval. The fixed active buffer remains owned through reset. TLC retains 288 production-serializer edges and one latch under shared ownership. Its cycle-delayed GPIO transaction is admitted only with 120 us scheduling slack; actual duration relative to the 250 us scan period still requires target GPIO timing and oscilloscope measurement.

HW-005 bounds repeated TLC deferral to 50 ms while retaining exclusive shared
clock ownership, complete 288-bit transfer, one XLAT pulse, clock-idle-low and a
fresh subsequent snapshot. Forced admission is visible as scan jitter/missed
deadlines. SK completion still waits for PIO TXSTALL, then enforces 100 us reset.
