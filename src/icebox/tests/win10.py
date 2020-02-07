import os
import struct
import sys
import unittest


class Windows(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.vm = icebox.attach("win10")

    @classmethod
    def tearDownClass(cls):
        cls.vm.resume()
        cls.vm.detach()

    def setUp(self):
        self.vm = Windows.vm

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
        lstar = p.symbols.string(self.vm.msr.lstar)
        self.assertEqual(lstar, "nt!KiSystemCall64Shadow")

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
        p.join_kernel()

        pa = self.vm.processes.current()
        pb = self.vm.processes.find_name(pa.name(), pa.flags())
        self.assertEqual(pa, pb)

        pd = self.vm.processes.wait("explorer.exe", icebox.flags_x64)
        self.assertEqual(pd.name(), "explorer.exe")

        pd.join_kernel()
        pd.join_user()

        pe = self.vm.processes.find_name("services.exe", icebox.flags_x64)
        pf = pe.parent()
        self.assertEqual(pf.name(), "wininit.exe")

        if False:  # too slow
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

            with self.vm.processes.break_on_create(on_create):
                with self.vm.processes.break_on_delete(on_delete):
                    while num_create < 2 or num_delete < 2:
                        self.vm.exec()

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

        bufb = p.memory[rip: rip + len(bufa)]
        self.assertEqual(bufa, bufb)

        idx = len(bufa) >> 1
        val = p.memory[rip + idx]
        self.assertEqual(val, bufb[idx])

        phy = p.memory.physical_address(rip)
        self.assertIsNotNone(phy)

        bufc = bytearray(len(bufa))
        self.vm.physical.read(bufc, phy)
        self.assertEqual(bufa, bufc)

        bufd = self.vm.physical[phy: phy + len(bufc)]
        self.assertEqual(bufc, bufd)

        val = self.vm.physical[phy + idx]
        self.assertEqual(val, bufc[idx])

    def test_memory_writes(self):
        rip = self.vm.registers.rip
        p = self.vm.processes.current()
        backup = p.memory[rip]
        p.memory[rip] = 0xcc
        val = p.memory[rip]
        self.assertEqual(val, 0xcc)
        p.memory[rip] = backup

        backup = p.memory[rip: rip + 16]
        zero = b'\x00' * len(backup)
        p.memory[rip: rip + len(backup)] = zero
        val = p.memory[rip: rip + len(backup)]
        self.assertEqual(zero, val)
        p.memory.write(rip, backup)
        val = p.memory[rip: rip + len(backup)]
        self.assertEqual(val, backup)

        phy = p.memory.physical_address(rip)
        self.assertIsNotNone(phy)

        phy_backup = self.vm.physical[phy: phy + 16]
        self.assertEqual(backup, phy_backup)
        zero = b'\x00' * len(backup)
        self.vm.physical[phy: phy + len(backup)] = zero
        val = self.vm.physical[phy: phy + len(backup)]
        self.assertEqual(zero, val)
        self.vm.physical.write(phy, backup)
        val = self.vm.physical[phy: phy + len(backup)]
        self.assertEqual(val, backup)

    def test_symbols(self):
        p = self.vm.processes.current()
        lstar = self.vm.msr.lstar
        self.assertEqual(p.symbols.string(lstar), "nt!KiSystemCall64Shadow")
        self.assertEqual(lstar, p.symbols.address("nt!KiSystemCall64Shadow"))
        strucs = []
        for s in p.symbols.strucs("nt"):
            strucs.append(s)
        self.assertIn("_LDR_DATA_TABLE_ENTRY", strucs)
        struc = p.symbols.struc("nt!_LDR_DATA_TABLE_ENTRY")
        self.assertGreater(struc.size, 1)
        members = []
        offset = None
        for m in struc.members:
            members.append(m.name)
            if m.name == "SizeOfImage":
                offset = m.offset
        self.assertIn("SizeOfImage", members)
        self.assertIsNotNone(offset)
        self.assertGreater(offset, 0)
        proc_ptr = struct.unpack_from("<Q", p.proc)[0]
        print()
        p.symbols.dump_type("nt!_EPROCESS", proc_ptr)

    def test_modules(self):
        p = self.vm.processes.find_name("dwm.exe")
        p.join_user()
        modules = []
        for mod in p.modules():
            modules.append(mod)
            addr, size = mod.span()
            header = p.memory[addr: addr + 2]
            self.assertEqual(header, b'MZ')
            other = p.modules.find(addr)
            self.assertEqual(mod, other)
            other = p.modules.find(addr + size - 1)
            self.assertEqual(mod, other)
            flags = mod.flags()
            self.assertIsNotNone(flags)
        self.assertGreater(len(modules), 1)

    def test_symbols_load(self):
        p = self.vm.processes.find_name("dwm.exe", icebox.flags_x64)
        p.join_user()

        kernelbase = p.modules.find_name("kernelbase.dll", icebox.flags_any)
        self.assertIsNotNone(kernelbase)

        addr, size = kernelbase.span()
        p.symbols.load_module_memory(addr, size)

        p.symbols.load_module("kernel32")

        if False:  # too slow
            p.symbols.load_modules()
            with p.symbols.autoload_modules() as bp:
                self.assertIsNotNone(bp)

    def test_breakpoints(self):
        p = self.vm.processes.wait("dwm.exe", icebox.flags_x64)
        name = "ntdll!NtWaitForMultipleObjects"
        addr = p.symbols.address(name)

        global hit
        hit = 0

        def on_break():
            global hit
            hit += 1

        def run_until_hit():
            global hit
            hit = 0
            while hit == 0:
                self.vm.exec()

        with self.vm.break_on(addr, on_break):
            run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)

        with self.vm.break_on_process(p, addr, on_break):
            run_until_hit()
        dtb = self.vm.registers.cr3
        t = self.vm.threads.current()
        self.assertEqual(self.vm.registers.rip, addr)
        self.assertEqual(self.vm.processes.current(), p)

        with self.vm.break_on_thread(t, addr, on_break):
            run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)
        self.assertEqual(self.vm.threads.current(), t)

        phy = p.memory.physical_address(addr)
        with self.vm.break_on_physical(phy, on_break):
            run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)

        with self.vm.break_on_physical_process(dtb, phy, on_break):
            run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)
        self.assertEqual(self.vm.processes.current(), p)

    def test_drivers(self):
        drivers = []
        for drv in self.vm.drivers():
            drivers.append(drv)
            addr, size = drv.span()
            other = self.vm.drivers.find(addr)
            self.assertEqual(drv, other)
            other = self.vm.drivers.find(addr + size - 1)
            self.assertEqual(drv, other)
        self.assertGreater(len(drivers), 1)
        self.vm.symbols.load_driver("hal")

    def test_functions(self):
        name = "nt!SwapContext"
        p = self.vm.processes.current()
        addr = p.symbols.address(name)
        with self.vm.break_on(addr, lambda: None):
            self.vm.exec()
        stack_0 = self.vm.functions.read_stack(0)
        self.assertIsNotNone(stack_0)
        arg_0 = self.vm.functions.args[0]
        self.assertIsNotNone(arg_0)
        self.vm.functions.args[0] = arg_0
        args = self.vm.functions.args[1: 3]
        self.vm.functions.args[1:] = args
        self.vm.functions.break_on_return(lambda: None)
        self.vm.exec()

    def test_callstacks(self):
        p = self.vm.processes.wait("dwm.exe", icebox.flags_any)
        p.join_user()
        mod = p.modules.find_name("dwm.exe", icebox.flags_x64)
        self.assertIsNotNone(mod)
        p.callstack.load_module(mod)
        mod = p.modules.find_name("ntdll.dll", icebox.flags_x64)
        self.assertIsNotNone(mod)
        p.callstack.load_module(mod)
        name = "nt!NtWaitForMultipleObjects"
        addr = p.symbols.address(name)
        with self.vm.break_on_process(p, addr, lambda: None):
            self.vm.exec()

        p = self.vm.processes.current()
        addrs = []
        for x in p.callstack():
            addrs.append(x)
        self.assertGreater(len(addrs), 1)

    def test_vma(self):
        p = self.vm.processes.current()
        for vma in p.vm_areas():
            _, _ = vma.span()


if __name__ == '__main__':
    path = os.path.abspath(sys.argv[1])
    sys.path.append(path)
    del sys.argv[1]
    global icebox
    import icebox
    unittest.main()
