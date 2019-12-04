import os
import inspect
import sys
import unittest

class Fixture(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.vm = icebox.Vm("win10")

    @classmethod
    def tearDownClass(cls):
        cls.vm.resume()
        cls.vm.detach()

    def setUp(self):
        self.vm = Fixture.vm

    def test_attach(self):
        pass

    def test_state(self):
        self.vm.pause()
        self.vm.step_once()
        self.vm.resume()
        self.vm.resume()
        self.vm.pause()

    def test_registers(self):
        self.vm.registers.rax += 1
        self.vm.registers.rax -= 1
        print(hex(self.vm.msr.lstar))

if __name__ == '__main__':
    path = os.path.abspath(sys.argv[1])
    sys.path.append(path)
    sys.argv.pop()
    global icebox
    import icebox
    print()
    unittest.main()