# Bring-up plan

1. Inspect every board, connector key, cable, arcade switch/LED lead and rail
   for assembly defects; photograph and record actual `MOD1`–`MOD8` assignment.
2. With modules disconnected, current-limit 12 V input and verify polarity,
   3.3 V, 5 V, Pico VSYS path, regulator temperature and BLANK-at-startup.
3. Attach one unpowered module at a time; continuity-check connector pins and
   confirm no rail short before applying power.
4. Flash the identity skeleton, verify CDC reconnect/reset behavior, then use
   raw snapshot diagnostics from HW-002 to walk every 165 bit and determine
   encoder direction, push thresholds, bounce and practical scan rate.
5. At conservative limits, walk addressable pixels and TLC channels, verify
   RGB polarity/order and external pad LED ordering. Measure rail current and
   temperature before increasing brightness.
6. Run simultaneous presses, fast encoder turns, held reconnect, bus-arbitration
   stress and full-state reconciliation. Only afterward run protocol/host tests.

Never hot-plug an unverified module and never infer success from host tests.
