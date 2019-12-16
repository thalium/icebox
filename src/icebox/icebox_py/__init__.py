import binascii
import os
import struct

from . import _icebox


# voodoo magic to attach dynamic properties to a single class instance
def _attach_dynamic_property(instance, name, propr):
    class_name = instance.__class__.__name__ + '_'
    child_class = type(class_name, (instance.__class__,), {name: propr})
    instance.__class__ = child_class


class Registers:
    def __init__(self, regs, read, write):
        for name, idx in regs():
            def get_property(idx):
                def fget(_): return read(idx)
                def fset(_, arg): return write(idx, arg)
                return property(fget, fset)
            _attach_dynamic_property(self, name, get_property(idx))


class Flags:
    def __init__(self, dict):
        for k, v in dict.items():
            setattr(self, k, v)


kFlags_x86 = Flags({"is_x86": True,  "is_x64": False})
kFlags_x64 = Flags({"is_x86": False, "is_x64": True})


def dump_bytes(buf):
    if len(buf) == 1:
        return hex(struct.unpack_from("<B", buf)[0])[2:]
    if len(buf) == 2:
        return hex(struct.unpack_from("<H", buf)[0])[2:]
    if len(buf) == 4:
        return hex(struct.unpack_from("<I", buf)[0])[2:]
    if len(buf) == 8:
        return hex(struct.unpack_from("<Q", buf)[0])[2:]
    if len(buf) > 8:
        return dump_bytes(buf[:8]) + " " + dump_bytes(buf[8:])
    return binascii.hexlify(buf).decode()


class Symbols:
    def __init__(self, py_proc):
        self.py_proc = py_proc
        self.proc = py_proc.proc

    def address(self, name):
        module, symbol = name.split("!")
        return _icebox.symbols_address(self.proc, module, symbol)

    def strucs(self, module):
        return _icebox.symbols_struc_names(self.proc, module)

    def struc_size(self, name):
        module, struc_name = name.split("!")
        return _icebox.symbols_struc_size(self.proc, module, struc_name)

    def members(self, name):
        module, struc = name.split("!")
        return _icebox.symbols_struc_members(self.proc, module, struc)

    def member_offset(self, name):
        module, struc = name.split("!")
        struc_name, struc_member = struc.split("::")
        return _icebox.symbols_member_offset(self.proc, module, struc_name, struc_member)

    def string(self, ptr):
        return _icebox.symbols_string(self.proc, ptr)

    def load_module_memory(self, addr, size):
        return _icebox.symbols_load_module_memory(self.proc, addr, size)

    def load_module(self, name):
        return _icebox.symbols_load_module(self.proc, name)

    def load_modules(self):
        return _icebox.symbols_load_modules(self.proc)

    def autoload_modules(self):
        return _icebox.symbols_autoload_modules(self.proc)

    def dump_type(self, name, ptr):
        size = self.struc_size(name)
        members = [(m, self.member_offset("%s::%s" % (name, m)), 0)
                   for m in self.members(name)]
        last_offset = size
        num_members = len(members)
        max_name = 0
        for i, (mname, offset, _) in enumerate(reversed(members)):
            if offset >= last_offset:
                continue  # union types...
            max_name = max(len(mname), max_name)
            members[num_members - 1 - i] = mname, offset, last_offset - offset
            last_offset = offset
        print("%s %x" % (name, ptr))
        for mname, offset, msize in members:
            if msize == 0:
                continue
            buf = self.py_proc.memory[ptr + offset: ptr + offset + msize]
            print("  %3x %s %s" %
                  (offset, mname.ljust(max_name), dump_bytes(buf)))


class Virtual:
    def __init__(self, proc):
        self.proc = proc

    def __getitem__(self, key):
        if isinstance(key, slice):
            buf = bytearray(key.stop - key.start)
            self.read(buf, key.start)
            return buf

        buf = bytearray(1)
        self.read(buf, key)
        return buf[0]

    def read(self, buf, ptr):
        return _icebox.memory_read_virtual_process(buf, self.proc, ptr)

    def physical_address(self, ptr):
        return _icebox.memory_virtual_to_physical(self.proc, ptr)


