# Limitations
* Set only one CPU per virtual machine
* Do not allocate more that 8GB of RAM per virtual machine

# Windows
Activate Intel VT-x instruction in your BIOS/UEFI Firmeware.
Deactivate Hyper-V.

Set your os in testsigning mode (in an admin prompt launch "bcdedit.exe -set TESTSIGNING on").\
You must also select 'Disable driver signature enforcement' at boot:
* Press Shift and click on Restart
* Troubleshoot -> Advanced Options -> Startup Settings -> Restart
* At boot press F7

You are ready to install IceBox.

In an admin prompt :
* If you followed the build instructions ([BUILD.gen.md](/BUILD.gen.md))
```cmd
cd icebox/third_party/virtualbox/out/win.amd64/release/bin
```

* If not unzip icebox/bin/VBoxBin.7z<br>
```cmd
cd icebox/bin/VBoxBin
install.cmd
```

Two warning will appear, accept them.\
You can now launch VirtualBox.exe !

# Linux
Activate Intel VT-x instruction in your BIOS/UEFI Firmeware.
Deactivate Hyper-V.

Considering that you followed the build instructions ([BUILD.gen.md](/BUILD.gen.md))
```bash
cd icebox/third_party/virtualbox/out/linux.amd64/release/bin/src
make
sudo make install
cd ..
```

Create vboxuser group (where MYUSERNAME is your own username)
```bash
sudo groupadd vboxusers
sudo usermod -a -G vboxusers $MYUSERNAME
```

Start the driver
```bash
sudo ./vboxdrv.sh start
sudo chmod -R g+rw /dev/vboxdrv
```

You can now launch Vbox !
```bash
./VirtualBox
```
