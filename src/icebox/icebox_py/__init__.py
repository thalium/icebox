import binascii
import os
import struct
import types

from . import libicebox


# magic to attach dynamic properties to a single class instance
def _attach_dynamic_properties(instance, properties):
    class_name = instance.__class__.__name__ + "_"
    child_class = type(class_name, (instance.__class__,), properties)
    instance.__class__ = child_class


class Number(int):
    def __repr__(self):
        return hex(self)


class Registers:
    def __init__(self, regs, read, write):
        props = {}
        self.regs = []
        self.read = read
        for name, idx in regs():

            def get_property(idx):
                def fget(_):
                    return Number(read(idx))

                def fset(_, arg):
                    return write(idx, arg)

                return property(fget, fset)

            props[name] = get_property(idx)
            self.regs.append((name, idx))
        _attach_dynamic_properties(self, props)

    def __call__(self):
        return [x for x, _ in self.regs]

    def dump(self):
        """Dump all registers."""
        for name, idx in self.regs:
            print("%3s 0x%x" % (name, self.read(idx)))


class Flags:
    def __init__(self, dict):
        for k, v in dict.items():
            setattr(self, k, v)

    def __repr__(self):
        return str(vars(self))

    def __eq__(self, other):
        return vars(self) == vars(other)


flags_any = Flags({"is_x86": False, "is_x64": False})
flags_x86 = Flags({"is_x86": True, "is_x64": False})
flags_x64 = Flags({"is_x86": False, "is_x64": True})


def dump_bytes(buf):
    """Dump bytes from input buffer."""
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
        libicebox.drop_breakpoint(self.bpid)
        del self.bpid
        del self.callback


class Symbols:
    def __init__(self, py_proc):
        self.py_proc = py_proc
        self.proc = py_proc.proc

    def address(self, name):
        """Convert symbol to process virtual address."""
        module, symbol = name.split("!")
        return libicebox.symbols_address(self.proc, module, symbol)

    def strucs(self, module):
        """List all structure names from process module name."""
        return libicebox.symbols_list_strucs(self.proc, module)

    def struc(self, name):
        """Read structure type from name."""
        module, struc_name = name.split("!")
        ret = libicebox.symbols_read_struc(self.proc, module, struc_name)
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
        """Convert process virtual memory address to symbol string."""
        return libicebox.symbols_string(self.proc, ptr)

    def load_module_memory(self, addr, size):
        """Load symbols from virtual memory address and size."""
        return libicebox.symbols_load_module_memory(self.proc, addr, size)

    def load_module(self, name):
        """Load symbols from process module name."""
        return libicebox.symbols_load_module(self.proc, name)

    def load_modules(self):
        """Load symbols from all process modules."""
        return libicebox.symbols_load_modules(self.proc)

    def autoload_modules(self):
        """Return an object auto-loading symbols from process modules."""
        bpid = libicebox.symbols_autoload_modules(self.proc)
        return BreakpointId(bpid, None)

    def dump_type(self, name, ptr):
        """Dump type at virtual memory address."""
        struc = self.struc(name)
        max_name = 0
        for m in struc.members:
            max_name = max(len(m.name), max_name)
        print("%s %x" % (name, ptr))
        for m in struc.members:
            size = m.bits >> 3
            if not size:
                continue

            buf = self.py_proc.memory[ptr + m.offset : ptr + m.offset + size]
            print("  %3x %s %s" % (m.offset, m.name.ljust(max_name), dump_bytes(buf)))


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
        """Read buffer from virtual memory."""
        return libicebox.memory_read_virtual(buf, self.proc, ptr)

    def physical_address(self, ptr):
        """Convert virtual memory address to physical address."""
        return libicebox.memory_virtual_to_physical(self.proc, ptr)

    def __setitem__(self, key, item):
        if isinstance(key, slice):
            return self.write(key.start, item)
        return self.write(key, struct.pack("B", item))

    def write(self, ptr, buf):
        """Write buffer to virtual memory."""
        return libicebox.memory_write_virtual(buf, self.proc, ptr)


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
        """Return module name."""
        return libicebox.modules_name(self.proc, self.mod)

    def span(self):
        """Return module base address and size."""
        return libicebox.modules_span(self.proc, self.mod)

    def flags(self):
        """Return module flags."""
        return Flags(libicebox.modules_flags(self.mod))


