import icebox

vm = icebox.attach("win10")

proc = vm.processes.find_name("Taskmgr.exe")  # find Taskmgr process
proc.symbols.load_module("ntdll")


def print_hit():
    print("%s hit!" % vm.processes.current().name())


# add breakpoint on ntdll!NtQuerySystemInformation
addr = proc.symbols.address("ntdll!NtQuerySystemInformation")
with vm.break_on(addr, print_hit):
    vm.exec()

# filter breakpoint on process
with vm.break_on_process(proc, addr, print_hit):
    vm.exec()

# filter breakpoint on thread
thread = vm.threads.current()
with vm.break_on_thread(thread, addr, print_hit):
    vm.exec()

# add breakpoint on physical address
phy = proc.memory.physical_address(addr)
with vm.break_on_physical(phy, print_hit):
    vm.exec()
