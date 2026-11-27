# Firmware architecture

Firmware `0.1.0` targets the RP2040 Pico SDK with CMake and USB CDC ACM. The
portable core has no steady-state allocation requirement; board identification,
raw input snapshots, debounced digital state, quadrature state, optional
switches, logical illumination, USB transport/host state and diagnostics are
separate interfaces. GPIO code never assigns PAGE, TRACK, SOLO or musical
meaning. The pinned upstream protocol contract is the `neurogenic` commit
recorded in the manifest; the dependency-free header is copied only at build
integration time in a later protocol task, not forked here. HW-002 adds the
portable scanner, fixed event queue, independent debounce, quadrature counters,
Pico GPIO backend and bounded diagnostic parser described in `INPUT_SCANNING.md`.

One bounded scan asserts shared `/PL` for a simultaneous snapshot, then clocks
up to the longest 24-bit chain while sampling nine independent returns. Shorter
chains ignore later clocks. A fixed per-input integrator/deadline debounce and
quadrature state table consume snapshots; no unbounded event queue is allowed.
The baseline schedule target is one scan transaction per 250 µs with fixed
buffers, but this is a design value, not a measured physical rate. Snapshots,
pressed-state bitmaps and host generations reconcile held buttons and lost
releases after USB reconnect. Invalid quadrature jumps increment diagnostics.

`SharedBus` is the sole owner of `SCAN_CLK`. A TLC transfer blanks outputs,
shifts exactly 288 MSB-first grayscale bits with clock idle low, latches once,
re-snapshots inputs, then unblanks and releases ownership. No scan overlaps it,
and ownership failure occurs before any clock or latch event. Startup keeps
BLANK asserted until a complete capped frame exists. At a conservative 1 MHz
software clock, the maximum planned interruption is 300 µs including latch and
recovery; measure and revise it on hardware.

Illumination is represented as hue, normalized brightness, operational state,
global brightness and gamma separately from physical output. Pure helpers map
8-bit perceptual values to 12-bit PWM and estimate current. Hardware SK6812 and
TLC drivers are deferred to HW-003.

The budget model assumes up to 20 mA per SK6812 color and 20 mA per encoder
channel, sums PWM-weighted channel duties, defaults global brightness to 24/255
at boot, and caps the modeled LED load at 900 mA. Diagnostics and animations
use the same budget. This deliberately leaves regulator and logic margin. PWM
averages do not model LED tolerances, switching peaks, wiring loss, regulator
thermal behavior or synchronized PWM phases; software limiting never replaces
electrical and thermal validation.

## HW-003 illumination authority

Logical RGB/brightness state publishes complete immutable electrical frames. The pure stage applies global brightness, gamma and one combined Q12 current scale before the driver split. SK6812 uses a two-buffer PIO/DMA publisher; TLC5947 uses the scanner's single shared-clock owner and a complete 288-bit transaction. Diagnostics use the same authority and cannot bypass the limiter. Physical identities remain separate from musical semantics.
