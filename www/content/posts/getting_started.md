---
title: "Getting Started with Icebox VMI"
date: 2020-01-22T10:12:57+01:00
draft: true
---

**Icebox** is a VMI (Virtual Machine Introspection) enabling you to stealthily trace and debug any process system-wide.
all **Icebox** code can be found on our [github](https://github.com/thalium/icebox)

## Try Icebox

**Icebox** now comes with new python bindings allowing fast prototyping on top of VMI, whether you want to trace a user process or inspect the kernel internals. 
Here's an example to give you a taste of what's possible.

```python
import icebox

vm = icebox.attach("win10")    # attach to vm named "win10"
vm.symbols.load_driver("ndis") # load ndis driver symbols

# convert ndis!NdisSendNetBufferLists to physical address
proc = vm.processes.current()
addr = proc.symbols.address("ndis!NdisSendNetBufferLists")
phy = proc.memory.physical_address(addr)

# called on every buffer sent from nt kernel
def on_ndis_send():
    net_buffer_list = vm.registers.rdx
    # append this network buffer to a pcap file
    dump_net_buffer_to_pcap(net_buffer_list)

# put breakpoint on every ndis!NdisSendNetBufferLists
with vm.break_on_physical("ndis!NdisSendNetBufferLists", phy, on_ndis_send):
    for i in range(0, 100):
        vm.exec()
``` 


```sh
# export icebox_dir on your root icebox directory
export icebox_dir=~/icebox

# install prerequisites
sudo apt install acpica-tools build-essential g++-multilib gcc-multilib \
    libcap-dev libcurl4-openssl-dev libdevmapper-dev libidl-dev libelf-dev \
    libopus-dev libpam0g-dev libqt5x11extras5-dev libsdl1.2-dev libsdl2-dev \
    libssl-dev libvpx-dev libxml2-dev libxmu-dev linux-headers-$(uname -r) \
    linux-libc-dev makeself p7zip-full python-dev qt5-default \
    qttools5-dev-tools xsltproc

# include fdp into virtualbox
cd $icebox_dir/third_party/virtualbox/include
ln -s ../../../src/FDP

# build virtualbox
cd $icebox_dir/third_party/virtualbox
./configure --disable-hardening --disable-docs --disable-java
source env.sh
kmk VBOX_WITH_ADDITIONS= VBOX_WITH_TESTCASES= VBOX_WITH_TESTSUITE= VBOX_DO_STRIP=1

# build & install virtualbox kernel modules
cd $icebox_dir/third_party/virtualbox/out/linux.amd64/release/bin/src
make
sudo make install

# add yourself to vboxusers group
sudo groupadd vboxusers
sudo usermod -a -G vboxusers $USERNAME

# start vboxdrv driver
cd $icebox_dir/third_party/virtualbox/out/linux.amd64/release/bin
sudo ./vboxdrv.sh start
sudo chmod g+rw /dev/vboxdrv

# start virtualbox
cd $icebox_dir/third_party/virtualbox/out/linux.amd64/release/bin
./VirtualBox &
```

You can now install a windows 10 VM. You can use the latest version, 1909 at the redaction of this text, or an older version like 1803. We will assume now that the virtual machine name is 'win10'.

```sh
cd $icebox_dir/build
NO_CLANG_FORMAT=1 ./configure.sh
cd $icebox_dir/out/x64
make -j2
```

```sh
# setup symbols directory
cd $icebox_dir
mkdir symbols
export _NT_SYMBOL_PATH=$icebox_dir/symbols
sudo apt install python3-pip
pip3 install tqdm
cd $icebox_dir/bin/x64
# download all pdbs from x64 & wow64 process
# this will take a while depending on your network speed
python3 $icebox_dir/build/symdl.py init win10
```

```sh
# run python tests
python3 $icebox_dir/src/icebox/tests/win10.py $icebox_dir/bin/x64
```