class Modules:
    def __init__(self, proc):
        self.proc = proc

    def __call__(self):
        """List all process modules."""
        for x in libicebox.modules_list(self.proc):
            yield Module(self.proc, x)

    def find(self, addr):
        """Find module by address."""
        mod = libicebox.modules_find(self.proc, addr)
        return Module(self.proc, mod) if mod else None

    def find_name(self, name, flags=flags_any):
        """Find module by name."""
        mod = libicebox.modules_find_name(self.proc, name, flags)
        return Module(self.proc, mod) if mod else None

    def break_on_create(self, callback, flags=flags_any):
        """Return breakpoint on modules creation."""

        def fmod(mod):
            return callback(Module(self.proc, mod))

        bpid = libicebox.modules_listen_create(self.proc, flags, fmod)
        return BreakpointId(bpid, fmod)


class Callstack:
    def __init__(self, proc):
        self.proc = proc

    def __call__(self):
        """List callstack addresses."""
        for x in libicebox.callstacks_read(self.proc, 256):
            yield x

    def load_module(self, mod):
        """Load unwind data from module."""
        return libicebox.callstacks_load_module(self.proc, mod.mod)

    def load_driver(self, drv):
        """Load unwind data from driver."""
        return libicebox.callstacks_load_driver(self.proc, drv.drv)

    def autoload_modules(self):
        """Return an object auto-loading unwind data from modules."""
        bpid = libicebox.callstacks_autoload_modules(self.proc)
        return BreakpointId(bpid, None)


class VmArea:
    def __init__(self, proc, vma):
        self.proc = proc
        self.vma = vma

    def span(self):
        """Return vma base address and size."""
        return libicebox.vm_area_span(self.proc, self.vma)


class VmAreas:
    def __init__(self, proc):
        self.proc = proc

    def __call__(self):
        """List all current active virtual memory areas."""
        for x in libicebox.vm_area_list(self.proc):
            yield VmArea(self.proc, x)


class Process:
    def __init__(self, proc):
        self.proc = proc
        self.symbols = Symbols(self)
        self.memory = Virtual(proc)
        self.modules = Modules(proc)
        self.callstack = Callstack(proc)
        self.vm_areas = VmAreas(proc)

    def __eq__(self, other):
        return self.proc == other.proc

    def __repr__(self):
        return "%s pid:%d" % (self.name(), self.pid())

    def native(self):
        """Return native process handle."""
        return libicebox.process_native(self.proc)

    def kdtb(self):
        """Return kernel Directory Table Base."""
        return libicebox.process_kdtb(self.proc)

    def udtb(self):
        """Return user Directory Table Base."""
        return libicebox.process_udtb(self.proc)

    def name(self):
        """Return process name."""
        return libicebox.process_name(self.proc)

    def is_valid(self):
        """Return whether this process is valid."""
        return libicebox.process_is_valid(self.proc)

    def pid(self):
        """Return process identifier."""
        return libicebox.process_pid(self.proc)

    def flags(self):
        """Return process flags."""
        return Flags(libicebox.process_flags(self.proc))

    def parent(self):
        """Return parent process."""
        ret = libicebox.process_parent(self.proc)
        return Process(ret) if ret else None

    def threads(self):
        """List all process threads."""
        for x in libicebox.thread_list(self.proc):
            yield Thread(x)


