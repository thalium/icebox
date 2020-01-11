Oracle VM VirtualBox Guest Additions

Where have the Windows drivers gone?
- The Windows Guest Additions drivers were removed from this directory to
  save space on your hard drive. To get the files you have to extract them
  from the Windows Guest Additions installers:

  To extract the 32-bit drivers to "C:\Drivers", do the following:
  VBoxWindowsAdditions-x86 /extract /D=C:\Drivers

  For the 64-bit drivers:
  VBoxWindowsAdditions-amd64 /extract /D=C:\Drivers

  Note: The extraction routine will create an additional sub directory
  with the selected architecture (x86 or amd64) to prevent mixing up
  the drivers.

  To get further help with the command line parameters of the installer,
  type: VBoxWindowsAdditions-<arch> /?
