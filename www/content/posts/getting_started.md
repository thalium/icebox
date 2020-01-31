---
title: "Getting Started with Icebox VMI"
date: 2020-01-24T12:00:00+01:00
draft: false
author: "Benoît Amiaux"
twitter: "bamiaux"
---

**Icebox** is a VMI (Virtual Machine Introspection) framework enabling you to stealthily trace and debug any kernel or user code system-wide.

All Icebox source code can be found on our [github page](https://github.com/thalium/icebox).

## Try Icebox

Icebox now comes with full Python bindings allowing fast prototyping on top of VMI, whether you want to trace a user process or inspect the kernel internals.

The core itself is in C++ and exposes most of its public functions into an `icebox` Python 3 module.

This article is a prelude to more technical articles exploring various kernel internals or malicious code behavior.

### User-land breakpoints & callstacks example

This is a short example showing a few features of the Icebox Python bindings.

This script will attach to a live VirtualBox VM running windows and named "win10".

Then it will find the Desktop Windows Manager process, break on every `ntdll!NtWaitForSingleObject` function call, and print the current callstack.

```python
import icebox

vm = icebox.attach("win10")  # attach to vm named "win10"
proc = vm.processes.find_name("dwm.exe")  # find process named 'dwm'
print("%s pid:%d" % (proc.name(), proc.pid()))
for mod in ["kernel32", "kernelbase"]:
    proc.symbols.load_module(mod)  # load some symbols

counter = icebox.counter()  # run the vm until we've updated this counter twice
def dump_callstack():  # dump the current proc callstack
    print()  # skip a line
    for addr in proc.callstack():  # read current dwm.exe callstack
        print(proc.symbols.string(addr))  # convert & print callstack address
    counter.add()  # update counter

# set a breakpoint on ntdll!NtWaitForSingleObject
with vm.break_on_process(proc, "ntdll!NtWaitForSingleObject", dump_callstack):
    while counter.read() < 2:  # run until dump_callstack is called twice
        vm.exec()
```

Full script can be found at `src/icebox/icebox_py/examples/modules.py`
Make sure you've started the "win10" VM first & run it with the following command:
```sh
PYTHONPATH=$icebox_dir/bin/x64 python3 \
    $icebox_dir/src/icebox/icebox_py/examples/getting_started.py
```

Output will look like this:

```
17:34:09.478 INFO| core: waiting for shm...
17:34:09.490 INFO| core: attached
17:34:09.505 INFO| nt: kernel: 0xfffff8003c8a4000-0xfffff8003d295000 size:0x9f1000
17:34:09.573 INFO| sym: loaded 69247313056076BBCB3411FD964287141 nt
17:34:09.594 INFO| nt: kernel: kpcr:0xfffff8003bc24000 kdtb:0x1aa002 version:10.0
17:34:09.603 INFO| nt: loading ntdll from process dwm.exe
17:34:09.623 INFO| sym: loaded 2055091C8F2C5808D8DFE02C75D129591 ntdll
dwm.exe pid:896
17:34:09.641 INFO| sym: loaded 7C073461BC41EEA8F242AD876C2F4A9F1 kernel32
17:34:09.689 INFO| sym: loaded 7BF8014B3D39BE6ADDC83DA4991008EB1 kernelbase

17:34:09.708 INFO| unwind: loading C:\Windows\SYSTEM32\ntdll.dll
17:34:09.721 INFO| unwind: loading C:\Windows\System32\KERNELBASE.dll
17:34:09.737 INFO| unwind: loading C:\Windows\system32\d3d10warp.dll
17:34:09.770 INFO| unwind: loading C:\Windows\System32\KERNEL32.DLL
ntdll!NtWaitForSingleObject
kernelbase!WaitForSingleObjectEx+0x93
d3d10warp+0xC16EF
d3d10warp+0x33FE25
d3d10warp+0x341F08
ntdll!TppWorkpExecuteCallback+0x130
ntdll!TppWorkerThread+0x644
kernel32!BaseThreadInitThunk+0x14
ntdll!RtlUserThreadStart+0x21

17:34:09.807 INFO| unwind: loading C:\Windows\system32\dwmcore.dll
ntdll!NtWaitForSingleObject
kernelbase!WaitForSingleObjectEx+0x93
dwmcore+0x52EDD
dwmcore+0x52986
dwmcore+0xB9321
kernel32!BaseThreadInitThunk+0x14
ntdll!RtlUserThreadStart+0x21
```

During startup, the Icebox core has attached itself to the live VirtualBox VM named "win10", detected various kernel structures, like the kernel base address, or the current KPCR (Kernel Processor Control Region).

The Desktop Windows Manager process has been found with PID 896 and we loaded symbols from modules "kernel32" & "kernelbase". Note that you will need to have _NT_SYMBOL_PATH defined so Icebox can read & parse PDBs.

Eventually, we break on every `ntdll!NtWaitForSingleObject` function call from dwm.exe and print the current callstack.

## Build Hypervisor Back-end

The first step to use Icebox is to build or download a customized hypervisor back-end. We currently support a customized VirtualBox hypervisor only.
- On Windows, you can download a ready-to-use compiled binary on the [releases](https://github.com/thalium/icebox/releases) github page.
- On Linux, we will need to compile our own version with the following instructions.

### Build VirtualBox

These instructions have been tested on Ubuntu 18.04.3 LTS.

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

On Windows, before using VirtualBox you will need to be able to load unsigned drivers first. Please be aware that enabling unsigned drivers is a security risk and take appropriate measures.
Use the following instructions to start VirtualBox on Windows using our precompiled binaries:

```
# start an admin console
# enable testsigning
bcdedit.exe -set TESTSIGNING on

# disable driver signature enforcement at boot
Shift-Click Reboot
Troubleshoot > Advanced Options > Startup Settings > Restart
Press F7

# unzip the release from https://github.com/thalium/icebox/releases
# start an admin console
install.cmd
# windows will ask you to confirm two unsigned drivers installation
# start VirtualBox.exe
```

## Prepare a Windows 10 VM

To use Icebox, you will need to prepare a VirtualBox VM, a full Windows 10 VM in this example.
There are only two prerequisites:
- Configure the VM to have only one CPU
- Configure the VM to have at most 8192 MB of RAM

We plan to possibly remove those two limitations eventually.

We will assume that your VM is named "win10" for the rest of this article.
We advise you to create a snapshot of the VM after installation so you can quickly restore a clean environment and skip the need to reboot the whole operating system.

## Build Icebox

We can now compile Icebox itself with the following instructions.
These instructions have been tested on Ubuntu 18.04.3 LTS.

```sh
cd $icebox_dir/build
NO_CLANG_FORMAT=1 ./configure.sh
cd $icebox_dir/out/x64
make -j2
```

## Download symbols

Before using Icebox on our "win10" VM, we need to download PDBs so we can understand & match addresses to symbols and read/write kernel structures.

For Windows guests, you will need at least matching ntkrnlmp and ntdll pdbs.

On Linux hosts, you can use the symbols.py script to automatically download missing symbols from Microsoft servers to a local directory.
Before running the following instructions, check that:
- The VM is running and named "win10"
- The Wow64 Task Manager is running in the guest (found at `c:\windows\syswow64\Taskmgr.exe`).

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

Note that downloading PDBs is also possible using `symchk.exe` tool from Microsoft, with a command like this:
```
symchk /r ntoskrnl.exe /od /s SRV*c:\symbols\*http://msdl.microsoft.com/download/symbols
```

## Run tests (optional)

You can run our automated tests to ensure everything is working, that you can import and use Icebox through Python.

```sh
# run python tests
python3 $icebox_dir/src/icebox/tests/win10.py $icebox_dir/bin/x64
```

## Icebox Bindings

Starting from here, we have:
- An hypervisor back-end, based on VirtualBox
- A live VM running Windows 10, named "win10"
- An Icebox Python module

We will now describe the various features Icebox expose to users.

### VM API

After importing `icebox`, you can attach to a live VM using `icebox.attach` function. It will return an `icebox.Vm` object allowing you to:
- Pause, resume, step once and control the VM execution
- Read & write common & MSR registers

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
Full script can be found at `src/icebox/icebox_py/examples/vm.py`.

You can run this example and all others with a command like this:
```sh
PYTHONPATH=$icebox_dir/bin/x64 python3 \
    $icebox_dir/src/icebox/icebox_py/examples/vm.py
```

### Process API

All process-related API can be found in `vm.processes`. You can:
- List all current processes
- Read the current process
- Read process PID, name and other properties
- Find any process by name or PID
- Join a process in kernel or user-mode
- Listen to create/delete process events

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
```
Full script can be found at `src/icebox/icebox_py/examples/process.py`.

### Threads API

All thread-related API can be found in `vm.threads` or by accessing threads through a process. You can:
- Read current thread
- List all process threads
- Read thread properties, like thread process or thread TID
- Listen to create/delete thread events

```python
# list current threads
proc = vm.processes.current()
for thread in proc.threads():
    print("%s: %d" % (proc.name(), thread.tid()))

thread = vm.threads.current()  # get current thread
proc_bis = thread.process()
assert(proc == proc_bis)

counter = icebox.counter()
def on_create(thread):
    print("+ %s: %d" % (thread.process().name(), thread.tid()))
    counter.add()

def on_delete(p):
    print("- %s: %d" % (thread.process().name(), thread.tid()))
    counter.add()

# put breakpoints on thread creation & deletion
with vm.threads.break_on_create(on_create):
    with vm.threads.break_on_delete(on_delete):
        while counter.read() < 4:
            vm.exec()
```
Full script can be found at `src/icebox/icebox_py/examples/threads.py`.

### Memory API

Both virtual & physical memory can be accessed for reading & writing.
You can:
- Read & write virtual memory from one process through `proc.memory`
- Convert a virtual address from one process to a physical address
- Read & write physical memory through `vm.physical`

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
Full script can be found at `src/icebox/icebox_py/examples/memory.py`.

### Symbols API

In order to ease manipulation of OS entities, we need two things:
- Symbols: converting a raw address to a string & vice-versa
- Types: reading structures & members from memory

Symbol-related API is mostly per-process and accessed through `proc.symbols`.

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

Full script can be found at `src/icebox/icebox_py/examples/symbols.py`.

### Modules & drivers API

Drivers are global OS entities accessible through `vm.drivers`.
Modules are per-process and accessible through `proc.modules`.
On both of them, you can:
- Find whether an address belong to any driver/module
- Load their symbols
- Read their base address, size and name
- Listen to their create/delete events

```python
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
```

Full script can be found at `src/icebox/icebox_py/examples/modules.py`.

### Breakpoints API

Icebox has various breakpoint handlers. When possible, use the right breakpoint so filtering is done in the core or better, in VirtualBox itself.

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
Full script can be found at `src/icebox/icebox_py/examples/breakpoints.py`.

### Funtions & callstacks API

Icebox offer various helpers, like `vm.functions` and `proc.callstack` allowing you to:
- Read & write function arguments easily
- Set a single-use breakpoint on function return
- Read callstack addresses

```python
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
```

Full script can be found at `src/icebox/icebox_py/examples/callstacks.py`.

## Conclusion

This concludes our overview of the **Icebox** Python API. Following these instructions, you should have a working Icebox installation with:
- An hypervisor backend based on VirtualBox & ready to run
- A Windows 10 VM and a ready-to-go snapshot
- A freshly compiled Icebox Python module which you can import and use

Icebox currently supports one backend, VirtualBox and two OS guests: Windows & Linux.
The goals of this project are to enable fast tool prototyping on top of VMI and novel ways to analyze code and kernel internals.

All source code can be found at our [github page](https://github.com/thalium/icebox). Comments, patches or suggestions are welcome!