class Processes:
    def __init__(self):
        pass

    def __call__(self):
        """List all current processes."""
        for x in libicebox.process_list():
            yield Process(x)

    def current(self):
        """Return current active process."""
        return Process(libicebox.process_current())

    def find_name(self, name, flags=flags_any):
        """Find process by name."""
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
        """Find process by PID."""
        for p in self():
            if p.pid() == pid:
                return p
        return None

    def wait(self, name, flags=flags_any):
        """Return or wait for process to start from name."""
        return Process(libicebox.process_wait(name, flags))

    def break_on_create(self, callback):
        """Return breakpoint on process creation."""

        def fproc(proc):
            return callback(Process(proc))

        bpid = libicebox.process_listen_create(fproc)
        return BreakpointId(bpid, fproc)

    def break_on_delete(self, callback):
        """Return breakpoint on process deletion."""

        def fproc(proc):
            return callback(Process(proc))

        bpid = libicebox.process_listen_delete(fproc)
        return BreakpointId(bpid, fproc)


class Thread:
    def __init__(self, thread):
        self.thread = thread

    def __eq__(self, other):
        return self.thread == other.thread

    def __repr__(self):
        return "%s tid:%d" % (self.process().name(), self.tid())

    def process(self):
        """Return thread processus."""
        return Process(libicebox.thread_process(self.thread))

    def program_counter(self):
        """Return thread program counter."""
        return libicebox.thread_program_counter(self.thread)

    def tid(self):
        """Return thread id."""
        return libicebox.thread_tid(self.thread)


class Threads:
    def __init__(self):
        pass

    def current(self):
        """Return current active thread."""
        return Thread(libicebox.thread_current())

    def break_on_create(self, callback):
        """Return breakpoint on thread creation."""

        def fthread(thread):
            return callback(Thread(thread))

        bpid = libicebox.thread_listen_create(fthread)
        return BreakpointId(bpid, fthread)

    def break_on_delete(self, callback):
        """Return breakpoint on thread deletion."""

        def fthread(thread):
            return callback(Thread(thread))

        bpid = libicebox.thread_listen_delete(fthread)
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
        """Read physical memory to buffer."""
        return libicebox.memory_read_physical(buf, ptr)

    def __setitem__(self, key, item):
        if isinstance(key, slice):
            return self.write(key.start, item)
        return self.write(key, struct.pack("B", item))

    def write(self, ptr, buf):
        """Write buffer to physical memory."""
        return libicebox.memory_write_physical(buf, ptr)


class Args:
    def __init__(self):
        pass

    def __getitem__(self, key):
        if isinstance(key, slice):
            args = []
            start = key.start if key.start else 0
            for i in range(start, key.stop):
                args.append(libicebox.functions_read_arg(i))
            return args
        return libicebox.functions_read_arg(key)

    def __setitem__(self, key, item):
        if isinstance(key, slice):
            start = key.start if key.start else 0
            for i in range(start, start + len(item)):
                libicebox.functions_write_arg(i, item[i - start])
            return
        return libicebox.functions_write_arg(key, item)


class Functions:
    def __init__(self):
        self.args = Args()

    def read_stack(self, idx):
        """Read indexed stack value."""
        return libicebox.functions_read_stack(idx)

    def read_arg(self, idx):
        """Read indexed function argument."""
        return libicebox.functions_read_arg(idx)

    def write_arg(self, idx, arg):
        """Write indexed function argument."""
        return libicebox.functions_write_arg(idx, arg)

    def break_on_return(self, callback, name=""):
        """Set a single-use breakpoint callback on function return."""
        # TODO do we need to keep ref on callback ?
        return libicebox.functions_break_on_return(name, callback)


class Driver:
    def __init__(self, drv):
        self.drv = drv

    def __eq__(self, other):
        return self.drv == other.drv

    def __repr__(self):
        addr, size = self.span()
        return "%s: %x-%x" % (self.name(), addr, addr + size)

    def name(self):
        """Return driver name."""
        return libicebox.drivers_name(self.drv)

    def span(self):
        """Return driver base address and size."""
        return libicebox.drivers_span(self.drv)


class Drivers:
    def __init__(self):
        pass

    def __call__(self):
        """List all current drivers."""
        for x in libicebox.drivers_list():
            yield Driver(x)

    def find(self, addr):
        """Find driver from address."""
        drv = libicebox.drivers_find(addr)
        return Driver(drv) if drv else None

    def find_name(self, name):
        """Find driver from name."""
        name = name.casefold()
        for drv in self():
            drv_name, _ = os.path.splitext(os.path.basename(drv.name()))
            if drv_name.casefold() == name:
                return drv
        return None

    def break_on(self, callback):
        """Return breakpoint on driver load and unload."""

        def fdrv(drv, load):
            return callback(Driver(drv), load)

        bpid = libicebox.drivers_listen(fdrv)
        return BreakpointId(bpid, fdrv)


