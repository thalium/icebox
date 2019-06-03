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

  If you followed the build instructions ([BUILD.gen.md](/doc/BUILD.gen.md#job-msvc-1)) go to:
```cmd
icebox/third_party/virtualbox/out/win.amd64/release/bin
```

If not unzip the release. Then in an admin prompt:<br>
```cmd
install.cmd
```

Two warning will appear, accept them.\
You can now launch VirtualBox.exe !

# Linux
Activate Intel VT-x instruction in your BIOS/UEFI Firmeware.
Deactivate Hyper-V.

If you followed the build instructions ([BUILD.gen.md](/doc/BUILD.gen.md#job-gcc-1)) go to:
```bash
icebox/third_party/virtualbox/out/linux.amd64/release/bin/
```

If not unzip the release. Then create vboxuser group (where MYUSERNAME is your own username)
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
