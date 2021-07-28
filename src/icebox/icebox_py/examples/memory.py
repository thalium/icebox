import icebox

vm = icebox.attach("win10")

# find a virtual address in current process to read
proc = vm.processes.current()
rip = vm.registers.rip

# read & write virtual memory
backup = proc.memory[rip : rip + 16]  # array-like reads
backup_bis = bytearray(16)
proc.memory.read(backup_bis, rip)
assert backup == backup_bis

proc.memory[rip] = 0xCC  # array-like writes
assert proc.memory[rip] == 0xCC
proc.memory[rip : rip + 16] = b"\x00" * len(backup)
proc.memory.write(rip, backup)

# convert virtual address to physical memory address
phy = proc.memory.physical_address(rip)
print("virtual 0x%x -> physical 0x%x" % (rip, phy))

# read & write physical memory
backup = vm.physical[phy : phy + 16]  # array-like reads
backup_bis = bytearray(16)
vm.physical.read(backup_bis, phy)
assert backup == backup_bis

vm.physical[phy] = 0xCC  # array-like writes
assert vm.physical[phy] == 0xCC
vm.physical[phy : phy + 16] = b"\x00" * len(backup)
vm.physical.write(phy, backup)
