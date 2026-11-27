#!/usr/bin/env python3
"""Generate/check C++ vectors from the pinned upstream baseline fixture."""
import argparse,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
SOURCE=ROOT/'protocol/fixtures/baseline_v1_3.json'
OUTPUT=ROOT/'protocol/generated/baseline_fixtures.hpp'
def bytes_init(value):return ','.join(f'0x{value[i:i+2]}' for i in range(0,len(value),2))
def render():
 data=json.loads(SOURCE.read_text());vectors=data['vectors']
 rows=[]
 for v in vectors:
  frame=v['frame_hex'];payload=v['payload_hex']
  rows.append(f'Fixture{{"{v["name"]}",{v["type"]}u,{v["sequence"]}u,{{{bytes_init(payload)}}},{{{bytes_init(frame)}}}}}')
 return '''#pragma once
// GENERATED from protocol/fixtures/baseline_v1_3.json; do not edit.
#include <cstdint>
#include <string_view>
#include <vector>
namespace ng::baseline_fixtures {
struct Fixture { std::string_view name; std::uint8_t type; std::uint32_t sequence; std::vector<std::uint8_t> payload, frame; };
inline const std::vector<Fixture> vectors={%s};
inline constexpr char upstream_revision[]="%s";
}
'''%(','.join(rows),data['upstream_revision'])
def main():
 p=argparse.ArgumentParser();p.add_argument('--check',action='store_true');a=p.parse_args();value=render()
 if a.check:
  if not OUTPUT.exists() or OUTPUT.read_text()!=value:print('stale protocol fixture header',file=sys.stderr);return 1
  print('generated protocol fixture header: fresh');return 0
 OUTPUT.parent.mkdir(parents=True,exist_ok=True);OUTPUT.write_text(value);print(f'wrote {OUTPUT}');return 0
if __name__=='__main__':raise SystemExit(main())
