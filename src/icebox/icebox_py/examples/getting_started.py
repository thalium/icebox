import icebox

vm = icebox.attach("win10")  # attach to vm named "win10"
proc = vm.processes.find_name("dwm.exe")  # find process named 'dwm'
print("%s pid:%d" % (proc.name(), proc.pid()))
for mod in ["kernel32", "kernelbase"]:
    proc.symbols.load_module(mod)  # load some symbols

counter = icebox.counter()  # run the vm until we've updated this counter twice


def dump_callstack():  # dump the current proc callstack
    print()  # skip a line
    for addr in proc.callstack():  # read current dwm.exe callstack
        print(proc.symbols.string(addr))  # convert & print callstack address
    counter.add()  # update counter


# set a breakpoint on ntdll!NtWaitForSingleObject
with vm.break_on_process(proc, "ntdll!NtWaitForSingleObject", dump_callstack):
    while counter.read() < 2:  # run until dump_callstack is called twice
        vm.exec()
