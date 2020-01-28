import icebox

vm = icebox.attach("win10")

# load symbols from current module
# which will allow nice callstack symbols
proc = vm.processes.current()
proc.symbols.load_modules()

counter = icebox.counter()


def print_callstack():
    print()
    proc = vm.processes.current()
    for i, addr in enumerate(proc.callstack()):
        print("%2d: %s" % (i, proc.symbols.string(addr)))
    counter.add()


# print callstack at every ntdll!NtClose
addr = proc.symbols.address("ntdll!NtClose")
phy = proc.memory.physical_address(addr)
with vm.break_on_physical(phy, print_callstack):
    while counter.read() < 4:
        vm.exec()

# various function helpers
# only valid on function entry-point
_ = vm.functions.read_stack(0)  # indexed stack read
arg0 = vm.functions.read_arg(0)  # indexed arg read
vm.functions.write_arg(0, arg0)  # indexed arg write

# add single-use breakpoint on function return
addrs = [x for x in vm.processes.current().callstack()]
vm.functions.break_on_return(lambda: None)
vm.exec()

# check callstack
addrs_return = [x for x in vm.processes.current().callstack()]
assert(addrs[1:] == addrs_return)
