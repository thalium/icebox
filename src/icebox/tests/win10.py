import os
import inspect
import sys
import unittest

class Fixture(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        icebox.attach("win10")

    @classmethod
    def tearDownClass(cls):
        icebox.resume()
        icebox.detach()

    def test_attach(self):
        pass

    def test_state(self):
        icebox.pause()
        icebox.single_step()
        icebox.resume()
        icebox.resume()
        icebox.pause()

    def test_registers(self):
        for (name, idx) in icebox.register_list():
            print("%s: 0x%x" % (name, idx))
        for name, idx in icebox.msr_list(): 
            print("%s: 0x%x" % (name, idx))

if __name__ == '__main__':
    path = os.path.abspath(sys.argv[1])
    sys.path.append(path)
    sys.argv.pop()
    import icebox
    print()
    unittest.main()