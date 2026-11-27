import pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
import neurogenic_diag as d
class TestDiag(unittest.TestCase):
 def test_commands_are_bounded(self):
  self.assertIn('snapshot',d.COMMANDS);self.assertNotIn('',d.COMMANDS);self.assertTrue(all(len(x)<64 for x in d.COMMANDS))
  self.assertTrue(d.valid_command('led pixel 57 255 0 1'));self.assertFalse(d.valid_command('led pixel 58 0 0 0'));self.assertTrue(d.valid_command('led channel 23 4095'));self.assertFalse(d.valid_command('led brightness 256'))
if __name__=='__main__':unittest.main()
