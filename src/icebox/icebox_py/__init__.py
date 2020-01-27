import binascii
import os
import struct
import types

from . import _icebox


# magic to attach dynamic properties to a single class instance
def _attach_dynamic_properties(instance, properties):
    class_name = instance.__class__.__name__ + '_'
    child_class = type(class_name, (instance.__class__,), properties)
    instance.__class__ = child_class


class Registers:
    def __init__(self, regs, read, write):
        props = {}
        for name, idx in regs():
            def get_property(idx):
                def fget(_): return read(idx)
                def fset(_, arg): return write(idx, arg)
                return property(fget, fset)
            props[name] = get_property(idx)
        _attach_dynamic_properties(self, props)


class Flags:
    def __init__(self, dict):
        for k, v in dict.items():
            setattr(self, k, v)

    def __repr__(self):
        return str(vars(self))

    def __eq__(self, other):
        return vars(self) == vars(other)


flags_any = Flags({"is_x86": False,  "is_x64": False})
flags_x86 = Flags({"is_x86": True,   "is_x64": False})
flags_x64 = Flags({"is_x86": False,  "is_x64": True})


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
        rpy = ""
        count = 0
        max_count = 256
        while len(buf) > 8 and count < max_count:
            rpy += dump_bytes(buf[:8]) + " "
            buf = buf[8:]
            count += 1
        rpy = rpy[:-1]
        if count == max_count and len(buf) > 8:
            rpy += " ... [%d bytes truncated]" % len(buf)
        return rpy
    return binascii.hexlify(buf).decode()


class BreakpointId:
    def __init__(self, bpid, callback):
        self.bpid = bpid
        self.callback = callback

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        _icebox.drop_breakpoint(self.bpid)
        del self.bpid
        del self.callback


class Symbols:
    def __init__(self, py_proc):
        self.py_proc = py_proc
        self.proc = py_proc.proc

    def address(self, name):
        module, symbol = name.split("!")
        return _icebox.symbols_address(self.proc, module, symbol)

    def strucs(self, module):
        return _icebox.symbols_list_strucs(self.proc, module)

    def struc(self, name):
        module, struc_name = name.split("!")
        ret = _icebox.symbols_read_struc(self.proc, module, struc_name)
        if not ret:
            return ret

        struc = types.SimpleNamespace()
        setattr(struc, "name", ret["name"])
        setattr(struc, "size", ret["bytes"])
        members = []
        for m in ret["members"]:
            item = types.SimpleNamespace()
            setattr(item, "name", m["name"])
            setattr(item, "bits", m["bits"])
            setattr(item, "offset", m["offset"])
            members.append(item)
        setattr(struc, "members", members)
        return struc

    def string(self, ptr):
        return _icebox.symbols_string(self.proc, ptr)

    def load_module_memory(self, addr, size):
        return _icebox.symbols_load_module_memory(self.proc, addr, size)

    def load_module(self, name):
        return _icebox.symbols_load_module(self.proc, name)

    def load_modules(self):
        return _icebox.symbols_load_modules(self.proc)

    def autoload_modules(self):
        bpid = _icebox.symbols_autoload_modules(self.proc)
        return BreakpointId(bpid, None)

    def dump_type(self, name, ptr):
        struc = self.struc(name)
        max_name = 0
        for m in struc.members:
            max_name = max(len(m.name), max_name)
        print("%s %x" % (name, ptr))
        for m in struc.members:
            size = m.bits >> 3
            if not size:
                continue

            buf = self.py_proc.memory[ptr + m.offset: ptr + m.offset + size]
            print("  %3x %s %s" %
                  (m.offset, m.name.ljust(max_name), dump_bytes(buf)))


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
        return _icebox.memory_read_virtual(buf, self.proc, ptr)

    def physical_address(self, ptr):
        return _icebox.memory_virtual_to_physical(self.proc, ptr)

    def __setitem__(self, key, item):
        if isinstance(key, slice):
            return self.write(item, key.start)
        return self.write(struct.pack("B", item), key)

    def write(self, buf, ptr):
        return _icebox.memory_write_virtual(buf, self.proc, ptr)


class Module:
    def __init__(self, proc, mod):
        self.proc = proc
        self.mod = mod

    def __eq__(self, other):
        return self.mod == other.mod

    def __repr__(self):
        addr, size = self.span()
        flags = str(self.flags())
        return "%s: %x-%x flags:%s" % (self.name(), addr, addr + size, flags)

    def name(self):
        return _icebox.modules_name(self.proc, self.mod)

    def span(self):
        return _icebox.modules_span(self.proc, self.mod)

    def flags(self):
        return Flags(_icebox.modules_flags(self.mod))


