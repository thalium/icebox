import icebox

vm = icebox.attach("win10")

vm.symbols.load_driver("hal")  # load driver symbols
vm.symbols.load_drivers()  # load all driver symbols

proc = vm.processes.find_name("Taskmgr.exe", icebox.flags_x86)
proc.symbols.load_module("ntdll")  # load module symbol
proc.symbols.load_modules()  # load all module symbols

lstar = vm.msr.lstar
symbol = proc.symbols.string(lstar)  # convert address to string
print("%s = 0x%x" % (symbol, lstar))
addr = proc.symbols.address(symbol)  # convert string to address
assert lstar == addr

proc.join_kernel()
print(proc.symbols.string(vm.registers.rip))

proc.join_user()
print(proc.symbols.string(vm.registers.rip))

# list all known strucs for named module
count = 0
for struc_name in proc.symbols.strucs("nt"):
    count += 1
print("nt: %d structs" % count)

# access struc properties & members
struc = proc.symbols.struc("nt!_EPROCESS")  # read struc
assert struc.name == "_EPROCESS"
assert struc.size > 0
assert len(struc.members) > 0
member = [x for x in struc.members if x.name == "ActiveProcessLinks"][0]
assert member.name == "ActiveProcessLinks"
assert member.bits > 0
assert member.offset > 0

# dump type at input address
proc.symbols.dump_type("nt!_KPCR", vm.msr.kernel_gs_base)
