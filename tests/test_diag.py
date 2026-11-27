import pathlib,sys,unittest
sys.path.insert(0,str(pathlib.Path(__file__).parents[1]/'tools'))
import neurogenic_diag as d
class TestDiag(unittest.TestCase):
 def test_commands_are_bounded(self):
  self.assertIn('snapshot',d.COMMANDS);self.assertNotIn('',d.COMMANDS);self.assertTrue(all(len(x)<64 for x in d.COMMANDS))
if __name__=='__main__':unittest.main()
