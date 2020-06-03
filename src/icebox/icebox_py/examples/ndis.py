import icebox

vm = icebox.attach("win10")
vm.symbols.load_drivers()
proc = vm.processes.current()
addr = proc.symbols.address("ndis!NdisSendNetBufferLists")
phy = proc.memory.physical_address(addr)

def on_break():
    p = vm.processes.current()
    p.symbols.load_modules()
    p.symbols.dump_type("ndis!_NET_BUFFER_LIST", vm.registers.rdx)
    for addr in p.callstack():
        print(p.symbols.string(addr))

with vm.break_on_physical(phy, on_break):
    vm.exec()
