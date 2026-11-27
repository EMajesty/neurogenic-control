#!/usr/bin/env python3
import argparse,hashlib,json,pathlib,sys
ROOT=pathlib.Path(__file__).resolve().parents[1];SRC=ROOT/'hardware/illumination_calibration.json';OUT=ROOT/'firmware/generated/illumination_calibration.hpp'
def render():
 d=json.loads(SRC.read_text());errors=[]
 fingerprint=hashlib.sha256(SRC.read_bytes()).hexdigest()
 if d.get('schema_version')!=1:errors.append('schema_version must be 1')
 for driver in ('sk6812','tlc5947'):
  v=d.get('driver_gain_q8',{}).get(driver)
  if not isinstance(v,list) or len(v)!=3 or any(not isinstance(x,int) or x<0 or x>512 for x in v):errors.append(f'{driver} gains must be 3 integers 0..512')
 for k,lo,hi in [('global_ceiling',0,255),('minimum_assigned',0,255),('transition_ms',0,1000)]:
  if not isinstance(d.get(k),int) or not lo<=d[k]<=hi:errors.append(f'{k} out of range')
 if d.get('gamma_profile') not in ('linear','quadratic'):errors.append('invalid gamma profile')
 p=d.get('power',{})
 for k,hi in [('limit_ua',2000000),('sk_channel_ua',30000),('sk_quiescent_ua',5000),('tlc_channel_ua',30000)]:
  if not isinstance(p.get(k),int) or not 0<=p[k]<=hi:errors.append(f'power.{k} out of range')
 if errors:raise ValueError('\n'.join(errors))
 a=d['driver_gain_q8']['sk6812'];b=d['driver_gain_q8']['tlc5947'];gamma=0 if d['gamma_profile']=='linear' else 1
 return f'''#pragma once
// GENERATED from hardware/illumination_calibration.json; uncalibrated defaults.
#include <array>
#include <cstdint>
namespace ng::calibration_defaults {{inline constexpr uint16_t schema_version=1;inline constexpr char sha256[]="{fingerprint}";inline constexpr char status[]="{d['status']}";inline constexpr std::array<uint16_t,3> sk_gain={{{','.join(map(str,a))}}};inline constexpr std::array<uint16_t,3> tlc_gain={{{','.join(map(str,b))}}};inline constexpr uint8_t global_ceiling={d['global_ceiling']},minimum_assigned={d['minimum_assigned']};inline constexpr uint16_t transition_ms={d['transition_ms']};inline constexpr uint8_t gamma_profile={gamma};inline constexpr uint32_t limit_ua={p['limit_ua']},sk_channel_ua={p['sk_channel_ua']},sk_quiescent_ua={p['sk_quiescent_ua']},tlc_channel_ua={p['tlc_channel_ua']};}}
'''
def main():
 a=argparse.ArgumentParser();a.add_argument('--check',action='store_true');x=a.parse_args();s=render()
 if x.check:
  if not OUT.exists() or OUT.read_text()!=s:print('stale generated calibration header',file=sys.stderr);return 1
  print('generated calibration header: fresh');return 0
 OUT.write_text(s);print(f'wrote {OUT}');return 0
if __name__=='__main__':raise SystemExit(main())
