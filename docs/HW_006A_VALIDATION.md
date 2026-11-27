# HW-006A validation

Validation date: 2026-11-27. Normative upstream source was current
`EMajesty/neurogenic` `origin/main` at
`5f94323acff8a78059c05a4e1e508a6a7e0ced44`. A clean archive of that exact
revision was used for its production `neurogenic_controller_v2_protocol_tests`;
the unrelated dirty files in the developer's upstream worktree were not read as
authority or modified.

## Implemented foundation

- exact dependency-free upstream wire header, revision/hash lock and MIT
  license provenance under `protocol/upstream`;
- ten deterministic upstream-derived baseline frame vectors plus malformed
  CRC, length and major cases;
- portable bounded CRC/frame codec used by the native conformance test;
- generated baseline fixture header with build-time freshness enforcement;
- explicitly provisional, non-normative extension vectors for fallback,
  independent capability combinations, invalid inventory, unknown capability
  and stale generation;
- source-grounded audit, extension specification, compatibility matrix, and
  HW-006B/HW-006C handoffs.

No production binary USB endpoint, optional capability, protocol version or
wire ID was enabled.

## Exact results

Commands were run from repository root:

```text
python3 tools/verify_manifest.py
python3 tools/generate_manifest_header.py --check
python3 tools/generate_harness_header.py --check
python3 tools/generate_calibration_header.py --check
python3 tools/generate_protocol_fixture_header.py --check
python3 tools/verify_upstream_contract.py --upstream /home/emaj/git/neurogenic
```

All passed. Manifest verification covered six resolved PCB netlists; the
provisional harness assignment remains explicitly unverified. The contract
check matched the pinned upstream object, vendored SHA-256, fixture revision,
locked constants and license SHA-256.

`nix develop path:.#hardware --command scripts/verify_schematics.sh` used KiCad
10.0.8 to freshly export all six schematic netlists. Component-pin nets agreed
with resolved PCB pad nets and the manifest.

Native and sanitizer commands were:

```text
nix develop path:. --command bash -lc 'cmake -S . -B build/hw006a-host -G Ninja && cmake --build build/hw006a-host && ctest --test-dir build/hw006a-host --output-on-failure'
nix develop path:. --command bash -lc 'cmake -S . -B build/hw006a-asan -G Ninja -DNEUROGENIC_SANITIZERS=ON && cmake --build build/hw006a-asan && ctest --test-dir build/hw006a-asan --output-on-failure'
```

Both passed 5/5 CTest targets: portable firmware host tests, diagnostic utility,
diagnostic workflow, protocol conformance, and fixture schema. The Python
targets contain 18 deterministic cases (1 diagnostic, 10 workflow, 7 protocol
fixture). After the final fixture-matrix assertion was added, the Python total
is 19 (1 diagnostic, 10 workflow, 8 protocol fixture). Ten baseline golden
frames encoded and decoded exactly; malformed
CRC, oversized payload, wrong major and over-limit encode were rejected.
ASan/UBSan reported no failures.

The pinned upstream clean archive was configured with GUI/Slint disabled and
its `neurogenic_controller_v2_protocol_tests` target built and ran successfully,
printing `neurogenic.controller-v2-protocol-tests.v1 passed`. This ties fixture
provenance to the actual upstream production codec/session test suite; the local
codec is not claimed as normative.

`nix flake check path:.` passed for x86_64-linux; aarch64-linux was reported
incompatible with the evaluation system and skipped. `scripts/test.sh` passed
all 5/5 targets. Initial native CMake outside Nix failed because Ninja/compiler
were absent, confirming the documented Nix shell is required on this machine.

## RP2040 Release artifact

```text
nix develop path:. --command bash -lc 'cmake -S firmware -B build/hw006a-pico -G Ninja -DCMAKE_BUILD_TYPE=Release -DPICO_BOARD=pico && cmake --build build/hw006a-pico'
```

Pico SDK 2.1.1 and ARM GCC 15.3.1 produced ELF, UF2, BIN, HEX and linker map.
Memory usage was text 124,284 bytes, data 0, BSS 5,304, total 129,588. Artifact
sizes were ELF 1,116,948 bytes, UF2 248,832, BIN 124,288, and map 660,886.
The pre-commit UF2 SHA-256 was
`2fb11d44bb974e811139d3c253c9d644583fb33d1c15d9b4223b6136d1513809`.
Because firmware embeds source identity, the final clean commit build and hash
are reported in the completion report.

## Not run and remaining decisions

Physical controller execution, USB binary interoperability, electrical timing,
input behavior and illumination remain **NOT RUN**. Upstream must approve the
minor-version policy, capability bits, optional push IDs, inventory record
format/fingerprints and snapshot suffix before HW-006C. The existing 1.3
LED_RENDER envelope is recommended without a richer-rendering capability.
