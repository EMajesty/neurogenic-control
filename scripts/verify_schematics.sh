#!/usr/bin/env sh
set -eu
mkdir -p build/netlists
for board in brain buttons-1x8 buttons-2x5 encoders nav pads; do
  kicad-cli sch export netlist -o "build/netlists/$board.net" "hardware/$board/$board.kicad_sch"
done
python3 tools/verify_manifest.py --schematic-dir build/netlists