class Module:
    def __init__(self, proc, mod):
        self.proc = proc
        self.mod = mod

    def __eq__(self, other):
        return self.mod == other.mod

    def __repr__(self):
        addr, size = self.span()
        return "%s: %x-%x flags:%s" % (self.name(), addr, addr + size, str(self.flags()))

    def name(self):
        return _icebox.modules_name(self.proc, self.mod)

    def span(self):
        return _icebox.modules_span(self.proc, self.mod)

    def flags(self):
        return _icebox.modules_flags(self.mod)


class Modules:
    def __init__(self, proc):
        self.proc = proc

    def __call__(self):
        for x in _icebox.modules_list(self.proc):
            yield Module(self.proc, x)

    def find(self, addr):
        mod = _icebox.modules_find(self.proc, addr)
        return Module(self.proc, mod) if mod else None

    def find_name(self, name):
        name = name.casefold()
        for mod in self():
            mod_name, _ = os.path.splitext(os.path.basename(mod.name()))
            if mod_name.casefold() == name:
                return mod
        return None

    def break_on_create(self, flags, callback):
        def fmod(mod): return callback(Module(self.proc, mod))
        bpid = _icebox.modules_listen_create(self.proc, flags, fmod)
        return Callback(bpid, fmod)


class Callstacks:
    def __init__(self, proc):
        self.proc = proc

    def __call__(self):
        for x in _icebox.callstacks_read(self.proc, 256):
            yield x

    def load_module(self, mod):
        return _icebox.callstacks_load_module(self.proc, mod.mod)

    def load_driver(self, drv):
        return _icebox.callstacks_load_driver(self.proc, drv.drv)

    def autoload_modules(self):
        return _icebox.callstacks_autoload_modules(self.proc)


class VmArea:
    def __init__(self, proc, vma):
        self.proc = proc
        self.vma = vma

    def span(self):
        return _icebox.vm_area_span(self.proc, self.vma)


class VmAreas:
    def __init__(self, proc):
        self.proc = proc

    def __call__(self):
        for x in _icebox.vm_area_list(self.proc):
            yield VmArea(self.proc, x)


class Process:
    def __init__(self, proc):
        self.proc = proc
        self.symbols = Symbols(self)
        self.memory = Virtual(proc)
        self.modules = Modules(proc)
        self.callstacks = Callstacks(proc)
        self.vm_areas = VmAreas(proc)

    def __eq__(self, other):
        return self.proc == other.proc

    def __repr__(self):
        return "%s pid:%d" % (self.name(), self.pid())

    def name(self):
        return _icebox.process_name(self.proc)

    def is_valid(self):
        return _icebox.process_is_valid(self.proc)

    def pid(self):
        return _icebox.process_pid(self.proc)

    def flags(self):
        return Flags(_icebox.process_flags(self.proc))

    def join(self, mode):
        if mode != "kernel" and mode != "user":
            raise BaseException("invalid join mode")

        return _icebox.process_join(self.proc, mode)

    def parent(self):
        ret = _icebox.process_parent(self.proc)
        return Process(ret) if ret else None

    def threads(self):
        for x in _icebox.thread_list(self.proc):
            yield Thread(x)


class Callback:
    def __init__(self, bpid, callback):
        self.bpid = bpid
        self.callback = callback


class Processes:
    def __init__(self):
        pass

    def __call__(self):
        for x in _icebox.process_list():
            yield Process(x)

    def current(self):
        return Process(_icebox.process_current())

    def find_name(self, name, flags=None):
        for p in self():
            got_name = os.path.basename(p.name())
            if got_name != name:
                continue

            got_flags = p.flags()
            if flags and flags.is_x64 and not got_flags.is_x64:
                continue

            if flags and flags.is_x86 and not got_flags.is_x86:
                continue

            return p
        return None

    def find_pid(self, pid):
        for p in self():
            if p.pid() == pid:
                return p
        return None

    def wait(self, name, flags):
        return Process(_icebox.process_wait(name, flags))

    def break_on_create(self, callback):
        def fproc(proc): return callback(Process(proc))
        bpid = _icebox.process_listen_create(fproc)
        return Callback(bpid, fproc)

    def break_on_delete(self, callback):
        def fproc(proc): return callback(Process(proc))
        bpid = _icebox.process_listen_delete(fproc)
        return Callback(bpid, fproc)


