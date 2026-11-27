#!/usr/bin/env sh
set -eu
python3 tools/verify_manifest.py
python3 tools/generate_manifest_header.py --check
python3 tools/generate_harness_header.py --check
python3 tools/generate_calibration_header.py --check
python3 tools/generate_protocol_fixture_header.py --check
python3 tools/verify_upstream_contract.py
cmake -S . -B build/host -DNEUROGENIC_PICO=OFF
cmake --build build/host
ctest --test-dir build/host --output-on-failure
