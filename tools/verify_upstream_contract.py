#!/usr/bin/env python3
"""Verify the offline vendored contract and fixture provenance pin."""
import argparse,hashlib,json,pathlib,re,subprocess,sys
ROOT=pathlib.Path(__file__).resolve().parents[1]
LOCK=ROOT/'protocol/upstream.lock.json'
def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()
def main():
 p=argparse.ArgumentParser();p.add_argument('--upstream',type=pathlib.Path);a=p.parse_args()
 lock=json.loads(LOCK.read_text());vendored=ROOT/lock['vendored_path'];errors=[]
 if digest(vendored)!=lock['sha256']:errors.append('vendored wire header hash mismatch')
 license_path=ROOT/'protocol/upstream/LICENSE.neurogenic'
 if digest(license_path)!=lock['license_sha256']:errors.append('vendored upstream license hash mismatch')
 fixture=json.loads((ROOT/'protocol/fixtures/baseline_v1_3.json').read_text())
 if fixture['upstream_revision']!=lock['revision']:errors.append('baseline fixture revision mismatch')
 text=vendored.read_text()
 expected={'NG_CONTROLLER_PROTOCOL_MAJOR':'1u','NG_CONTROLLER_V2_PROTOCOL_MINOR':'3u','NG_CONTROLLER_V2_CONTROL_COUNT':'67u','NG_CONTROLLER_V2_BUTTON_COUNT':'58u'}
 for key,value in expected.items():
  if not re.search(rf'#define\s+{key}\s+{re.escape(value)}\b',text):errors.append(f'{key} mismatch')
 if 'UINT32_C(0x3256434e)' not in text:errors.append('baseline fingerprint mismatch')
 if a.upstream:
  try:source=subprocess.check_output(['git','-C',str(a.upstream),'show',f"{lock['revision']}:{lock['source_path']}"])
  except subprocess.CalledProcessError as exc:errors.append(f'cannot read pinned upstream source: {exc}');source=b''
  if source and hashlib.sha256(source).hexdigest()!=lock['sha256']:errors.append('pinned upstream source differs from vendored header')
 if errors:
  print('\n'.join(errors),file=sys.stderr);return 1
 print(f"upstream contract: pinned {lock['revision']}; vendored header and fixtures verified")
 return 0
if __name__=='__main__':raise SystemExit(main())
