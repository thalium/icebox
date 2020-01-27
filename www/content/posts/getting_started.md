---
title: "Getting Started with Icebox VMI"
date: 2020-01-24T12:00:00+01:00
draft: false
author: "Benoît Amiaux"
twitter: "bamiaux"
---

**Icebox** is a VMI (Virtual Machine Introspection) framework enabling you to stealthily trace and debug any process system-wide.
All Icebox code can be found on our [github](https://github.com/thalium/icebox)

## Try Icebox

Icebox now comes with new python bindings allowing fast prototyping on top of VMI, whether you want to trace a user process or inspect the kernel internals. 

### User-land breakpoints & callstacks example

Here's a short example on how to break a user-land windows 10 process on every `ntdll!NtQuerySystemInformation` call. You will need to start the task manager in windows prior to running this script.

```python
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
``` 

Running this script on a live VM will return an output looking like this:

```
17:56:58.458 INFO| core: waiting for shm...
17:56:58.464 INFO| core: attached
17:56:58.472 INFO| nt: kernel: 0xfffff8044c000000 - 0xfffff8044cab6000 (11231232 0xab6000)
17:56:58.516 INFO| sym: loaded E0093F3AEF15D58168B753C9488A40431 nt
17:56:58.548 INFO| nt: kernel: kpcr: 0xfffff804493f2000 kdtb: 0x80000000001aa002 version: 10.0
17:56:58.550 INFO| nt: loading ntdll from process Taskmgr.exe
17:56:58.556 INFO| sym: loaded 0C2E19EA1901E9B82E4567D2D21E56D21 ntdll
Taskmgr.exe pid:1688
17:56:58.564 INFO| sym: loaded 637A6ADE3949DFB52DD9431EBE3812EE1 Taskmgr
[...] truncated

ntdll!NtQuerySystemInformation
Taskmgr!?WdcQueryProcessInformation@@YAJPEAU_WDC_EXPANDING_CALL@@@Z+0x78
Taskmgr!?WdcExpandingCall@@YAJP6AJPEAU_WDC_EXPANDING_CALL@@@ZPEA_KPEAPEAXKZZ+0x83
Taskmgr!?Update@WdcApplicationsMonitor@@MEAAJXZ+0xC6
Taskmgr!?DoUpdates@WdcDataMonitor@@MEAAJXZ+0xB8
Taskmgr!?UpdateThread@WdcDataMonitor@@KAKPEAX@Z+0x4D
kernel32!BaseThreadInitThunk+0x14
ntdll!RtlUserThreadStart+0x21

ntdll!NtQuerySystemInformation
kernelbase!GlobalMemoryStatusEx+0x3E
Taskmgr!?GetMemoryLoad@CRUMAPIHelper@@QEAAJPEAK@Z+0x33
Taskmgr!?CalcSysMemMetrics@CRUMHelper@@AEAAJXZ+0x18
Taskmgr!?Pass1CalcSysUtilization@CRUMHelper@@QEAAJXZ+0x51
Taskmgr!?Update@WdcApplicationsMonitor@@MEAAJXZ+0x224
Taskmgr!?DoUpdates@WdcDataMonitor@@MEAAJXZ+0xB8
Taskmgr!?UpdateThread@WdcDataMonitor@@KAKPEAX@Z+0x4D
kernel32!BaseThreadInitThunk+0x14
ntdll!RtlUserThreadStart+0x21
```

Icebox has attached to a live VM named "win10", read various kernel structures, like the kernel base address or the current KPCR.
It found Taskmgr process at pid 1688 and loaded symbols from all its modules like ntdll.dll. Note that you will need to have the environment variable _NT_SYMBOL_PATH defined so icebox can find PDBs.
Finally, we print callstacks on every `ntdll!NtQuerySystemInformation` call filtered on the Taskmgr.exe process.

## Build Hypervisor Back-end

The first step to use Icebox is to build or download a customized hypervisor back-end. We only currently support a customized VirtualBox hypervisor. On Windows, you can download a ready-to-use compiled binary on the [releases](https://github.com/thalium/icebox/releases) github page.
On Linux, we will need to compile our own version with the following instructions.

### Build VirtualBox

These instructions have been tested on ubuntu 18.04.3 LTS.

```sh
# export icebox_dir as your root icebox directory
export icebox_dir=~/icebox

# clone icebox repository
git clone https://github.com/thalium/icebox.git $icebox_dir

# install prerequisites
sudo apt install acpica-tools build-essential g++-multilib gcc-multilib \
    libcap-dev libcurl4-openssl-dev libdevmapper-dev libidl-dev libelf-dev \
    libopus-dev libpam0g-dev libqt5x11extras5-dev libsdl1.2-dev libsdl2-dev \
    libssl-dev libvpx-dev libxml2-dev libxmu-dev linux-headers-$(uname -r) \
    linux-libc-dev makeself p7zip-full python-dev qt5-default \
    qttools5-dev-tools xsltproc

# include fdp into virtualbox
cd $icebox_dir/third_party/virtualbox/include
ln -s $icebox_dir/src/FDP

# build virtualbox
cd $icebox_dir/third_party/virtualbox
./configure --disable-hardening --disable-docs --disable-java
source env.sh
kmk VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= \
    VBOX_DO_STRIP=1

# build & install virtualbox kernel modules
cd $icebox_dir/third_party/virtualbox/out/linux.amd64/release/bin/src
make
sudo make install

# add yourself to vboxusers group
# you may need to logout/login or reboot for these changes to take effect
sudo groupadd vboxusers
sudo usermod -a -G vboxusers $USERNAME
```

## Start VirtualBox

You can now start & run VirtualBox using the following instructions:

```sh
# start vboxdrv driver
cd $icebox_dir/third_party/virtualbox/out/linux.amd64/release/bin
sudo ./vboxdrv.sh start
sudo chmod g+rw /dev/vboxdrv

# start virtualbox GUI
cd $icebox_dir/third_party/virtualbox/out/linux.amd64/release/bin
./VirtualBox &
```

## Prepare a Windows 10 VM

To use Icebox, you will need to prepare a VirtualBox VM, a full Windows 10 VM in this example.
There are only two prerequisites:
- Configure the VM to have only one CPU
- Configure the VM to have exactly 8192 MB of RAM

Those two limitations may be removed eventually.

We will assume that your vm is named "win10" for the rest of this article.

## Build Icebox

We can now compile Icebox itself with the following instructions. These instructions have been tested on Ubuntu 18.04.3 LTS.

```sh
cd $icebox_dir/build
NO_CLANG_FORMAT=1 ./configure.sh
cd $icebox_dir/out/x64
make -j2
```

## Download symbols

Before using Icebox on our "win10" VM, we need to download PDBs so we can understand & match addresses to symbols and read/write kernel structures.

On linux, you can use the symbols.py script to automatically download missing symbols from microsoft servers to a local directory:

```sh
# setup symbols directory
cd
mkdir symbols
export _NT_SYMBOL_PATH=~/symbols

# install symbols.py prerequisites
sudo apt install python3-pip
pip3 install tqdm
cd $icebox_dir/bin/x64

# download all pdbs from x64 & wow64 processes
# make sure to start c:\windows\syswow64\Taskmgr.exe first
# this will take a while depending on your network speed
python3 $icebox_dir/src/icebox/icebox_py/symbols.py download win10

# in case of errors, you can try the following script with verbose output
python3 $icebox_dir/src/icebox/icebox_py/symbols.py check win10
```

## Run tests (optional)

You can run our automated tests to ensure everything is working, that you can import and use icebox through Python.

```sh
# run python tests
python3 $icebox_dir/src/icebox/tests/win10.py $icebox_dir/bin/x64
```

## Icebox Bindings

Starting from here, we will describe the Icebox python API

### VM API

The VM API allows you to attach to a named VM & control it.
Full script can be found at `src/icebox/icebox_py/examples/vm.py`

```python
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
```

### Process API

You can manipulate processes as entities.
Full script can be found at `src/icebox/icebox_py/examples/process.py`

```python
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
```

### Threads API

You can also manipulate process threads.
Full script can be found at `src/icebox/icebox_py/examples/threads.py`

```python
# list current threads
proc = vm.processes.current()
for thread in proc.threads():
    print("%s: %d" % (proc.name(), thread.tid()))

thread = vm.threads.current()  # get current thread
proc_bis = thread.process()
assert(proc == proc_bis)

def on_create(thread):
    print("+ %s: %d" % (thread.process().name(), thread.tid()))

def on_delete(p):
    print("- %s: %d" % (thread.process().name(), thread.tid()))

# put breakpoints on thread creation & deletion
with vm.threads.break_on_create(on_create):
    with vm.threads.break_on_delete(on_delete):
        for i in range(0, 4):
            vm.exec()
```

### Symbols API

Symbols can be manipulated per-process.
Full script can be found at `src/icebox/icebox_py/examples/symbols.py`

```python
vm.symbols.load_driver("hal")  # load driver symbols
vm.symbols.load_drivers()  # load all driver symbols

proc = vm.processes.find_name("Taskmgr.exe", icebox.flags_x86)
proc.symbols.load_module("ntdll")  # load module symbol
proc.symbols.load_modules()  # load all module symbols

lstar = vm.msr.lstar
symbol = proc.symbols.string(lstar)  # convert address to string
print("%s = 0x%x" % (symbol, lstar))
addr = proc.symbols.address(symbol)  # convert string to address
assert(lstar == addr)

proc.join("kernel")
print(proc.symbols.string(vm.registers.rip))

proc.join("user")
print(proc.symbols.string(vm.registers.rip))

# list all known strucs for named module
count = 0
for struc_name in proc.symbols.strucs("nt"):
    count += 1
print("nt: %d structs" % count)

# access struc properties & members
struc = proc.symbols.struc("nt!_EPROCESS")
assert(struc.name == "_EPROCESS")
assert(struc.size > 0)
assert(len(struc.members) > 0)
member = [x for x in struc.members if x.name == "ActiveProcessLinks"][0]
assert(member.name == "ActiveProcessLinks")
assert(member.bits > 0)
assert(member.offset > 0)

# dump type at input address
proc.symbols.dump_type("nt!_KPCR", vm.msr.kernel_gs_base)
```

### Memory API

Both virtual & physical memory can be accessed for read & write.
Full script can be found at `src/icebox/icebox_py/examples/memory.py`

```python
# find a virtual address in current process to read
proc = vm.processes.current()
rip = vm.registers.rip

# read & write virtual memory
backup = proc.memory[rip: rip+16]  # array-like reads
backup_bis = bytearray(16)
proc.memory.read(backup_bis, rip)
assert(backup == backup_bis)

proc.memory[rip] = 0xcc  # array-like writes
assert(proc.memory[rip] == 0xcc)
proc.memory[rip: rip+16] = b"\x00" * len(backup)
proc.memory.write(rip, backup)

# convert virtual address to physical memory address
phy = proc.memory.physical_address(rip)
print("virtual 0x%x -> physical 0x%x" % (rip, phy))

# read & write physical memory
backup = vm.physical[phy: phy+16]  # array-like reads
backup_bis = bytearray(16)
vm.physical.read(backup_bis, phy)
assert(backup == backup_bis)

vm.physical[phy] = 0xcc  # array-like writes
assert(vm.physical[phy] == 0xcc)
vm.physical[phy: phy+16] = b"\x00" * len(backup)
vm.physical.write(phy, backup)
```

### Modules & drivers API

Modules & drivers are first-class entities in Icebox.
Full script can be found at `src/icebox/icebox_py/examples/memory.py`

```python
# list drivers
for drv in vm.drivers():
    addr, size = drv.span()
    print("%x-%x %s" % (addr, addr+size, drv.name()))

# find driver by address
p = vm.processes.current()
vm.symbols.load_drivers()
addr = p.symbols.address("ndis!NdisSendNetBufferLists")
drv = vm.drivers.find(addr)
print(drv.name())
assert(drv.name() == "\\SystemRoot\\system32\\drivers\\ndis.sys")

# list modules
for mod in p.modules():
    addr, size = mod.span()
    flags = mod.flags()
    print("%x-%x: %s flags:%s" % (addr, addr+size, mod.name(), flags))

# call this function on every new module created
def on_create(mod):
    addr, _ = mod.span()
    print("%x: %s" % (addr, mod.name()))

# add breakpoint on module creation
p = vm.processes.wait("notepad.exe")
with p.modules.break_on_create(on_create):
    for i in range(0, 2):
        vm.exec()
```

### Breakpoint API

Icebox have various breakpoint handlers accessible with python.
Full script can be found at `src/icebox/icebox_py/examples/breakpoints.py`

```python
proc = vm.processes.find_name("Taskmgr.exe")  # find Taskmgr process
proc.symbols.load_module("ntdll")

# print current process name callback
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
```

### Funtions & callstacks API

Icebox offer useful helpers, like functions & callstacks
Full script can be found at `src/icebox/icebox_py/examples/callstacks.py`

```python
# load symbols from current module
# which will allow nice callstack symbols
proc = vm.processes.current()
proc.symbols.load_modules()

def print_callstack():
    print()
    proc = vm.processes.current()
    for i, addr in enumerate(proc.callstack()):
        print("%2d: %s" % (i, proc.symbols.string(addr)))

# print callstack at every ntdll!NtClose
addr = proc.symbols.address("ntdll!NtClose")
phy = proc.memory.physical_address(addr)
with vm.break_on_physical(phy, print_callstack):
    for i in range(0, 4):
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
```