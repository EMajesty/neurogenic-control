# Harness configuration

The boards have no module identity or presence pin. A disconnected pull-up and
a valid constant pattern can be indistinguishable, so status is configured,
suspect, or unknown—never reliable automatic identification.

[`hardware/harness.example.json`](../hardware/harness.example.json) is a bounded
**provisional, unapproved example**. It lists MOD1–MOD8 and ENC with physical
type, unique logical instance, exact chain length, enabled state and optional
analog return. Repeated types are supported; `unused` requires zero bits and a
disabled state. Names do not imply any musical assignment.

Run `python3 tools/generate_harness_header.py` and commit JSON plus header.
CMake/CI fails if they differ. Replace the example only after powered-off cable
tracing and owner approval. Raw diagnostics remain useful beforehand.
