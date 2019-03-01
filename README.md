# IceBox

## Install
### Windows
Activate Intel VT-x instruction in your BIOS/UEFI Firmeware.
Deactivate Hyper-V.

Set your os in testsigning mode (in an admin prompt launch "bcdedit.exe -set TESTSIGNING on").\
You must also select 'Disable driver signature enforcement' at boot:
* Press Shift and click on Restart
* Troubleshoot -> Advanced Options -> Startup Settings -> Restart
* At boot press F7

You are ready to install IceBox.
Unzip %PROJECT_DIR%/bin/VBoxBin-r4.7z
In an admin prompt :
* cd %PROJECT_DIR%/bin/VBoxBin
* install.cmd

Two warning will appear, accept them.\
You can now launch VirtualBox.exe

### Linux
TODO
```bash
sudo ./vboxdrv.sh start
sudo chmod -R g+rw /dev/vboxdrv
./VirtualBox
```

### Limitations 
* Set only one CPU per virtual machine
* Do not allocate more that 8GB of RAM per virtual machine

## Build Vbox
TODO

## Build IceBox
### Windows
Generate solution, in a prompt:
```
cd %PROJECT_DIR%/build/
configure_2017.cmd
```

Open solution in Visual Studio (%PROJECT_DIR%/out/x64/icebox.sln) and build it !

**To launch debug:**\
Right click on the project where your main is -> Set as startup project.\
Set the command arguments is Debug -> Configure %PROJECT_NAME%... -> General -> Debugging -> Command arguments

### Linux
In a terminal:
```bash
cd %PROJECT_DIR%/build/
./configure.sh
cd ../out/x64
make
cd ../../bin/icebox
./$WHATEVER_BINARY
```

## Project Organisation
TODO

## Getting Started
TODO
