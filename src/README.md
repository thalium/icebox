1. Activate test signing
--------------------------
Winbagility ships with a modified version of VirtualBox, obviously not signed by Oracle. In order to install it, you need to activate test mode :
    open an admin prompt and type "bcdedit \set TESTSIGNING ON"
    restart your PC.
You should see "Mode Test" along with your Windows version number (major and build) on the bottom right corner of the Desktop.

2. Install VBoxDrv catalog
--------------------------
Install VBoxBin\VBoxDrv.inf by righ-clicking on it and selecting "install" in the context menu


3. Install Virtual Box
--------------------------

Open an admin prompt where the VBoxBin folder is located and type:

>>> set PATH:%PATH%;%~dp0kmk
>>> loadall.cmd

You should get Windows signing warning for VirtualBox drivers since those aren't signed by a trusted root publisher. You can safely ignore those since you activate self-seigned drivers loading. 