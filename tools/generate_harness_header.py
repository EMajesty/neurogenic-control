#!/usr/bin/env python3
"""Generate/check bounded harness configuration from an explicit JSON file."""
import argparse,hashlib,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[1];SRC=ROOT/'hardware/harness.example.json';OUT=ROOT/'firmware/generated/harness_config.hpp'
TYPES={'unused':0,'buttons-1x8':8,'buttons-2x5':16,'nav':8,'pads':16,'encoders':24}
def render():
 d=json.loads(SRC.read_text());p=d.get('ports',[]);errors=[]
 fingerprint=hashlib.sha256(SRC.read_bytes()).hexdigest()
 if d.get('schema_version')!=1:errors.append('schema_version must be 1')
 if len(p)!=9 or [x.get('port') for x in p]!=[f'MOD{i}' for i in range(1,9)]+['ENC']:errors.append('ports must be ordered MOD1..MOD8,ENC')
 names=set()
 for x in p:
  t=x.get('module_type');n=x.get('logical_instance')
  if t not in TYPES or x.get('chain_bits')!=TYPES.get(t):errors.append(f"{x.get('port')} invalid type/chain")
  if not isinstance(n,str) or not n or n in names:errors.append('logical instances must be nonempty and unique')
  names.add(n)
  if x.get('analog') not in (None,'ANALOG1','ANALOG2','ANALOG3'):errors.append(f"{x.get('port')} invalid analog")
 if errors:raise ValueError('\n'.join(errors))
 bits=','.join(str(x['chain_bits']) for x in p);enabled=','.join('true' if x['enabled'] else 'false' for x in p);codes={'unused':0,'buttons-1x8':1,'buttons-2x5':2,'nav':3,'pads':4,'encoders':5};types=','.join(str(codes[x['module_type']]) for x in p)
 led_counts={'unused':0,'buttons-1x8':8,'buttons-2x5':10,'nav':6,'pads':16,'encoders':0};counts=[];offsets=[];offset=0
 for x in p[:8]:
  offsets.append(offset);count=led_counts[x['module_type']] if x['enabled'] else 0;counts.append(count);offset+=count
 assembly_count=json.loads((ROOT/'hardware/manifest.json').read_text())['assembly']['sk6812_count']
 if offset!=assembly_count:raise ValueError(f'configured pixel count {offset} != assembly count {assembly_count}')
 return f'''#pragma once
// GENERATED from hardware/harness.example.json; PROVISIONAL, NOT OWNER-APPROVED.
#include <array>
#include <cstdint>
namespace ng::harness {{inline constexpr uint16_t schema_version={d['schema_version']};inline constexpr char sha256[]="{fingerprint}";inline constexpr char status[]="{d['status']}";inline constexpr std::array<uint8_t,9> chain_bits={{{bits}}};inline constexpr std::array<bool,9> enabled={{{enabled}}};inline constexpr std::array<uint8_t,9> module_type={{{types}}};inline constexpr std::array<uint8_t,8> pixel_offset={{{','.join(map(str,offsets))}}};inline constexpr std::array<uint8_t,8> pixel_count={{{','.join(map(str,counts))}}};inline constexpr uint8_t total_pixels={offset};}}
'''
def main():
 a=argparse.ArgumentParser();a.add_argument('--check',action='store_true');x=a.parse_args();s=render()
 if x.check:
  if not OUT.exists() or OUT.read_text()!=s:print('stale generated harness header',file=sys.stderr);return 1
  print('generated harness header: fresh');return 0
 OUT.write_text(s);print(f'wrote {OUT}');return 0
if __name__=='__main__':raise SystemExit(main())
