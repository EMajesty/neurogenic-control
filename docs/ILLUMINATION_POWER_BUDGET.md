# Illumination power budget

The bring-up policy is a combined 900 mA modeled illumination limit. It applies after logical brightness, global brightness and gamma conversion to both 58 SK6812MINI-E pixels and all 24 TLC5947 channels. The deterministic limiter estimates the requested current, subtracts SK6812 quiescent current, and applies one Q12 scale factor to every nonzero electrical channel. This preserves RGB ratios. Integer divisions round to nearest during PWM conversion and down when calculating the safety scale; outputs saturate at 8-bit or 12-bit limits and zero stays exactly zero.

Defaults and assumptions:

- boot global brightness: 24/255; diagnostics may request up to the policy cap of 128/255;
- modeled SK6812 channel current: 12 mA at full PWM, configurable;
- modeled SK6812 quiescent current: 0.6 mA per pixel, separate;
- modeled TLC5947 channel current: 10.5 mA, based on the fitted 4.7 kOhm IREF resistor and subject to tolerance;
- combined modeled ceiling: 900 mA against a nominal 2 A R-78K5.0-2.0 rail.

Every static diagnostic, walk animation and all-white request goes through the same limiter. There is no bypass command. `led status` reports requested current, limited current and Q12 scale.

This is an average PWM-duty estimate. It does not bound instantaneous LED pulses, wiring drop, inrush, converter transient response or regulator temperature. Purchased Adafruit 4960 and incoming marketplace LEDs may differ. Software limiting does not replace current-limited supply tests, thermal measurements or characterization of the installed parts.

HW-004 changes uncharacterized defaults to 20 mA per SK6812 color channel and 1 mA quiescent per pixel. TLC remains 10.5 mA/channel and the combined ceiling remains 900 mA. Calibration, minimum brightness, animation and diagnostics all precede the same final electrical limiter; zero budget produces zero PWM.

HW-005 stress, guided walks and calibration demonstrations retain this limiter.
The 30-second watchdog and stress completion queue all-off and assert TLC BLANK,
but cannot sense rail power or replace removal of power during a fault.
