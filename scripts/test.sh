#!/usr/bin/env sh
set -eu
python3 tools/verify_manifest.py
cmake -S . -B build/host -DNEUROGENIC_PICO=OFF
cmake --build build/host
ctest --test-dir build/host --output-on-failure
