import inspect
import os
import struct
import sys
import unittest


class Windows(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        cls.vm = icebox.Vm("win10")

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

            bpa = self.vm.processes.break_on_create(on_create)
            bpb = self.vm.processes.break_on_delete(on_delete)
            while num_create < 2 or num_delete < 2:
                self.vm.resume()
                self.vm.wait()
            del bpa
            del bpb

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
        self.assertEqual(phy, phy)

        bufc = bytearray(len(bufa))
        self.vm.physical.read(bufc, phy)
        self.assertEqual(bufa, bufc)

        bufd = self.vm.physical[phy: phy + len(bufc)]
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
        moff = "nt!_LDR_DATA_TABLE_ENTRY::SizeOfImage"
        offset = p.symbols.member_offset(moff)
        self.assertGreater(offset, 0)
        proc_ptr = struct.unpack_from("<Q", p.proc)[0]
        print()
        p.symbols.dump_type("nt!_EPROCESS", proc_ptr)

    def test_modules(self):
        p = self.vm.processes.find_name("dwm.exe")
        p.join("user")
        modules = []
        for mod in p.modules():
            modules.append(mod)
            addr, size = mod.span()
            other = p.modules.find(addr)
            self.assertEqual(mod, other)
            other = p.modules.find(addr + size - 1)
            self.assertEqual(mod, other)
            flags = mod.flags()
            self.assertIsNotNone(flags)
        self.assertGreater(len(modules), 1)

    def test_symbols_load(self):
        p = self.vm.processes.find_name("dwm.exe", icebox.kFlags_x64)
        p.join("user")

        kernelbase = p.modules.find_name("kernelbase")
        self.assertIsNotNone(kernelbase)

        addr, size = kernelbase.span()
        p.symbols.load_module_memory(addr, size)

        p.symbols.load_module("kernel32")

        if False:  # too slow
            p.symbols.load_modules()
            bp = p.symbols.autoload_modules()
            self.assertIsNotNone(bp)

    def test_breakpoints(self):
        p = self.vm.processes.wait("dwm.exe", icebox.kFlags_x64)
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
                self.vm.resume()
                self.vm.wait()

        bp = self.vm.break_on("break_on " + name, addr, on_break)
        run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)
        del bp

        bp = self.vm.break_on_process(
            "break_on_process " + name, p, addr, on_break)
        run_until_hit()
        dtb = self.vm.registers.cr3
        t = self.vm.threads.current()
        self.assertEqual(self.vm.registers.rip, addr)
        self.assertEqual(self.vm.processes.current(), p)
        del bp

        bp = self.vm.break_on_thread(
            "break_on_thread " + name, t, addr, on_break)
        run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)
        self.assertEqual(self.vm.threads.current(), t)
        del bp

        phy = p.memory.physical_address(addr)
        bp = self.vm.break_on_physical(
            "break_on_physical " + name, phy, on_break)
        run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)
        del bp

        bp = self.vm.break_on_physical_process(
            "break_on_physical_process " + name, dtb, phy, on_break)
        run_until_hit()
        self.assertEqual(self.vm.registers.rip, addr)
        self.assertEqual(self.vm.processes.current(), p)
        del bp

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

    def test_functions(self):
        name = "nt!SwapContext"
        p = self.vm.processes.current()
        addr = p.symbols.address(name)
        bp = self.vm.break_on(name, addr, lambda: None)
        self.vm.resume()
        self.vm.wait()
        del bp
        stack_0 = self.vm.functions.read_stack(0)
        self.assertIsNotNone(stack_0)
        arg_0 = self.vm.functions.read_arg(0)
        self.assertIsNotNone(arg_0)
        self.vm.functions.write_arg(0, arg_0)
        self.vm.functions.break_on_return(name + " return", lambda: None)
        self.vm.resume()
        self.vm.wait()

    def test_callstacks(self):
        p = self.vm.processes.wait("dwm.exe", icebox.kFlags_x64)
        p.join("user")
        mod = p.modules.find_name("dwm")
        self.assertIsNotNone(mod)
        p.callstacks.load_module(mod)
        mod = p.modules.find_name("ntdll")
        self.assertIsNotNone(mod)
        p.callstacks.load_module(mod)
        name = "nt!NtWaitForMultipleObjects"
        addr = p.symbols.address(name)
        bp = self.vm.break_on_process(name, p, addr, lambda: None)
        self.vm.resume()
        self.vm.wait()
        del bp

        p = self.vm.processes.current()
        addrs = []
        for x in p.callstacks():
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
