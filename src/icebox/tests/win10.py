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
        p = self.vm.processes.current()
        self.assertEqual(p.symbols.string(self.vm.msr.lstar), "nt!KiSystemCall64Shadow")

    def test_processes(self):
        num = 0
        for p in self.vm.processes():
            num += 1
        self.assertGreater(num, 1)

        p = self.vm.processes.find_pid(4)
        self.assertEqual(p.name(), "System")
        self.assertTrue(p.is_valid())
        self.assertEqual(p.pid(), 4)
        flags = p.flags()
        self.assertTrue(flags.is_x64)
        self.assertFalse(flags.is_x86)
        p.join("kernel")

        pa = self.vm.processes.current()
        pb = self.vm.processes.find_name(pa.name(), pa.flags())
        self.assertEqual(pa, pb)

        pd = self.vm.processes.wait("explorer.exe", icebox.kFlags_x64)
        self.assertEqual(pd.name(), "explorer.exe")

        pd.join("kernel")
        pd.join("user")

        pe = self.vm.processes.find_name("services.exe", icebox.kFlags_x64)
        pf = pe.parent()
        self.assertEqual(pf.name(), "wininit.exe")

        global num_create
        num_create = 0
        def on_create(proc):
            self.assertIsNotNone(proc.name())
            self.assertIsNotNone(proc.pid())
            global num_create
            num_create += 1

        global num_delete
        num_delete = 0
        def on_delete(proc):
            self.assertIsNotNone(proc.name())
            self.assertIsNotNone(proc.pid())
            global num_delete
            num_delete += 1

        bpa = self.vm.processes.break_on_create(on_create)
        bpb = self.vm.processes.break_on_delete(on_delete)
        while num_create < 2 and num_delete < 2:
            self.vm.resume()
            self.vm.wait()

    def test_threads(self):
        p = self.vm.processes.current()
        threads = []
        for t in p.threads():
            threads.append(t)
        self.assertGreater(len(threads), 0)

        t = self.vm.threads.current()
        self.assertIn(t, threads)

        pa = t.process()
        self.assertEqual(p, pa)
        self.assertNotEqual(t.program_counter(), 0)
        self.assertNotEqual(t.tid(), 0)

    def test_memory(self):
        rip = self.vm.registers.rip
        p = self.vm.processes.current()

        bufa = bytearray(16)
        p.memory.read(bufa, rip)

        bufb = p.memory[rip : rip + len(bufa)]
        self.assertEqual(bufa, bufb)

        idx = len(bufa) >> 1
        val = p.memory[rip + idx]
        self.assertEqual(val, bufb[idx])

        phy = p.memory.physical_address(rip)
        self.assertEqual(phy, phy)

        bufc = bytearray(len(bufa))
        self.vm.physical.read(bufc, phy)
        self.assertEqual(bufa, bufc)

        bufd = self.vm.physical[phy : phy + len(bufc)]
        self.assertEqual(bufc, bufd)

        val = self.vm.physical[phy + idx]
        self.assertEqual(val, bufc[idx])

    def test_symbols(self):
        p = self.vm.processes.current()
        lstar = self.vm.msr.lstar
        self.assertEqual(p.symbols.string(lstar), "nt!KiSystemCall64Shadow")
        self.assertEqual(lstar, p.symbols.address("nt!KiSystemCall64Shadow"))
        strucs = []
        for s in p.symbols.strucs("nt"):
            strucs.append(s)
        self.assertIn("_LDR_DATA_TABLE_ENTRY", strucs)
        size = p.symbols.struc_size("nt!_LDR_DATA_TABLE_ENTRY")
        self.assertGreater(size, 1)
        members = []
        for m in p.symbols.members("nt!_LDR_DATA_TABLE_ENTRY"):
            members.append(m)
        self.assertIn("SizeOfImage", members)
        offset = p.symbols.member_offset("nt!_LDR_DATA_TABLE_ENTRY::SizeOfImage")
        self.assertGreater(offset, 0)

if __name__ == '__main__':
    path = os.path.abspath(sys.argv[1])
    sys.path.append(path)
    sys.argv.pop()
    global icebox
    import icebox
    print()
    unittest.main()