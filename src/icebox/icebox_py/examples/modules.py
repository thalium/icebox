import icebox

vm = icebox.attach("win10")

# list drivers
for drv in vm.drivers():
    addr, size = drv.span()
    print("%x-%x %s" % (addr, addr+size, drv.name()))

# find driver by address
proc = vm.processes.current()
vm.symbols.load_drivers()
addr = proc.symbols.address("ndis!NdisSendNetBufferLists")
drv = vm.drivers.find(addr)
print(drv.name())
assert(drv.name() == "\\SystemRoot\\system32\\drivers\\ndis.sys")

# list modules
for mod in proc.modules():
    addr, size = mod.span()
    flags = mod.flags()
    print("%x-%x: %s flags:%s" % (addr, addr+size, mod.name(), flags))


# call this function on every new module created
counter = icebox.counter()


def on_create(mod):
    addr, _ = mod.span()
    print("%x: %s" % (addr, mod.name()))
    counter.add()


# add breakpoint on module creation
proc = vm.processes.wait("notepad.exe")
with proc.modules.break_on_create(on_create):
    while counter.read() < 2:
        vm.exec()
