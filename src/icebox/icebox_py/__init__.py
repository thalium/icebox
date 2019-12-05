import enum
import inspect
import os
import sys

# voodoo magic to attach dynamic properties to a single class instance
def _attach_dynamic_property(instance, name, propr):
    class_name = instance.__class__.__name__ + '_'
    child_class = type(class_name, (instance.__class__,), {name: propr})
    instance.__class__ = child_class

class Registers:
    def __init__(self, regs, read, write):
        for name, idx in regs():
            def get_property(idx):
                fget = lambda _: read(idx)
                fset = lambda _, arg: write(idx, arg)
                return property(fget, fset)
            _attach_dynamic_property(self, name, get_property(idx))

class Flags:
    def __init__(self, dict):
        for k, v in dict.items():
            setattr(self, k, v)

kFlags_x86 = Flags({"is_x86": True,  "is_x64": False})
kFlags_x64 = Flags({"is_x86": False, "is_x64": True})

class Process:
    def __init__(self, proc):
        self.proc = proc

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

class Callback:
    def __init__(self, bpid, callback):
        self.bpid = bpid
        self.callback = callback

class Processes:
    def __init__(self, vm):
        self.vm = vm

    def list_all(self):
        for x in _icebox.process_list():
            yield Process(x)

    def current(self):
        return Process(_icebox.process_current())

    def find_name(self, name, flags):
        for p in self.list_all():
            got_name = os.path.basename(p.name())
            if got_name != name:
                continue

            got_flags = p.flags()
            if flags.is_x64 and not got_flags.is_x64:
                continue

            if flags.is_x86 and not got_flags.is_x86:
                continue

            return p
        return None

    def find_pid(self, pid):
        for p in self.list_all():
            if p.pid() == pid:
                return p
        return None

    def wait(self, name, flags):
        return Process(_icebox.process_wait(name, flags))

    def break_on_create(self, callback):
        fproc = lambda proc: callback(Process(proc))
        bpid = _icebox.process_listen_create(fproc)
        return Callback(bpid, fproc)

    def break_on_delete(self, callback):
        fproc = lambda proc: callback(Process(proc))
        bpid = _icebox.process_listen_delete(fproc)
        return Callback(bpid, fproc)

class Thread:
    def __init__(self, vm, thread):
        self.vm = vm
        self.thread = thread

    def process(self):
        pass

    def program_counter(self):
        pass

    def tid(self):
        pass

class Threads:
    def __init__(self, vm):
        self.vm = vm

    def enumerate(self):
        pass

    def current(self):
        pass

    def break_on_create(self):
        pass

    def break_on_delete(self):
        pass

class Vm:
    def __init__(self, name):
        curr = inspect.getsourcefile(lambda: 0)
        path = os.path.abspath(os.path.join(curr, ".."))
        sys.path.append(path)
        global _icebox
        import _icebox
        _icebox.attach(name)
        self.registers = Registers(_icebox.register_list, _icebox.register_read, _icebox.register_write)
        self.msr = Registers(_icebox.msr_list, _icebox.msr_read, _icebox.msr_write)
        self.threads = Threads(self)
        self.processes = Processes(self)

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

    def break_on(self, name, ptr, callback):
        pass

    def break_on_process(self, name, proc, ptr, callback):
        pass

    def break_on_thread(self, name, thread, ptr, callback):
        pass

    def break_on_physical(self, name, phy, callback):
        pass

    def break_on_physical_process(self, name, proc, phy, callback):
        pass