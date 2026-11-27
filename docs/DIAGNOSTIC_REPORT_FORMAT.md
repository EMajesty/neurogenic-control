# Diagnostic report format

`schemas/diagnostic_report.schema.json` defines report schema 1. The Python
implementation performs stricter semantic checks without a third-party JSON
Schema dependency. Reports contain immutable firmware/hardware/configuration
identities, board instances, the 15 ordered stages, expected and observed
behavior, automated observations, separately labeled operator confirmations,
manual measurements, bounded counters, proposals, notes and outstanding issues.

Statuses are exactly `PASS`, `FAIL`, `NOT RUN`, `BLOCKED`, and `UNKNOWN`.
Unentered measurements are initialized to `NOT RUN` with a null value. Operator
confirmation is never represented as automated electrical evidence. Overall
status remains `NOT RUN` while any stage is unrun, becomes `FAIL` or `BLOCKED`
when such a stage exists, and becomes `PASS` only when every stage passes.

The versioned `NGD1` device format is line-oriented ASCII:

```text
NGD1 type=identity diag=1 firmware=0.4.0 source=<sha> dirty=0 ...
NGD1 type=stats scan_target_hz=4000 scans=... missed=... ...
NGD1 type=error code=unsupported-command
```

Each token after `NGD1` is one unique `key=value` field; values contain no
spaces. Commands are at most 63 bytes. Host parsing rejects non-NGD1 lines,
duplicate fields, absent response types, unsupported diagnostic versions and
oversized responses. Human-oriented legacy responses remain available for a
terminal, but guided automation uses NGD1 responses.

Configuration exports contain the source manifest SHA-256 and an explicit
`PROPOSED_OPERATOR_REVIEW_REQUIRED` status. Harness, encoder inversion/counts,
and calibration proposals are never installed, committed or written to flash
automatically.

Electrical fields cover supply topology/current limit, 12 V input, 5 V and
3.3 V rails, idle/representative/limited-white current, farthest-module voltage,
regulator and ambient temperature, SK timing, TLC clock/latch timing, input
jitter, instrument and notes. The tool records values entered by an operator;
it does not claim to measure them.
