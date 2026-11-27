import binascii,hashlib,json,pathlib,struct,sys,unittest,zlib
ROOT=pathlib.Path(__file__).parents[1]

def crc16(data):
 value=0xffff
 for byte in data:
  value^=byte<<8
  for _ in range(8):value=((value<<1)^0x1021)&0xffff if value&0x8000 else (value<<1)&0xffff
 return value

def encode(kind,sequence,payload):
 body=struct.pack('<HBBHI',0x474e,1,kind,len(payload),sequence)+payload
 return body+struct.pack('<H',crc16(body))

class ProtocolFixtureTests(unittest.TestCase):
 @classmethod
 def setUpClass(cls):
  cls.base=json.loads((ROOT/'protocol/fixtures/baseline_v1_3.json').read_text())
  cls.ext=json.loads((ROOT/'protocol/fixtures/proposed_extension_v1.json').read_text())
 def test_upstream_vectors(self):
  for vector in self.base['vectors']:
   self.assertEqual(encode(vector['type'],vector['sequence'],bytes.fromhex(vector['payload_hex'])).hex(),vector['frame_hex'],vector['name'])
 def test_baseline_locked(self):
  self.assertEqual(self.base['inventory'],{'count':67,'buttons':58,'encoders':9,'fingerprint':'0x3256434e'})
  self.assertEqual(self.base['frame']['maximum_frame'],268)
 def test_proposed_push_ids_are_disjoint_and_ordered(self):
  ids=[int(x['proposed_wire_id'],16) for x in self.ext['push_inventory']['entries']]
  self.assertEqual(ids,list(range(0x0160,0x0168)));self.assertTrue(all(x>0x0159 for x in ids))
 def test_provisional_fingerprints(self):
  push=b''.join(x['identity'].encode()+b'\0'+int(x['proposed_wire_id'],16).to_bytes(2,'little')+b'\1' for x in self.ext['push_inventory']['entries'])
  rgb=b''.join(f'parameter.{i}'.encode()+b'\0'+(0x011f+i).to_bytes(2,'little')+b'\2' for i in range(1,9))
  self.assertEqual(f"0x{zlib.crc32(push)&0xffffffff:08x}",self.ext['push_inventory']['provisional_fingerprint'])
  self.assertEqual(f"0x{zlib.crc32(rgb)&0xffffffff:08x}",self.ext['encoder_rgb_inventory']['provisional_fingerprint'])
 def test_manifest_physical_inventory(self):
  manifest=json.loads((ROOT/'hardware/manifest.json').read_text());physical=manifest['assembly']['physical_inputs']
  self.assertEqual(physical,{'buttons':58,'rotary_encoders':9,'optional_encoder_pushes':8})
  self.assertEqual(manifest['assembly']['sk6812_count'],58);self.assertEqual(manifest['assembly']['encoder_rgb_count'],8)
  nav=manifest['board_types']['nav']['data_encoder'];self.assertEqual(nav['push'],'U1.bit3')
 def test_extension_is_non_normative(self):
  self.assertEqual(self.ext['status'],'PROVISIONAL_NOT_NORMATIVE_NOT_IMPLEMENTED')
  self.assertFalse(self.ext['version_policy']['production_version_changed'])
  self.assertEqual(self.ext['unassigned_numbers']['new_message_type_ids'],'TBD by upstream approval')
 def test_provisional_case_matrix(self):
  cases={case['name']:case for case in self.ext['cases']}
  self.assertEqual(set(cases),{'baseline-fallback','push-only','rgb-only','both','unknown-capability','wrong-push-inventory','stale-generation'})
  self.assertEqual(cases['baseline-fallback']['negotiated_minor'],3)
  self.assertEqual(cases['push-only']['capabilities'],['CONTEXT_ENCODER_PUSH'])
  self.assertEqual(cases['rgb-only']['capabilities'],['CONTEXT_ENCODER_RGB'])
  self.assertIn('incompatible',cases['wrong-push-inventory']['result'])
  self.assertIn('reject',cases['stale-generation']['result'])
 def test_vendored_header_hash(self):
  lock=json.loads((ROOT/'protocol/upstream.lock.json').read_text());actual=hashlib.sha256((ROOT/lock['vendored_path']).read_bytes()).hexdigest()
  self.assertEqual(actual,lock['sha256']);self.assertEqual(lock['revision'],self.base['upstream_revision'])

if __name__=='__main__':unittest.main()
