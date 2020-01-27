import icebox

# attach to 'win10' VM
vm = icebox.attach("win10")

# control vm execution
vm.resume()
vm.pause()
vm.step_once()

# read/write registers
# rax, rbx, ..., rbp, rip
print(hex(vm.registers.rip))

rax = vm.registers.rax
vm.registers.rax += 1
vm.registers.rax = rax

# read/write MSR registers
print(hex(vm.msr.lstar))