class Modules:
    def __init__(self, proc):
        self.proc = proc

    def __call__(self):
        for x in _icebox.modules_list(self.proc):
            yield Module(self.proc, x)

    def find(self, addr):
        mod = _icebox.modules_find(self.proc, addr)
        return Module(self.proc, mod) if mod else None

    def find_name(self, name, flags=flags_any):
        mod = _icebox.modules_find_name(self.proc, name, flags)
        return Module(self.proc, mod) if mod else None

    def break_on_create(self, callback, flags=flags_any):
        def fmod(mod): return callback(Module(self.proc, mod))
        bpid = _icebox.modules_listen_create(self.proc, flags, fmod)
        return BreakpointId(bpid, fmod)


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
        bpid = _icebox.callstacks_autoload_modules(self.proc)
        return BreakpointId(bpid, None)


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


class Processes:
    def __init__(self):
        pass

    def __call__(self):
        for x in _icebox.process_list():
            yield Process(x)

    def current(self):
        return Process(_icebox.process_current())

    def find_name(self, name, flags=flags_any):
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

    def wait(self, name, flags=flags_any):
        return Process(_icebox.process_wait(name, flags))

    def break_on_create(self, callback):
        def fproc(proc): return callback(Process(proc))
        bpid = _icebox.process_listen_create(fproc)
        return BreakpointId(bpid, fproc)

    def break_on_delete(self, callback):
        def fproc(proc): return callback(Process(proc))
        bpid = _icebox.process_listen_delete(fproc)
        return BreakpointId(bpid, fproc)


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
        return BreakpointId(bpid, fthread)

    def break_on_delete(self, callback):
        def fthread(thread): return callback(Thread(thread))
        bpid = _icebox.thread_listen_delete(fthread)
        return BreakpointId(bpid, fthread)


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

    def __setitem__(self, key, item):
        if isinstance(key, slice):
            return self.write(item, key.start)
        return self.write(struct.pack("B", item), key)

    def write(self, buf, ptr):
        return _icebox.memory_write_physical(buf, ptr)


class Functions:
    def __init__(self):
        pass

    def read_stack(self, idx):
        return _icebox.functions_read_stack(idx)

    def read_arg(self, idx):
        return _icebox.functions_read_arg(idx)

    def write_arg(self, idx, arg):
        return _icebox.functions_write_arg(idx, arg)

    def break_on_return(self, callback, name=""):
        # TODO do we need to keep ref on callback ?
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
        return BreakpointId(bpid, fdrv)


class KernelSymbols:
    def __init__(self):
        pass

    def load_drivers(self):
        return _icebox.symbols_load_drivers()

    def load_driver(self, name):
        return _icebox.symbols_load_driver(name)


class Vm:
    def __init__(self, name):
        _icebox.attach(name)
        r, w = _icebox.register_read, _icebox.register_write
        self.registers = Registers(_icebox.register_list, r, w)
        r, w = _icebox.msr_read, _icebox.msr_write
        self.msr = Registers(_icebox.msr_list, r, w)
        self.threads = Threads()
        self.processes = Processes()
        self.physical = Physical()
        self.functions = Functions()
        self.drivers = Drivers()
        self.symbols = KernelSymbols()

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

    def exec(self):
        self.resume()
        self.wait()

    def _to_virtual(self, where, get_proc):
        if not isinstance(where, str):
            return where
        proc = get_proc()
        return proc.symbols.address(where)

    def _to_physical(self, where, get_proc):
        if not isinstance(where, str):
            return where
        proc = get_proc()
        addr = proc.symbols.address(where)
        phy = proc.memory.physical_address(addr)
        return phy

    def break_on(self, where, callback, name=""):
        where = self._to_virtual(where, lambda: self.processes.current())
        bp = _icebox.break_on(name, where, callback)
        return BreakpointId(bp, callback)

    def break_on_process(self, proc, where, callback, name=""):
        where = self._to_virtual(where, lambda: proc)
        bp = _icebox.break_on_process(name, proc.proc, where, callback)
        return BreakpointId(bp, callback)

    def break_on_thread(self, thread, where, callback, name=""):
        where = self._to_virtual(where, lambda: where.process())
        bp = _icebox.break_on_thread(name, thread.thread, where, callback)
        return BreakpointId(bp, callback)

    def break_on_physical(self, where, callback, name=""):
        where = self._to_physical(where, lambda: self.processes.current())
        bp = _icebox.break_on_physical(name, where, callback)
        return BreakpointId(bp, callback)

    def break_on_physical_process(self, dtb, where, callback, name=""):
        where = self._to_physical(where, self.processes.current())
        bp = _icebox.break_on_physical_process(name, dtb, where, callback)
        return BreakpointId(bp, callback)


def attach(name):
    return Vm(name)
