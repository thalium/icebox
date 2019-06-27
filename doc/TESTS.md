## Common statements

After building, tests can be launch with the _icebox_tests_ binary.

<br>

## Linux Guest

To launch linux tests specifically, use command line __icebox_tests --gtest_filter=Linux*__

You must have a linux guest running in the modified VirtualBox. The VM must be named "linux".

Finaly, Icebox must have the right profil of the linux kernel running as guest. See doc/make_profil_linux/make_profil.md and doc/USE.md for more details.

#### VM prerequisites
To be able to test all the features of Icebox, the linux guest must have these prerequisites achieved :
1. Launch __./linux_tst_ibx &__ (this file should be found in doc/tests__utilities)<br>
It may need to install gcc-multilib before running it (because it is a 32 bits elf).<br><br>
_linux_tst_ibx.c_ is also available if you need to build it again. Take aware to build it as a __32 bits__ elf : __gcc -pthread -m32 -o linux_tst_ibx linux_tst_ibx.c__<br><br>
2. And that's it.