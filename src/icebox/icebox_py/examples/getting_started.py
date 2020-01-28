import icebox

vm = icebox.attach("win10")  # attach to vm named "win10"
proc = vm.processes.find_name("Taskmgr.exe")  # find process named 'Taskmgr'
print("%s pid:%d" % (proc.name(), proc.pid()))
proc.symbols.load_modules()  # load all Taskmgr module symbols

counter = icebox.counter()  # create a shared counter


def print_taskmgr_callstack():  # callback called on every breakpoint
    print()  # skip a line
    for addr in proc.callstack():  # read current Taskmgr callstack
        print(proc.symbols.string(addr))  # convert & print address
    counter.add()


# set a breakpoint on ntdll!NtQuerySystemInformation
addr = "ntdll!NtQuerySystemInformation"
with vm.break_on_process(proc, addr, print_taskmgr_callstack):
    while counter.read() < 2:  # run until callback is called twice
        vm.exec()
