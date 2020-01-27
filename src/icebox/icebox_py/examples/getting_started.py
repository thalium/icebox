import icebox

vm = icebox.attach("win10")  # attach to vm named "win10"
proc = vm.processes.find_name("Taskmgr.exe")  # find process named 'Taskmgr'
print("%s pid:%d" % (proc.name(), proc.pid()))
proc.symbols.load_modules()  # load all Taskmgr module symbols


def print_taskmgr_callstack():
    print()
    for addr in proc.callstack():
        print(proc.symbols.string(addr))


# set a breakpoint on ntdll!NtQuerySystemInformation
addr = "ntdll!NtQuerySystemInformation"
with vm.break_on_process(proc, addr, print_taskmgr_callstack):
    for i in range(0, 2):
        vm.exec()  # execute vm until break
