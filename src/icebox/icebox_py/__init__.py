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