class Thread:
    def __init__(self, thread):
        self.thread = thread

    def __eq__(self, other):
        return self.thread == other.thread

    def __repr__(self):
        return "%s tid:%d" % (self.process().name(), self.tid())

    def process(self):
        return Process(_icebox.thread_process(self.thread))

    def program_counter(self):
        return _icebox.thread_program_counter(self.thread)

    def tid(self):
        return _icebox.thread_tid(self.thread)


class Threads:
    def __init__(self):
        pass

    def current(self):
        return Thread(_icebox.thread_current())

    def break_on_create(self, callback):
        def fthread(thread): return callback(Thread(thread))
        bpid = _icebox.thread_listen_create(fthread)
        return Callback(bpid, fthread)

    def break_on_delete(self, callback):
        def fthread(thread): return callback(Thread(thread))
        bpid = _icebox.thread_listen_delete(fthread)
        return Callback(bpid, fthread)


class Physical:
    def __init__(self):
        pass

    def __getitem__(self, key):
        if isinstance(key, slice):
            buf = bytearray(key.stop - key.start)
            self.read(buf, key.start)
            return buf
        buf = bytearray(1)
        self.read(buf, key)
        return buf[0]

    def read(self, buf, ptr):
        return _icebox.memory_read_physical(buf, ptr)


class Functions:
    def __init__(self):
        pass

    def read_stack(self, idx):
        return _icebox.functions_read_stack(idx)

    def read_arg(self, idx):
        return _icebox.functions_read_arg(idx)

    def write_arg(self, idx, arg):
        return _icebox.functions_write_arg(idx, arg)

    def break_on_return(self, name, callback):
        return _icebox.functions_break_on_return(name, callback)


class Driver:
    def __init__(self, drv):
        self.drv = drv

    def __eq__(self, other):
        return self.drv == other.drv

    def __repr__(self):
        addr, size = self.span()
        return "%s: %x-%x" % (self.name(), addr, addr + size)

    def name(self):
        return _icebox.drivers_name(self.drv)

    def span(self):
        return _icebox.drivers_span(self.drv)


class Drivers:
    def __init__(self):
        pass

    def __call__(self):
        for x in _icebox.drivers_list():
            yield Driver(x)

    def find(self, addr):
        drv = _icebox.drivers_find(addr)
        return Driver(drv) if drv else None

    def find_name(self, name):
        name = name.casefold()
        for drv in self():
            drv_name, _ = os.path.splitext(os.path.basename(drv.name()))
            if drv_name.casefold() == name:
                return drv
        return None

    def break_on(self, callback):
        def fdrv(drv, load): return callback(Driver(drv), load)
        bpid = _icebox.drivers_listen(fdrv)
        return Callback(bpid, fdrv)


class Vm:
    def __init__(self, name):
        _icebox.attach(name)
        self.registers = Registers(
            _icebox.register_list, _icebox.register_read, _icebox.register_write)
        self.msr = Registers(
            _icebox.msr_list, _icebox.msr_read, _icebox.msr_write)
        self.threads = Threads()
        self.processes = Processes()
        self.physical = Physical()
        self.functions = Functions()
        self.drivers = Drivers()

    def detach(self):
        _icebox.detach()

    def resume(self):
        _icebox.resume()

    def pause(self):
        _icebox.pause()

    def step_once(self):
        _icebox.single_step()

    def wait(self):
        _icebox.wait()

    def break_on(self, name, where, callback):
        bp = _icebox.break_on(name, where, callback)
        return Callback(bp, callback)

    def break_on_process(self, name, proc, where, callback):
        bp = _icebox.break_on_process(name, proc.proc, where, callback)
        return Callback(bp, callback)

    def break_on_thread(self, name, thread, where, callback):
        bp = _icebox.break_on_thread(name, thread.thread, where, callback)
        return Callback(bp, callback)

    def break_on_physical(self, name, where, callback):
        bp = _icebox.break_on_physical(name, where, callback)
        return Callback(bp, callback)

    def break_on_physical_process(self, name, dtb, where, callback):
        bp = _icebox.break_on_physical_process(name, dtb, where, callback)
        return Callback(bp, callback)