class KernelSymbols:
    def __init__(self):
        pass

    def load_drivers(self):
        """Load symbols from all drivers."""
        return libicebox.symbols_load_drivers()

    def load_driver(self, name):
        """Load symbols from named driver."""
        return libicebox.symbols_load_driver(name)


def _get_default_logger():
    import logging

    logging.basicConfig(
        format="%(asctime)s:%(levelname)s %(message)s", level=logging.INFO
    )

    def log_it(level, msg):
        if level == 0:
            logging.info(msg)
        elif level == 1:
            logging.error(msg)

    return log_it


class Vm:
    def __init__(self, name, attach_only=False, get_logger=_get_default_logger):
        libicebox.log_redirect(get_logger())
        if attach_only:
            libicebox.attach_only(name)
        else:
            libicebox.attach(name)
        r, w = libicebox.register_read, libicebox.register_write
        self.registers = Registers(libicebox.register_list, r, w)
        r, w = libicebox.msr_read, libicebox.msr_write
        self.msr = Registers(libicebox.msr_list, r, w)
        self.threads = Threads()
        self.processes = Processes()
        self.physical = Physical()
        self.functions = Functions()
        self.drivers = Drivers()
        self.symbols = KernelSymbols()

    def detach(self):
        """Detach from vm."""
        libicebox.detach()

    def detect(self):
        """Detect OS."""
        libicebox.detect()

    def resume(self):
        """Resume vm."""
        libicebox.resume()

    def pause(self):
        """Pause vm."""
        libicebox.pause()

    def step_once(self):
        """Execute a single instruction."""
        libicebox.single_step()

    def wait(self):
        """Wait for vm to break."""
        libicebox.wait()

    def exec(self):
        """Execute vm once until next break."""
        self.resume()
        self.wait()

    def interrupt(self):
        """Interrupt vm execution."""
        libicebox.interrupt()

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
        """Return breakpoint on virtual address."""
        where = self._to_virtual(where, lambda: self.processes.current())
        bp = libicebox.break_on(name, where, callback)
        return BreakpointId(bp, callback)

    def break_on_process(self, proc, where, callback, name=""):
        """Return breakpoint on virtual address filtered by process."""
        where = self._to_virtual(where, lambda: proc)
        bp = libicebox.break_on_process(name, proc.proc, where, callback)
        return BreakpointId(bp, callback)

    def break_on_thread(self, thread, where, callback, name=""):
        """Return breakpoint on virtual address filtered by thread."""
        where = self._to_virtual(where, lambda: where.process())
        bp = libicebox.break_on_thread(name, thread.thread, where, callback)
        return BreakpointId(bp, callback)

    def break_on_physical(self, where, callback, name=""):
        """Return breakpoint on physical address."""
        where = self._to_physical(where, lambda: self.processes.current())
        bp = libicebox.break_on_physical(name, where, callback)
        return BreakpointId(bp, callback)

    def break_on_physical_process(self, dtb, where, callback, name=""):
        """Return breakpoint on physical address filtered by DTB."""
        where = self._to_physical(where, self.processes.current())
        bp = libicebox.break_on_physical_process(name, dtb, where, callback)
        return BreakpointId(bp, callback)


def attach(name):
    """Attach to live VM by name."""
    return Vm(name)


def attach_only(name):
    """Attach to live VM by name without os detection."""
    return Vm(name, attach_only=True)


class Counter:
    def __init__(self):
        """Initialize a new counter."""
        self.count = 0

    def add(self, arg=1):
        """Add arg value to counter."""
        self.count += arg

    def read(self):
        """Read current counter value."""
        return self.count


def counter():
    """Return a new counter object."""
    return Counter()
