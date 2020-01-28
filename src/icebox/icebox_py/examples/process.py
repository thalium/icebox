import icebox

vm = icebox.attach("win10")

# list current processes
for proc in vm.processes():
    print("%d: %s" % (proc.pid(), proc.name()))

proc = vm.processes.current()  # get current process
proc = vm.processes.find_name("explorer.exe")  # get explorer.exe
proc = vm.processes.find_name("Taskmgr.exe", icebox.flags_x86)
proc = vm.processes.find_pid(4)  # get process by pid
proc = vm.processes.wait("Taskmgr.exe")  # get or wait for process to begin

assert(proc.is_valid())
assert(proc.name() == "Taskmgr.exe")
assert(proc.pid() > 0)
assert(proc.flags() == icebox.flags_x86)
assert(proc.parent())

proc.join_kernel()
print(hex(vm.registers.rip))

proc.join_user()
print(hex(vm.registers.rip))

counter = icebox.counter()


def on_create(proc):
    print("+ %d: %s" % (proc.pid(), proc.name()))
    counter.add()


def on_delete(proc):
    print("- %d: %s" % (proc.pid(), proc.name()))
    counter.add()


# put breakpoints on process creation & deletion
with vm.processes.break_on_create(on_create):
    with vm.processes.break_on_delete(on_delete):
        while counter.read() < 4:
            vm.exec()
