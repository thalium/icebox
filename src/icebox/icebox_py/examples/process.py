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

proc.join("kernel")
print(hex(vm.registers.rip))

proc.join("user")
print(hex(vm.registers.rip))


def on_create(proc):
    print("+ %d: %s" % (proc.pid(), proc.name()))


def on_delete(proc):
    print("- %d: %s" % (proc.pid(), proc.name()))


# put breakpoints on process creation & deletion
with vm.processes.break_on_create(on_create):
    with vm.processes.break_on_delete(on_delete):
        for i in range(0, 4):
            vm.exec()
