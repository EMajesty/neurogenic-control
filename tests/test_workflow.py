import json,pathlib,sys,tempfile,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
import diagnostic_workflow as w

class FakeSerial:
 def __init__(self,replies=None,error=None):self.replies=replies or {};self.error=error;self.commands=[]
 def exchange(self,command,timeout_s,max_bytes):
  self.commands.append(command)
  if self.error:raise self.error
  return self.replies.get(command,[])

class WorkflowTests(unittest.TestCase):
 def test_identity_and_version(self):
  line='NGD1 type=identity diag=1 firmware=0.4.0 hardware=x source='+'a'*40+' dirty=0'
  value=w.parse_ngd1(line);w.validate_identity(value)
  value['diag']='2'
  with self.assertRaises(ValueError):w.validate_identity(value)
  with self.assertRaises(ValueError):w.require_capability({'raw'},'stress')
 def test_malformed_and_timeout(self):
  with self.assertRaises(ValueError):w.parse_ngd1('NOT-NGD')
  with self.assertRaises(TimeoutError):w.query_endpoint(FakeSerial(),'info')
 def test_safe_normal_termination(self):
  ep=FakeSerial({'info':['NGD1 type=identity diag=1 dirty=1 source=unknown']})
  w.safe_session(ep,['info']);self.assertEqual(ep.commands[-1],'led off')
 def test_button_missing_release_and_unexpected(self):
  self.assertEqual(w.guided_button_result('MOD1.0',[{'control':'MOD1.0','event':'PRESS'}])['status'],'FAIL')
  result=w.guided_button_result('MOD1.0',[{'control':'MOD1.0','event':'PRESS'},{'control':'MOD2.0','event':'PRESS'},{'control':'MOD1.0','event':'RELEASE'}])
  self.assertIn('unexpected-simultaneous-transition',result['reasons'])
 def test_report_never_promotes_missing_evidence(self):
  report=w.new_report();w.validate_report(report)
  self.assertEqual(report['overall_status'],'NOT RUN')
  w.set_stage(report,'brain-inspection','PASS');self.assertEqual(report['overall_status'],'NOT RUN')
  self.assertFalse(w.run_prerequisite(lambda _: 'no','power checked'))
 def test_report_and_markdown_export(self):
  schema=json.loads((pathlib.Path(__file__).parents[1]/'schemas/diagnostic_report.schema.json').read_text());self.assertEqual(schema['properties']['schema_version']['const'],1)
  report=w.new_report({'firmware':'fixture','dirty':'1'});report['fixture']=True
  with tempfile.TemporaryDirectory() as directory:
   path=pathlib.Path(directory)/'report.json';w.save_report(report,path)
   self.assertEqual(w.load_report(path)['fixture'],True)
   self.assertIn('NOT RUN',w.markdown(report))
 def test_harness_profiles_derive_chain_lengths(self):
  proposal=w.propose_harness('one-2x5');self.assertEqual(proposal['ports'][0]['chain_bits'],16)
  self.assertEqual(proposal['ports'][-1]['chain_bits'],0)
 def test_encoder_proposal(self):
  value=w.propose_encoder('DATA',-40,40,10)
  self.assertTrue(value['invert']);self.assertEqual(value['counts_per_increment'],4)
 def test_calibration_proposal(self):
  source=json.loads((pathlib.Path(__file__).parents[1]/'hardware/illumination_calibration.json').read_text())
  value=w.propose_calibration(source);self.assertEqual(value['status'],'PROPOSED_OPERATOR_REVIEW_REQUIRED');self.assertEqual(len(value['source_manifest_sha256']),64)
 def test_disconnection(self):
  with self.assertRaises(ConnectionError):w.query_endpoint(FakeSerial(error=ConnectionError('gone')),'info')

if __name__=='__main__':unittest.main()
